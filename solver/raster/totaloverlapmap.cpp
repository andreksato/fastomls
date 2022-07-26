#include "totaloverlapmap.h"

#ifndef CONSOLE
	#include "colormap.h"
#endif

using namespace RASTERVORONOIPACKING;

TotalOverlapMapSet::TotalOverlapMapSet() : shrinkValX(0), shrinkValY(0) {
    numAngles = 4;
}

TotalOverlapMapSet::TotalOverlapMapSet(int numberOfOrientations) : shrinkValX(0), shrinkValY(0), numAngles(numberOfOrientations) {}

void TotalOverlapMapSet::addOverlapMap(int orbitingPieceId, int orbitingAngleId, std::shared_ptr<TotalOverlapMap> ovm) {
    int orbitingKey =  orbitingPieceId*numAngles + orbitingAngleId;
    mapSet.insert(orbitingKey, ovm);
}

std::shared_ptr<TotalOverlapMap> TotalOverlapMapSet::getOverlapMap(int orbitingPieceId, int orbitingAngleId) {
    int orbitingKey =  orbitingPieceId*numAngles + orbitingAngleId;
    return mapSet[orbitingKey];
}

TotalOverlapMap::TotalOverlapMap(std::shared_ptr<RasterNoFitPolygon> ifp) : originalWidth(ifp->width()), originalHeight(ifp->height()) {
    init(ifp->width(), ifp->height());
    reference = ifp->getOrigin();
}

TotalOverlapMap::TotalOverlapMap(int width, int height) : originalWidth(width), originalHeight(height) {
    init(width, height);
}

TotalOverlapMap::TotalOverlapMap(int width, int height, QPoint _reference) : originalWidth(width), originalHeight(height), reference(_reference) {
	init(width, height);
}

TotalOverlapMap::TotalOverlapMap(QRect &boundingBox) : originalWidth(boundingBox.width()), originalHeight(boundingBox.height()) {
	init(boundingBox.width(), boundingBox.height());
	reference = -boundingBox.topLeft();
}

void TotalOverlapMap::init(uint _width, uint _height) {
    this->width = _width;
    this->height = _height;
    data = new float[width*height];
    Q_CHECK_PTR(data);
    std::fill(data, data+width*height, (float)0.0);
    #ifdef QT_DEBUG
        initialWidth = width;
    #endif
}

void TotalOverlapMap::reset() {
     std::fill(data, data+width*height, (float)0.0);
}

float *TotalOverlapMap::scanLine(int y) {
    return data+y*width;
}

bool TotalOverlapMap::getLimits(QPoint relativeOrigin, int vmWidth, int vmHeight, QRect &intersection) {
    intersection.setCoords(relativeOrigin.x() < 0 ? 0 : relativeOrigin.x(),
                           relativeOrigin.y()+vmHeight > getHeight() ? getHeight()-1 : relativeOrigin.y()+vmHeight-1,
                           relativeOrigin.x()+vmWidth > getWidth() ? getWidth()-1 : relativeOrigin.x()+vmWidth-1,
                           relativeOrigin.y() < 0 ? 0 : relativeOrigin.y());

    if(intersection.topRight().x() < 0 || intersection.topRight().y() < 0 ||
       intersection.bottomLeft().x() > getWidth() || intersection.bottomLeft().y() > getHeight()) return false;
    return true;
}

bool TotalOverlapMap::getLimits(QPoint relativeOrigin, int vmWidth, int vmHeight, QRect &intersection, int zoomFactorInt) {
	intersection.setCoords(relativeOrigin.x() < 0 ? 0 : relativeOrigin.x(),
		relativeOrigin.y() + vmHeight > zoomFactorInt*getHeight() ? zoomFactorInt*getHeight() - 1 : relativeOrigin.y() + vmHeight - 1,
		relativeOrigin.x() + vmWidth > zoomFactorInt*getWidth() ? zoomFactorInt*getWidth() - 1 : relativeOrigin.x() + vmWidth - 1,
		relativeOrigin.y() < 0 ? 0 : relativeOrigin.y());

	if (intersection.topRight().x() < 0 || intersection.topRight().y() < 0 ||
		intersection.bottomLeft().x() > zoomFactorInt*getWidth() || intersection.bottomLeft().y() > zoomFactorInt*getHeight()) return false;
	return true;
}

