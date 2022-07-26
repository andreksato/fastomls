#include "packingproblem.h"
#include <QFileInfo>
#include <QtMath>
#include "polybool/polybool.h"
#include "annealing/cShape.h"
#include "annealing/cRectangularContainer.h"
#include "annealing/cShapeInlines.h"
#include "annealing/problemInstance.h"

#define RASTER_EPS 0.0001
using namespace RASTERPACKING;

void Bresenham(int x1, int y1, int const x2, int const y2, QImage &image) {
	int delta_x(x2 - x1);
	// if x1 == x2, then it does not matter what we set here
	signed char const ix((delta_x > 0) - (delta_x < 0));
	delta_x = std::abs(delta_x) << 1;

	int delta_y(y2 - y1);
	// if y1 == y2, then it does not matter what we set here
	signed char const iy((delta_y > 0) - (delta_y < 0));
	delta_y = std::abs(delta_y) << 1;

	image.setPixel(x1, y1, 0);

	if (delta_x >= delta_y)
	{
		// error may go below zero
		int error(delta_y - (delta_x >> 1));

		while (x1 != x2)
		{
			if ((error >= 0) && (error || (ix > 0)))
			{
				error -= delta_x;
				y1 += iy;
			}
			// else do nothing

			error += delta_y;
			x1 += ix;

			image.setPixel(x1, y1, 0);
		}
	}
	else
	{
		// error may go below zero
		int error(delta_x - (delta_y >> 1));

		while (y1 != y2)
		{
			if ((error >= 0) && (error || (iy > 0)))
			{
				error -= delta_y;
				x1 += ix;
			}
			// else do nothing

			error += delta_x;
			y1 += iy;

			image.setPixel(x1, y1, 0);
		}
	}
}

void BresenhamVec(int x1, int y1, int const x2, int const y2, int *S, int width) {
	int delta_x(x2 - x1);
	// if x1 == x2, then it does not matter what we set here
	signed char const ix((delta_x > 0) - (delta_x < 0));
	delta_x = std::abs(delta_x) << 1;

	int delta_y(y2 - y1);
	// if y1 == y2, then it does not matter what we set here
	signed char const iy((delta_y > 0) - (delta_y < 0));
	delta_y = std::abs(delta_y) << 1;

	S[y1*width + x1] = 1;

	if (delta_x >= delta_y)
	{
		// error may go below zero
		int error(delta_y - (delta_x >> 1));

		while (x1 != x2)
		{
			if ((error >= 0) && (error || (ix > 0)))
			{
				error -= delta_x;
				y1 += iy;
			}
			// else do nothing

			error += delta_y;
			x1 += ix;

			S[y1*width + x1] = 1;
		}
	}
	else
	{
		// error may go below zero
		int error(delta_x - (delta_y >> 1));

		while (y1 != y2)
		{
			if ((error >= 0) && (error || (iy > 0)))
			{
				error -= delta_y;
				x1 += ix;
			}
			// else do nothing

			error += delta_x;
			y1 += iy;

			S[y1*width + x1] = 1;
		}
	}
}

