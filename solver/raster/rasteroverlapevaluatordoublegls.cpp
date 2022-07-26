#include "rasteroverlapevaluator.h"
#include <QtCore/qmath.h>

using namespace RASTERVORONOIPACKING;

void getScaledSolution(RasterPackingSolution &originalSolution, RasterPackingSolution &newSolution, qreal scaleFactor) {
	newSolution = RasterPackingSolution(originalSolution.getNumItems());
	for (int i = 0; i < originalSolution.getNumItems(); i++) {
		newSolution.setOrientation(i, originalSolution.getOrientation(i));
		QPoint finePos = QPoint(qRound((qreal)originalSolution.getPosition(i).x() * scaleFactor), qRound((qreal)originalSolution.getPosition(i).y() * scaleFactor));
		newSolution.setPosition(i, finePos);
	}
}

void RasterTotalOverlapMapEvaluatorDoubleGLS::createSearchMaps() {
	int zoomFactorInt = this->problem->getScale() / searchProblemScale;
	for (int itemId = 0; itemId < problem->count(); itemId++) {
		for (uint angle = 0; angle < problem->getItem(itemId)->getAngleCount(); angle++) {
			std::shared_ptr<RasterNoFitPolygon> curIfp = problem->getIfps()->getRasterNoFitPolygon(-1, -1, problem->getItemType(itemId), angle);
			int newWidth = 1 + (curIfp->width() - 1) / zoomFactorInt; int newHeight = 1 + (curIfp->height() - 1) / zoomFactorInt;
			QPoint newReferencePoint = QPoint(curIfp->getOrigin().x() / zoomFactorInt, curIfp->getOrigin().y() / zoomFactorInt);
			maps.addOverlapMap(itemId, angle, std::shared_ptr<TotalOverlapMap>(new TotalOverlapMap(newWidth, newHeight, newReferencePoint)));
			// FIXME: Delete innerift polygons as they are used to release memomry
		}
	}
}

std::shared_ptr<TotalOverlapMap> RasterTotalOverlapMapEvaluatorDoubleGLS::getTotalOverlapMap(int itemId, int orientation, RasterPackingSolution &solution) {
	QPoint pos = getMinimumOverlapSearchPosition(itemId, orientation, solution);
	int zoomSquareSize = ZOOMNEIGHBORHOOD*qRound(this->problem->getScale() / searchProblemScale);
	return getRectTotalOverlapMap(itemId, orientation, pos, zoomSquareSize, zoomSquareSize, solution);
}

// --> Get absolute minimum overlap position
QPoint RasterTotalOverlapMapEvaluatorDoubleGLS::getMinimumOverlapSearchPosition(int itemId, int orientation, RasterPackingSolution &solution) {
	std::shared_ptr<TotalOverlapMap> map = getTotalOverlapSearchMap(itemId, orientation, solution);
	QPoint minRelativePos;
	map->getMinimum(minRelativePos);

	// Rescale position before returning
	return (this->problem->getScale() / searchProblemScale) *(minRelativePos - map->getReferencePoint());
}

std::shared_ptr<TotalOverlapMap> RasterTotalOverlapMapEvaluatorDoubleGLS::getTotalOverlapSearchMap(int itemId, int orientation, RasterPackingSolution &solution) {
	int zoomFactorInt = this->problem->getScale() / searchProblemScale;
	std::shared_ptr<TotalOverlapMap> currrentPieceMap = maps.getOverlapMap(itemId, orientation);
	currrentPieceMap->reset();
	for (int i = 0; i < problem->count(); i++) {
		if (i == itemId) continue;
		currrentPieceMap->addVoronoi(problem->getNfps()->getRasterNoFitPolygon(problem->getItemType(i), solution.getOrientation(i), problem->getItemType(itemId), orientation), solution.getPosition(i), glsWeights->getWeight(itemId, i), zoomFactorInt);
	}
	return currrentPieceMap;
}