void TotalOverlapMap::addVoronoi(std::shared_ptr<RasterNoFitPolygon> nfp, QPoint pos) {
    QPoint relativeOrigin = getReferencePoint() + pos - nfp->getOrigin();
    QRect intersection;
    if(!getLimits(relativeOrigin, nfp->width(), nfp->height(), intersection)) return;

	for (int j = intersection.bottomLeft().y(); j <= intersection.topRight().y(); j++) {
		float *dataPt = scanLine(j) + intersection.bottomLeft().x();
		for (int i = intersection.bottomLeft().x(); i <= intersection.topRight().x(); i++, dataPt++) {
			int indexValue = nfp->getPixel(i - relativeOrigin.x(), j - relativeOrigin.y());
			float distanceValue = 0.0;
			*dataPt += indexValue;
		}
	}
}

void TotalOverlapMap::addVoronoi(std::shared_ptr<RasterNoFitPolygon> nfp, QPoint pos, float weight) {
    QPoint relativeOrigin = getReferencePoint() + pos - nfp->getOrigin();
    QRect intersection;
    if(!getLimits(relativeOrigin, nfp->width(), nfp->height(), intersection)) return;

	for (int j = intersection.bottomLeft().y(); j <= intersection.topRight().y(); j++) {
		float *dataPt = scanLine(j) + intersection.bottomLeft().x();
		for (int i = intersection.bottomLeft().x(); i <= intersection.topRight().x(); i++, dataPt++) {
			int indexValue = nfp->getPixel(i - relativeOrigin.x(), j - relativeOrigin.y());
			float distanceValue = 0.0;
			*dataPt += weight * (float)indexValue;
		}
	}
}

void TotalOverlapMap::addVoronoi(std::shared_ptr<RasterNoFitPolygon> nfp, QPoint pos, float weight, int zoomFactorInt) {
	//QPoint relativeOrigin = getReferencePoint() + pos - nfp->getOrigin();
	QPoint relativeOrigin = zoomFactorInt * getReferencePoint() + pos - nfp->getOrigin();
	QRect intersection;
	if (!getLimits(relativeOrigin, nfp->width(), nfp->height(), intersection, zoomFactorInt)) return;

	int initialY = intersection.bottomLeft().y() % zoomFactorInt == 0 ? intersection.bottomLeft().y() : zoomFactorInt * ((intersection.bottomLeft().y() / zoomFactorInt) + 1);
	//int initialY = intersection.bottomLeft().y();
	for (int j = initialY; j <= intersection.topRight().y(); j += zoomFactorInt) {
		int initialX = intersection.bottomLeft().x() % zoomFactorInt == 0 ? intersection.bottomLeft().x() : zoomFactorInt * ((intersection.bottomLeft().x() / zoomFactorInt) + 1);
		//int initialX = intersection.bottomLeft().x();
		float *dataPt = scanLine(j/5) + initialX/5;
		for (int i = initialX; i <= intersection.topRight().x(); i += zoomFactorInt, dataPt++) {
			int indexValue = nfp->getPixel(i - relativeOrigin.x(), j - relativeOrigin.y());
			float distanceValue = 0.0;
			*dataPt += weight * (float)indexValue;
		}
	}
}

float TotalOverlapMap::getMinimum(QPoint &minPt) {
	float *curPt = data;
	float minVal = *curPt;
	minPt = QPoint(0, 0);
	int numVals = height*width;
	for (int id = 0; id < numVals; id++, curPt++) {
		float curVal = *curPt;
		if (curVal < minVal) {
			minVal = curVal;
			minPt = QPoint(id % width, id / width);
			if (qFuzzyCompare(1.0 + minVal, 1.0 + 0.0)) return minVal;
		}
	}
	return minVal;
}