int *Polygon::getRasterImageVector(QPoint &RP, qreal scale, int &width, int &height) {
	QPolygonF polygon;
	qreal xMin, xMax, yMin, yMax;

	const_iterator it = cbegin();
	xMin = scale*(*it).x(); xMax = scale*(*it).x();
	yMin = scale*(*it).y(); yMax = scale*(*it).y();
	for (int i = 0; i < size(); i++) {
		qreal x = scale*at(i).x();
		qreal y = scale*at(i).y();

		if (x < xMin) xMin = x;
		if (x > xMax) xMax = x;
		if (y < yMin) yMin = y;
		if (y > yMax) yMax = y;

		polygon << QPointF(x, y);
	}
	RP.setX(-qRound(xMin)); RP.setY(-qRound(yMin));
	polygon.translate(-xMin, -yMin);

	width = qRound(xMax - xMin) + 1;
	height = qRound(yMax - yMin) + 1;
	int *S = new int[width*height];
	std::fill_n(S, width*height, 1);

	int curScanline = width;
	for (int pixelY = 1; pixelY < height - 1; pixelY++) {

		//  Build a list of nodes.
		QVector<int> nodeX;
		int j = polygon.size() - 1;
		for (int i = 0; i < polygon.size(); i++) {
			qreal polyYi = polygon[i].y();
			qreal polyXi = polygon[i].x();
			qreal polyYj = polygon[j].y();
			qreal polyXj = polygon[j].x();

			if ((polyYi<(double)pixelY && polyYj >= (double)pixelY) || (polyYj<(double)pixelY && polyYi >= (double)pixelY)) {
				nodeX.push_back((int)(polyXi + (pixelY - polyYi) / (polyYj - polyYi)*(polyXj - polyXi)));
			}
			j = i;
		}

		// Sort
		qSort(nodeX);


		//  Fill the pixels between node pairs.
		for (int i = 0; i<nodeX.size(); i += 2) {
			int line = curScanline;
			line += nodeX[i] + 1;
			for (j = nodeX[i] + 1; j < nodeX[i + 1]; j++, line += 1)
				S[line] = 0;
		}

		curScanline += width;
	}


	// FIXME: Delete horizontal lines
	QPolygonF::const_iterator itj;
	for (QPolygonF::const_iterator iti = polygon.cbegin(); iti != polygon.cend(); iti++) {
		if (iti + 1 == polygon.cend()) itj = polygon.cbegin();
		else itj = iti + 1;
		if ((int)(*iti).y() == (int)(*itj).y()) {
			int left = (*iti).x() < (*itj).x() ? (*iti).x() : (*itj).x();
			int right = (*iti).x() < (*itj).x() ? (*itj).x() : (*iti).x();
			for (int i = left; i <= right; i++) {
				S[(int)(*iti).y()*width + i] = 1;
			}
		}
	}

	// FIXME: Test
	for (QVector<QPair<QPointF, QPointF>>::const_iterator it = this->degLines.cbegin(); it != this->degLines.cend(); it++) {
		QPointF p1 = scale*(*it).first; p1 -= QPointF(xMin, yMin);
		QPointF p2 = scale*(*it).second; p2 -= QPointF(xMin, yMin);
		BresenhamVec(qRound(p1.x()), qRound(p1.y()), qRound(p2.x()), qRound(p2.y()), S, width);
	}
	for (QVector<QPointF>::const_iterator it = this->degNodes.cbegin(); it != this->degNodes.cend(); it++) {
		QPointF p1 = scale*(*it); p1 -= QPointF(xMin, yMin);
		S[(int)p1.y()*width + (int)p1.x()] = 1;
	}

	return S;
}

bool compareEpsilon(qreal v1, qreal v2, qreal eps) {
	return qAbs(v2 - v1) < eps;
}

bool lessThanEqualEpsilon(qreal v1, qreal v2, qreal eps) {
	if (qAbs(v2 - v1) < eps) return true;
	else return v1 < v2;
}

bool greaterThanEqualEpsilon(qreal v1, qreal v2, qreal eps) {
	if (qAbs(v2 - v1) < eps) return true;
	else return v1 > v2;

}

bool lessThanNotEqualEpsilon(qreal v1, qreal v2, qreal eps) {
	if (qAbs(v2 - v1) < eps) return false;
	else return v1 < v2;
}

