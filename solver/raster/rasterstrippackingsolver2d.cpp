#include "rasterstrippackingsolver.h"

using namespace RASTERVORONOIPACKING;

// --> Generate initial solution using the bottom left heuristic and resize the container accordingly
void RasterStripPackingSolver::generateBottomLeftRectangleSolution(RasterPackingSolution &solution) {
	QVector<int> sequence;
	for (int i = 0; i < originalProblem->count(); i++) sequence.append(i);
	std::random_shuffle(sequence.begin(), sequence.end());
	int layoutLength = 0;
	int layoutHeight = 0;
	for (int k = 0; k < originalProblem->count(); k++) {
		int shuffledId = sequence[k];
		int minItemArea, curBestLayoutLength, curBestLayoutHeight;
		// Find left bottom placement for item
		for (unsigned int angle = 0; angle < originalProblem->getItem(shuffledId)->getAngleCount(); angle++) {
			//qDebug() << "Item" << k << "Angle" << angle;
			// Get IFP bounding box
			int  minIfpX, minIfpY, maxIfpX, maxIfpY;
			originalProblem->getIfpBoundingBox(shuffledId, angle, minIfpX, minIfpY, maxIfpX, maxIfpY);
			QPoint curPos(minIfpX, minIfpY);
			int i = 0;
			while (1) {
				int aux = 0;
				bool exit = false;
				for (int j = 0; j < i; j++){
					curPos = QPoint(minIfpX + i, minIfpY + aux);
					//qDebug() << curPos;
					if (!detectItemPartialOverlap(sequence, k, curPos, angle, solution)) { exit = true;  break; }
					curPos = QPoint(minIfpX + aux, minIfpY + i);
					//qDebug() << curPos;
					if (!detectItemPartialOverlap(sequence, k, curPos, angle, solution)) { exit = true;  break; }
					aux++;
				}
				if (exit) break;
				curPos = QPoint(minIfpX + i, minIfpY + i);
				//qDebug() << curPos;
				if (!detectItemPartialOverlap(sequence, k, curPos, angle, solution)) break;
				i++;
			}
			// Check minimum X and Y coordinate
			int maxItemY = curPos.y() + qRound(originalProblem->getScale()*originalProblem->getItem(shuffledId)->getMaxY(angle));
			int maxItemX = curPos.x() + qRound(originalProblem->getScale()*originalProblem->getItem(shuffledId)->getMaxX(angle));
			int curArea = qMax(maxItemX, layoutLength) * qMax(maxItemY, layoutHeight);
			if (angle == 0 || curArea < minItemArea) {
				minItemArea = curArea;
				solution.setPosition(shuffledId, curPos); solution.setOrientation(shuffledId, angle);
				curBestLayoutLength = qMax(maxItemX, layoutLength);
				curBestLayoutHeight = qMax(maxItemY, layoutHeight);
			}
		}
		layoutLength = curBestLayoutLength;
		layoutHeight = curBestLayoutHeight;
	}
	//setContainerWidth(layoutLength, solution, params);
	setContainerDimensions(layoutLength, layoutHeight, solution);
}

void RasterStripPackingSolver::generateBottomLeftSquareSolution(RasterPackingSolution &solution) {
	generateBottomLeftSolution(solution);
	int largestDim = currentWidth > currentHeight ? currentWidth : currentHeight;
	setContainerDimensions(largestDim, largestDim, solution);
}

bool RasterStripPackingSolver::setContainerDimensions(int &pixelWidthX, int &pixelWidthY, RasterPackingSolution &solution) {
	if (!setContainerDimensions(pixelWidthX, pixelWidthY)) return false;

	// Detect extruding items and move them horizontally back inside the container
	for (int itemId = 0; itemId < originalProblem->count(); itemId++) {
		std::shared_ptr<RasterNoFitPolygon> ifp = originalProblem->getIfps()->getRasterNoFitPolygon(-1, -1, originalProblem->getItemType(itemId), solution.getOrientation(itemId));
		int maxPositionX = -ifp->getOriginX() + ifp->width() - (this->initialWidth - this->currentWidth) - 1;
		int maxPositionY = -ifp->getOriginY() + ifp->height() - (this->initialHeight - this->currentHeight) - 1;
		QPoint curItemPos = solution.getPosition(itemId);
		if (curItemPos.x() > maxPositionX) curItemPos.setX(maxPositionX);
		if (curItemPos.y() > maxPositionY) curItemPos.setY(maxPositionY);
		solution.setPosition(itemId, curItemPos);
	}

	return true;
}

bool RasterStripPackingSolver::setContainerDimensions(int &pixelWidthX, int &pixelWidthY) {
	// Check if size is smaller than smallest item width
	if (pixelWidthX < this->getMinimumContainerWidth() || pixelWidthY < this->getMinimumContainerHeight()) {
		pixelWidthX = this->currentWidth; pixelWidthY = this->currentHeight;
		return false;
	}

	// Resize container
	overlapEvaluator->updateMapsDimensions(pixelWidthX, pixelWidthY);
	this->currentWidth = pixelWidthX; this->currentHeight = pixelWidthY;
	return true;
}