std::shared_ptr<TotalOverlapMap> RasterTotalOverlapMapEvaluatorDoubleGLS::getRectTotalOverlapMap(int itemId, int orientation, QPoint pos, int width, int height, RasterPackingSolution &solution) {
	// Determine zoomed area inside the innerfit polygon
	std::shared_ptr<RasterNoFitPolygon> curIfp = this->problem->getIfps()->getRasterNoFitPolygon(-1, -1, this->problem->getItemType(itemId), orientation);
	QRect curIfpBoundingBox(QPoint(-curIfp->getOriginX(), -curIfp->getOriginY()), QSize(curIfp->width() - maps.getShrinkValX(), curIfp->height() - maps.getShrinkValY()));
	QRect zoomSquareRect(QPoint(pos.x() - width / 2, pos.y() - height / 2), QSize(width, height));
	zoomSquareRect = zoomSquareRect.intersected(curIfpBoundingBox);

	// Create zoomed overlap Map
	std::shared_ptr<TotalOverlapMap> curZoomedMap = std::shared_ptr<TotalOverlapMap>(new TotalOverlapMap(zoomSquareRect));
	for (int j = zoomSquareRect.top(); j <= zoomSquareRect.bottom(); j++)
	for (int i = zoomSquareRect.left(); i <= zoomSquareRect.right(); i++)
		curZoomedMap->setValue(QPoint(i, j), getTotalOverlapMapSingleValue(itemId, orientation, QPoint(i, j), solution));

	return curZoomedMap;
}

int getRoughShrinkage(int deltaPixels, qreal scaleRatio) {
	qreal fracDeltaPixelsX = scaleRatio*(qreal)(deltaPixels);
	if (qFuzzyCompare(1.0 + fracDeltaPixelsX, 1.0 + qRound(fracDeltaPixelsX))) return qRound(fracDeltaPixelsX);
	return qRound(fracDeltaPixelsX);
}

void RasterTotalOverlapMapEvaluatorDoubleGLS::updateMapsLength(int pixelWidth) {
	int deltaPixels = problem->getContainerWidth() - pixelWidth;
	maps.setShrinkVal(deltaPixels);
	int deltaPixelsRough = getRoughShrinkage(deltaPixels, searchProblemScale / this->problem->getScale());

	for (int itemId = 0; itemId < problem->count(); itemId++)
	for (uint angle = 0; angle < problem->getItem(itemId)->getAngleCount(); angle++) {
		std::shared_ptr<TotalOverlapMap> curMap = maps.getOverlapMap(itemId, angle);
		curMap->setRelativeWidth(deltaPixelsRough);
	}
}

void RasterTotalOverlapMapEvaluatorDoubleGLS::updateMapsDimensions(int pixelWidth, int pixelHeight) {
	int deltaPixelsX = problem->getContainerWidth() - pixelWidth;
	int deltaPixelsY = problem->getContainerHeight() - pixelHeight;
	maps.setShrinkVal(deltaPixelsX, deltaPixelsY);
	int deltaPixelsRoughX = getRoughShrinkage(deltaPixelsX, searchProblemScale / this->problem->getScale());
	int deltaPixelsRoughY = getRoughShrinkage(deltaPixelsY, searchProblemScale / this->problem->getScale());

	for (int itemId = 0; itemId < problem->count(); itemId++)
	for (uint angle = 0; angle < problem->getItem(itemId)->getAngleCount(); angle++) {
		std::shared_ptr<TotalOverlapMap> curMap = maps.getOverlapMap(itemId, angle);
		curMap->setRelativeDimensions(deltaPixelsRoughX, deltaPixelsRoughY);
	}
}

qreal RasterTotalOverlapMapEvaluatorDoubleGLS::getTotalOverlapMapSingleValue(int itemId, int orientation, QPoint pos, RasterPackingSolution &solution) {
	qreal totalOverlap = 0;
	for (int i = 0; i < problem->count(); i++) {
		if (i == itemId) continue;
		totalOverlap += glsWeights->getWeight(itemId, i) * problem->getDistanceValue(itemId, pos, orientation, i, solution.getPosition(i), solution.getOrientation(i));
	}
	return totalOverlap;
}