int *Polygon::getRasterImageVectorWithContour(QPoint &RP, qreal scale, int &width, int &height) {
	QPolygonF polygon;
	qreal xMin, xMax, yMin, yMax;

	const_iterator it = cbegin();
	xMin = scale*(*it).x(); xMax = scale*(*it).x();
	yMin = scale*(*it).y(); yMax = scale*(*it).y();
	for (int i = 0; i < size(); i++) {
		qreal x = scale*at(i).x();
		qreal y = scale*at(i).y();

		if (x < xMin) xMin = x;
		if (x > xMax) xMax = x;
		if (y < yMin) yMin = y;
		if (y > yMax) yMax = y;

		polygon << QPointF(x, y);
	}
	RP.setX(-qFloor(xMin)); RP.setY(-qFloor(yMin));
	polygon.translate(RP.x(), RP.y());

	width = qCeil(xMax) + RP.x() + 1;
	height = qCeil(yMax) + RP.y() + 1;
	int *S = new int[width*height];
	std::fill_n(S, width*height, 1);

	int curScanline = 0;
	for (int pixelY = 0; pixelY < height; pixelY++) {
		//  Build a list of intersection nodes.
		QVector<qreal> nodeX;
		int j = polygon.size() - 1;
		for (int i = 0; i < polygon.size(); i++) {
			qreal polyXi = polygon[i].x(); qreal polyYi = polygon[i].y();
			qreal polyXj = polygon[j].x(); qreal polyYj = polygon[j].y();

			// Check if lines are horizontal
			if (compareEpsilon(polyYi, polyYj, RASTER_EPS)) {
				j = i; continue;
			}

			qreal curY = (qreal)pixelY;
			if (
				(lessThanNotEqualEpsilon(polyYi, curY, RASTER_EPS) && greaterThanEqualEpsilon(polyYj, curY, RASTER_EPS)) ||
				(lessThanNotEqualEpsilon(polyYj, curY, RASTER_EPS) && greaterThanEqualEpsilon(polyYi, curY, RASTER_EPS))
				)
				nodeX.push_back(polyXi + (pixelY - polyYi) / (polyYj - polyYi)*(polyXj - polyXi));
			j = i;
		}

		// Sort intersection points
		qSort(nodeX);

		//  Fill the pixels between node pairs.
		for (int i = 0; i<nodeX.size(); i += 2) {
			int line = curScanline;

			int initCoord, endCoord;
			initCoord = compareEpsilon(nodeX[i], qRound(nodeX[i]), RASTER_EPS) ? qRound(nodeX[i]) + 1 : qCeil(nodeX[i]);
			endCoord = compareEpsilon(nodeX[i + 1], qRound(nodeX[i + 1]), RASTER_EPS) ? qRound(nodeX[i + 1]) - 1 : qFloor(nodeX[i + 1]);
			line += initCoord;
			for (j = initCoord; j <= endCoord; j++, line += 1)
				S[line] = 0;
		}

		curScanline += width;
	}

	// FIXME: Delete horizontal lines
	QPolygonF::const_iterator itj;
	for (QPolygonF::const_iterator iti = polygon.cbegin(); iti != polygon.cend(); iti++) {
		if (iti + 1 == polygon.cend()) itj = polygon.cbegin();
		else itj = iti + 1;
		if (abs((*iti).y() - (*itj).y()) < RASTER_EPS && abs((*iti).y() - std::round((*iti).y())) < RASTER_EPS) {
			qreal left = (*iti).x() < (*itj).x() ? (*iti).x() : (*itj).x();
			qreal right = (*iti).x() < (*itj).x() ? (*itj).x() : (*iti).x();
			int initCoord, endCoord;
			initCoord = compareEpsilon(left, qRound(left), RASTER_EPS) ? qRound(left) + 1 : qCeil(left);
			endCoord = compareEpsilon(right, qRound(right), RASTER_EPS) ? qRound(right) - 1 : qFloor(right);
			int line = (int)(*iti).y()*width + initCoord;
			for (int i = initCoord; i <= endCoord; i++, line += 1) {
				S[line] = 1;
			}
		}
	}

	// FIXME: Use scan line to detect degenerated edges and nodes
	for (QVector<QPair<QPointF, QPointF>>::const_iterator it = this->degLines.cbegin(); it != this->degLines.cend(); it++) {
		QPointF p1 = scale*(*it).first; p1 -= QPointF(xMin, yMin);
		QPointF p2 = scale*(*it).second; p2 -= QPointF(xMin, yMin);
		BresenhamVec(qRound(p1.x()), qRound(p1.y()), qRound(p2.x()), qRound(p2.y()), S, width);
	}
	for (QVector<QPointF>::const_iterator it = this->degNodes.cbegin(); it != this->degNodes.cend(); it++) {
		QPointF p1 = scale*(*it); p1 -= QPointF(xMin, yMin);
		S[(int)p1.y()*width + (int)p1.x()] = 1;
	}

	return S;
}