#ifndef CONSOLE
    QImage TotalOverlapMap::getImage() {
        float maxD = 0;
        for(int pixelY = 0; pixelY < height; pixelY++) {
            float *mapLine = scanLine(pixelY);
            for(int pixelX = 0; pixelX < width; pixelX++) {
                if(*mapLine > maxD) maxD = *mapLine;
                mapLine++;
            }
        }

        QImage image(width, height, QImage::Format_Indexed8);
        setColormap(image);
        for(int pixelY = 0; pixelY < height; pixelY++) {
            uchar *resultLine = (uchar *)image.scanLine(pixelY);
            float *mapLine = scanLine(pixelY);
            for(int pixelX = 0; pixelX < width; pixelX++) {
                if(*mapLine==0)
                    *resultLine=0;
                else {
                    int index = (int)((*mapLine-1)*254/(maxD-1) + 1);
                    *resultLine = index;
                }
                resultLine ++; mapLine++;
            }
        }
        return image;
    }

	QImage TotalOverlapMap::getZoomImage(int _width, int _height, QPoint &displacement) {
		float maxD = 0;
		for (int pixelY = 0; pixelY < height; pixelY++) {
			float *mapLine = scanLine(pixelY);
			for (int pixelX = 0; pixelX < width; pixelX++) {
				if (*mapLine > maxD) maxD = *mapLine;
				mapLine++;
			}
		}

		QImage image(_width, _height, QImage::Format_Indexed8);
		image.fill(255);
		setColormap(image);
		image.setColor(255, qRgba(0, 0, 0, 0));
		for (int pixelY = 0; pixelY < _height; pixelY++) {
			uchar *resultLine = (uchar *)image.scanLine(pixelY);
			int mapPixelY = pixelY - displacement.y(); if (mapPixelY < 0 || mapPixelY >= height) continue;
			float *mapLine = scanLine(mapPixelY);

			int pixelX = 0;
			int mapPixelX = pixelX - displacement.x();
			while (mapPixelX < 0) resultLine++, mapPixelX++, pixelX++;
			for (; pixelX < _width; pixelX++, resultLine++, mapLine++, mapPixelX++) {
				if (mapPixelX >= width) break;
				if (*mapLine == 0)
					*resultLine = 0;
				else {
					int index = (int)((*mapLine - 1) * 253 / (maxD - 1) + 1);
					*resultLine = index;
				}
			}
		}
		return image;
	}
#endif

//QImage TotalOverlapMap::getImage2() {
//    float maxD = 0;
//    float minD = data[0];
//    for(int pixelY = 0; pixelY < height; pixelY++) {
//        float *mapLine = scanLine(pixelY);
//        for(int pixelX = 0; pixelX < width; pixelX++) {
//            if(*mapLine > maxD) maxD = *mapLine;
//            if(!qFuzzyCompare(1.0 + 0.0, 1.0 + minD) && *mapLine < minD) minD = *mapLine;
//            mapLine++;
//        }
//    }

//    QImage image(width, height, QImage::Format_Indexed8);
//    setColormap(image);
//    for(int pixelY = 0; pixelY < height; pixelY++) {
//        uchar *resultLine = (uchar *)image.scanLine(pixelY);
//        float *mapLine = scanLine(pixelY);
//        for(int pixelX = 0; pixelX < width; pixelX++) {
//            if(qFuzzyCompare(1.0 + 0.0, 1.0 + *mapLine))
//                *resultLine=0;
//            else {
//                int index = (int)((*mapLine-minD)*254/(maxD-minD) + 1);
//                *resultLine = index;
//            }
//            resultLine ++; mapLine++;
//        }
//    }
//    return image;
//}