int *Polygon::getRasterBoundingBoxImageVector(QPoint &RP, qreal scale, qreal epsilon, int &width, int &height) {
	qreal xMin, xMax, yMin, yMax;

	const_iterator it = cbegin();
	xMin = (*it).x(); xMax = (*it).x();
	yMin = (*it).y(); yMax = (*it).y();
	for (int i = 0; i < size(); i++) {
		qreal x = at(i).x();
		qreal y = at(i).y();

		if (x < xMin) xMin = x;
		if (x > xMax) xMax = x;
		if (y < yMin) yMin = y;
		if (y > yMax) yMax = y;
	}
	xMin = scale*xMin; xMax = scale*xMax;
	yMin = scale*yMin; yMax = scale*yMax;
	int xMinInt, yMinInt, xMaxInt, yMaxInt;
	if (abs(xMin - qRound(xMin)) < epsilon) xMinInt = qRound(xMin);
	else xMinInt = qCeil(xMin);
	if (abs(yMin - qRound(yMin)) < epsilon) yMinInt = qRound(yMin);
	else yMinInt = qCeil(yMin);
	//polygon.translate(-xMin, -yMin);
	if (abs(xMax - qRound(xMax)) < epsilon) xMaxInt = qRound(xMax);
	else xMaxInt = qFloor(xMax);
	if (abs(yMax - qRound(yMax)) < epsilon) yMaxInt = qRound(yMax);
	else yMaxInt = qFloor(yMax);

	RP.setX(-xMinInt); RP.setY(-yMinInt);
	width = xMaxInt - xMinInt + 1;
	height = yMaxInt - yMinInt + 1;
	int *S = new int[width*height];
	std::fill_n(S, width*height, 0);

	return S;
}

QImage Polygon::getRasterImage8bit(QPoint &RP, qreal scale) {
	int width, heigth;
	QPolygonF polygon;
	qreal xMin, xMax, yMin, yMax;

	const_iterator it = cbegin();
	xMin = scale*(*it).x(); xMax = scale*(*it).x();
	yMin = scale*(*it).y(); yMax = scale*(*it).y();
	for (int i = 0; i < size(); i++) {
		qreal x = scale*at(i).x();
		qreal y = scale*at(i).y();

		if (x < xMin) xMin = x;
		if (x > xMax) xMax = x;
		if (y < yMin) yMin = y;
		if (y > yMax) yMax = y;

		polygon << QPointF(x, y);
	}
	RP.setX(-qRound(xMin)); RP.setY(-qRound(yMin));
	polygon.translate(-xMin, -yMin);

	width = qRound(xMax - xMin) + 1;
	heigth = qRound(yMax - yMin) + 1;
	QImage image(width, heigth, QImage::Format_Indexed8);
	image.setColor(0, qRgb(255, 255, 255));
	image.setColor(1, qRgb(255, 0, 0));
	image.fill(0);


	for (int pixelY = 1; pixelY < image.height() - 1; pixelY++) {
		//  Build a list of nodes.
		QVector<int> nodeX;
		int j = polygon.size() - 1;
		for (int i = 0; i < polygon.size(); i++) {
			qreal polyYi = polygon[i].y();
			qreal polyXi = polygon[i].x();
			qreal polyYj = polygon[j].y();
			qreal polyXj = polygon[j].x();

			if ((polyYi<(double)pixelY && polyYj >= (double)pixelY) || (polyYj<(double)pixelY && polyYi >= (double)pixelY)) {
				nodeX.push_back((int)(polyXi + (pixelY - polyYi) / (polyYj - polyYi)*(polyXj - polyXi)));
			}
			j = i;
		}

		// Sort
		qSort(nodeX);


		//  Fill the pixels between node pairs.
		for (int i = 0; i<nodeX.size(); i += 2) {
			uchar *line = (uchar *)image.scanLine(pixelY);
			line += nodeX[i] + 1;
			for (j = nodeX[i] + 1; j < nodeX[i + 1]; j++, line += 1)
				*line = 1;
		}

		//        for(int i=0; i<nodeX.size(); i+=2) {

		//            // FIXME: Special Case verification necessary?
		//            if(nodeX[i]+1 > nodeX[i+1]-1) continue;

		//            uchar *line = (uchar *)image.scanLine(pixelY);
		//            int curLineIndex = 0;

		//            int initialCurLineIndex = (nodeX[i]+1) / 8;
		//            int initialCurLineOffset = (nodeX[i]+1) % 8;
		//            int finalCurLineIndex = (nodeX[i+1]-1) / 8;
		//            int finalCurLineOffset = (nodeX[i+1]-1) % 8;

		//            line += (curLineIndex = initialCurLineIndex);
		//            if(initialCurLineIndex != finalCurLineIndex) *line = *line | 255 >> initialCurLineOffset;
		//            else {
		//                *line = *line | ( (uchar)(255 >> initialCurLineOffset) & (uchar)(255 << (7-finalCurLineOffset)) );
		//                continue;
		//            }

		//            curLineIndex++;
		//            line++;
		//            while(curLineIndex < finalCurLineIndex) {
		//                *line = 255;
		//                curLineIndex++;
		//                line++;
		//            }

		//            *line = (uchar)(255 << (7-finalCurLineOffset));
		//        }
	}


	// FIXME: Delete horizontal lines
	QPolygonF::const_iterator itj;
	for (QPolygonF::const_iterator iti = polygon.cbegin(); iti != polygon.cend(); iti++) {
		if (iti + 1 == polygon.cend()) itj = polygon.cbegin();
		else itj = iti + 1;
		if ((int)(*iti).y() == (int)(*itj).y()) {
			int left = (*iti).x() < (*itj).x() ? (*iti).x() : (*itj).x();
			int right = (*iti).x() < (*itj).x() ? (*itj).x() : (*iti).x();
			for (int i = left; i <= right; i++) {
				image.setPixel(i, (int)(*iti).y(), 0);
			}
		}
	}

	// FIXME: Test
	for (QVector<QPair<QPointF, QPointF>>::const_iterator it = this->degLines.cbegin(); it != this->degLines.cend(); it++) {
		QPointF p1 = scale*(*it).first; p1 -= QPointF(xMin, yMin);
		QPointF p2 = scale*(*it).second; p2 -= QPointF(xMin, yMin);
		Bresenham(qRound(p1.x()), qRound(p1.y()), qRound(p2.x()), qRound(p2.y()), image);
	}
	for (QVector<QPointF>::const_iterator it = this->degNodes.cbegin(); it != this->degNodes.cend(); it++) {
		QPointF p1 = scale*(*it); p1 -= QPointF(xMin, yMin);
		image.setPixel(p1.x(), p1.y(), 0);
	}

	return image;
}

QImage Polygon::getRasterImage(QPoint &RP, qreal scale) {
	int width, heigth;
	QPolygonF polygon;
	qreal xMin, xMax, yMin, yMax;

	const_iterator it = cbegin();
	xMin = scale*(*it).x(); xMax = scale*(*it).x();
	yMin = scale*(*it).y(); yMax = scale*(*it).y();
	for (int i = 0; i < size(); i++) {
		qreal x = scale*at(i).x();
		qreal y = scale*at(i).y();

		if (x < xMin) xMin = x;
		if (x > xMax) xMax = x;
		if (y < yMin) yMin = y;
		if (y > yMax) yMax = y;

		polygon << QPointF(x, y);
	}
	RP.setX(-qRound(xMin)); RP.setY(-qRound(yMin));
	polygon.translate(-xMin, -yMin);

	width = qRound(xMax - xMin) + 1;
	heigth = qRound(yMax - yMin) + 1;
	QImage image(width, heigth, QImage::Format_Mono);
	image.setColor(0, qRgb(255, 255, 255));
	image.setColor(1, qRgb(255, 0, 0));
	image.fill(0);


	for (int pixelY = 1; pixelY < image.height() - 1; pixelY++) {
		//  Build a list of nodes.
		QVector<int> nodeX;
		int j = polygon.size() - 1;
		for (int i = 0; i < polygon.size(); i++) {
			qreal polyYi = polygon[i].y();
			qreal polyXi = polygon[i].x();
			qreal polyYj = polygon[j].y();
			qreal polyXj = polygon[j].x();

			if ((polyYi<(double)pixelY && polyYj >= (double)pixelY) || (polyYj<(double)pixelY && polyYi >= (double)pixelY)) {
				nodeX.push_back((int)(polyXi + (pixelY - polyYi) / (polyYj - polyYi)*(polyXj - polyXi)));
			}
			j = i;
		}

		// Sort
		qSort(nodeX);

		for (int i = 0; i<nodeX.size(); i += 2) {

			// FIXME: Special Case verification necessary?
			if (nodeX[i] + 1 > nodeX[i + 1] - 1) continue;

			uchar *line = (uchar *)image.scanLine(pixelY);
			int curLineIndex = 0;

			int initialCurLineIndex = (nodeX[i] + 1) / 8;
			int initialCurLineOffset = (nodeX[i] + 1) % 8;
			int finalCurLineIndex = (nodeX[i + 1] - 1) / 8;
			int finalCurLineOffset = (nodeX[i + 1] - 1) % 8;

			line += (curLineIndex = initialCurLineIndex);
			if (initialCurLineIndex != finalCurLineIndex) *line = *line | 255 >> initialCurLineOffset;
			else {
				*line = *line | ((uchar)(255 >> initialCurLineOffset) & (uchar)(255 << (7 - finalCurLineOffset)));
				continue;
			}

			curLineIndex++;
			line++;
			while (curLineIndex < finalCurLineIndex) {
				*line = 255;
				curLineIndex++;
				line++;
			}

			*line = (uchar)(255 << (7 - finalCurLineOffset));
		}
	}


	// FIXME: Delete horizontal lines
	QPolygonF::const_iterator itj;
	for (QPolygonF::const_iterator iti = polygon.cbegin(); iti != polygon.cend(); iti++) {
		if (iti + 1 == polygon.cend()) itj = polygon.cbegin();
		else itj = iti + 1;
		if ((int)(*iti).y() == (int)(*itj).y()) {
			int left = (*iti).x() < (*itj).x() ? (*iti).x() : (*itj).x();
			int right = (*iti).x() < (*itj).x() ? (*itj).x() : (*iti).x();
			for (int i = left; i <= right; i++) {
				image.setPixel(i, (int)(*iti).y(), 0);
			}
		}
	}

	// FIXME: Test
	for (QVector<QPair<QPointF, QPointF>>::const_iterator it = this->degLines.cbegin(); it != this->degLines.cend(); it++) {
		QPointF p1 = scale*(*it).first; p1 -= QPointF(xMin, yMin);
		QPointF p2 = scale*(*it).second; p2 -= QPointF(xMin, yMin);
		Bresenham(qRound(p1.x()), qRound(p1.y()), qRound(p2.x()), qRound(p2.y()), image);
	}
	for (QVector<QPointF>::const_iterator it = this->degNodes.cbegin(); it != this->degNodes.cend(); it++) {
		QPointF p1 = scale*(*it); p1 -= QPointF(xMin, yMin);
		image.setPixel(p1.x(), p1.y(), 0);
	}

	return image;
}

bool PackingProblem::load(QString fileName, QString fileType, qreal scale, qreal auxScale) {
	folder = QFileInfo(fileName).absolutePath();
	if (fileType != "esicup" && fileType != "cfrefp") {
		QFileInfo info(fileName);
		if (info.suffix() == "xml") fileType = "esicup";
		else if (info.suffix() == "txt") fileType = "cfrefp";
	}
	if (fileType == "esicup")
		return load(fileName);
	if (fileType == "cfrefp")
		return loadCFREFP(fileName, scale, auxScale);
	return false;
}

bool checkIfSingleContour(POLYBOOLEAN::PAREA *area, QString name) {
	if (area->f != area) {
		qWarning() << "Warning:" << name << "contains multiple contours! Ignoring extra areas.";
		return false;
	}
	//    if(area->NFP_lines != NULL) {
	//        qWarning() << "Warning:" << name << "contains degenerated line(s)! Ignoring extra areas.";
	//        return false;
	//    }
	//    if(area->NFP_nodes != NULL) {
	//        qWarning() << "Warning:" << name << "contains degenerated node(s)! Ignoring extra areas.";
	//        return false;
	//    }
	POLYBOOLEAN::PLINE2 *pline = area->cntr;
	if (pline != NULL && pline->next != NULL) {
		qWarning() << "Warning:" << name << "contains hole(s)! Ignoring extra areas.";
		return false;
	}
	return true;
}

POLYBOOLEAN::PAREA *getConcaveSingleContour(POLYBOOLEAN::PAREA *area) {
	if (area->f != area) {
		POLYBOOLEAN::PAREA * R = NULL;
		POLYBOOLEAN::PLINE2 *polyNode;
		POLYBOOLEAN::PAREA *areaNode = area;
		do {
			POLYBOOLEAN::PAREA * B = NULL;
			polyNode = areaNode->cntr;
			POLYBOOLEAN::PAREA::InclPline(&B, polyNode);
			POLYBOOLEAN::PAREA::Boolean(R, B, &R, POLYBOOLEAN::PAREA::UN);
			delete(B);
			areaNode = areaNode->f;
		} while (areaNode != area);
		R->NFP_lines = NULL; R->NFP_nodes = NULL; // FIXME: Delete
		return R;
	}
	return area;
}

void Polygon::fromPolybool(POLYBOOLEAN::PAREA *area, qreal scale) {
	checkIfSingleContour(area, name);

	if (area->cntr != NULL) {
		POLYBOOLEAN::VNODE2 *vn = area->cntr->head;
		do {
			this->push_back(QPointF((qreal)vn->g.x / scale, (qreal)vn->g.y / scale));
		} while ((vn = vn->next) != area->cntr->head);
	}

	// FIXME: Test
	for (POLYBOOLEAN::PLINE2 *NFPpline = area->NFP_lines; NFPpline != NULL; NFPpline = NFPpline->next) {
		POLYBOOLEAN::VNODE2 *vn1, *vn2;
		vn1 = NFPpline->head; vn2 = vn1->next;
		QPair<QPointF, QPointF> curLine;
		curLine.first = QPoint((qreal)vn1->g.x / scale, (qreal)vn1->g.y / scale);
		curLine.second = QPoint((qreal)vn2->g.x / scale, (qreal)vn2->g.y / scale);
		this->degLines.push_back(curLine);
	}
	for (POLYBOOLEAN::PLINE2 *NFPnode = area->NFP_nodes; NFPnode != NULL; NFPnode = NFPnode->next) {
		POLYBOOLEAN::VNODE2 *vn1 = NFPnode->head;
		this->degNodes.push_back(QPoint((qreal)vn1->g.x / scale, (qreal)vn1->g.y / scale));
	}
}

bool PackingProblem::loadCFREFP(QString &fileName, qreal scale, qreal auxScale) {
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qCritical() << "Puzzle file not found";
		return false;
	}
	QTextStream f(&file);
	bool ans = loadCFREFP(f, scale, auxScale);
	file.close();
	return ans;
}

bool PackingProblem::loadCFREFP(QTextStream &stream, qreal scale, qreal auxScale) {
	std::vector<std::shared_ptr<cShape> > shapes;
	std::shared_ptr<cRectangularContainer> container;

	container = readProblemInstance(stream, shapes, scale);

	unsigned int polygonid = 0;
	QString containerName;
	QMap<int, QString> pieceNames;

	// Add container
	{
		std::shared_ptr<Container> curContainer(new Container("board0", 1));
		containerName = "polygon" + QString::number(polygonid); polygonid++;
		std::shared_ptr<Polygon> curPolygon(new Polygon(containerName));
		curPolygon->fromPolybool(container->getShape()->getInnerData(), scale*auxScale);
		curContainer->setPolygon(curPolygon);
		this->containers.push_back(curContainer);
	}

	// Add pieces
	for (std::vector<std::shared_ptr<cShape>>::const_iterator it = shapes.cbegin(); it != shapes.cend(); it++) {
		std::shared_ptr<cShape> curShape = *it;
		std::shared_ptr<Piece> curPiece(new Piece("piece" + QString::number(curShape->getId()), curShape->getMultiplicity()));
		for (unsigned int i = 0; i < curShape->getAnglesCount(); i++){
			curPiece->addOrientation(curShape->getAngle(i));
		}
		curShape->getPlacement()->updatePlacement(cVector(0, 0));
		curShape->setRot(0); curShape->setdRot(0);
		QString pieceName = "polygon" + QString::number(polygonid); polygonid++;
		pieceNames.insert(curShape->getId(), pieceName);
		std::shared_ptr<Polygon> curPolygon(new Polygon(pieceName));
		curPolygon->fromPolybool(getConcaveSingleContour(curShape->getTranslatedPolygon()->getInnerData()), scale*auxScale);
		curPiece->setPolygon(curPolygon);
		this->pieces.push_back(curPiece);
	}

	// Add nofit polygons
	polygonid = 0;
	for (std::vector<std::shared_ptr<cShape>>::const_iterator sit = shapes.cbegin(); sit != shapes.cend(); sit++) {
		int staticAnglesCount = (*sit)->getAnglesCount();
		int staticId = (*sit)->getId();
		bool noStaticAngles = false;
		if (staticAnglesCount == 0) { staticAnglesCount = 1; noStaticAngles = true; }
		for (int i = 0; i < staticAnglesCount; i++){
			int sangle = noStaticAngles ? 0 : (*sit)->getAngle(i);
			for (std::vector<std::shared_ptr<cShape>>::const_iterator oit = shapes.cbegin(); oit != shapes.cend(); oit++) {
				int orbitingAnglesCount = (*oit)->getAnglesCount();
				int orbitingId = (*oit)->getId();
				bool noOrbitingAngles = false;
				if (orbitingAnglesCount == 0) { orbitingAnglesCount = 1; noOrbitingAngles = true; }
				for (int j = 0; j < orbitingAnglesCount; j++){
					int oangle = noOrbitingAngles ? 0 : (*oit)->getAngle(j);
					// -> Create Descriptor
					std::shared_ptr<NoFitPolygon> curNFP(new NoFitPolygon);
					curNFP->setStaticName(pieceNames[staticId]);
					curNFP->setOrbitingName(pieceNames[orbitingId]);
					curNFP->setStaticAngle(sangle);
					curNFP->setOrbitingAngle(oangle);
					QString polygonName = "nfpPolygon" + QString::number(polygonid); polygonid++;
					curNFP->setName(polygonName);

					// --> Get Nofit Polygon
					std::shared_ptr<cShape> staticShape = *sit;
					std::shared_ptr<cShape> orbitingShape = *oit;
					staticShape->getPlacement()->updatePlacement(cVector(0, 0));
					staticShape->setRot(sangle); staticShape->setdRot(sangle);
					std::shared_ptr<cPolygon> staticPolygon = staticShape->getTranslatedPolygon();
					orbitingShape->getPlacement()->updatePlacement(cVector(0, 0));
					orbitingShape->setRot(oangle); orbitingShape->setdRot(oangle);
					std::shared_ptr<cPolygon> orbitingPolygon = orbitingShape->getTranslatedPolygon();
					std::shared_ptr<cPolygon> nfp = orbitingPolygon->getNoFitPolygon(staticPolygon);

					std::shared_ptr<Polygon> curPolygon(new Polygon(polygonName));
					curPolygon->fromPolybool(nfp->getInnerData(), scale*auxScale);
					curNFP->setPolygon(curPolygon);
					this->nofitPolygons.push_back(curNFP);
				}

			}
		}
	}

	// Add innerfit polygons
	polygonid = 0;
	for (std::vector<std::shared_ptr<cShape>>::const_iterator oit = shapes.cbegin(); oit != shapes.cend(); oit++) {
		int orbitingAnglesCount = (*oit)->getAnglesCount();
		int orbitingId = (*oit)->getId();
		bool noOrbitingAngles = false;
		if (orbitingAnglesCount == 0) { orbitingAnglesCount = 1; noOrbitingAngles = true; }
		for (int j = 0; j < orbitingAnglesCount; j++){
			int oangle = noOrbitingAngles ? 0 : (*oit)->getAngle(j);
			// -> Create Descriptor
			std::shared_ptr<InnerFitPolygon> curIFP(new InnerFitPolygon);
			curIFP->setStaticName(containerName);
			curIFP->setOrbitingName(pieceNames[orbitingId]);
			curIFP->setStaticAngle(0);
			curIFP->setOrbitingAngle(oangle);
			QString polygonName = "ifpPolygon" + QString::number(polygonid); polygonid++;
			curIFP->setName(polygonName);

			// --> Get Inner Polygon
			std::shared_ptr<cShape> orbitingShape = *oit;
			orbitingShape->getPlacement()->updatePlacement(cVector(0, 0));
			orbitingShape->setRot(oangle); orbitingShape->setdRot(oangle);
			std::shared_ptr<cPolygon> orbitingPolygon = orbitingShape->getTranslatedPolygon();
			std::shared_ptr<cPolygon> ifp = container->getInnerFitPolygon(orbitingPolygon);

			std::shared_ptr<Polygon> curPolygon(new Polygon(polygonName));
			curPolygon->fromPolybool(ifp->getInnerData(), scale*auxScale);
			curIFP->setPolygon(curPolygon);
			this->innerfitPolygons.push_back(curIFP);
		}
	}

	return true;
}