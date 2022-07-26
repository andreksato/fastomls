#include "rasteroverlapevaluator.h"

using namespace RASTERVORONOIPACKING;

std::shared_ptr<TotalOverlapMap> RasterTotalOverlapMapEvaluatorGLS::getTotalOverlapMap(int itemId, int orientation, RasterPackingSolution &solution) {
	std::shared_ptr<TotalOverlapMap> currrentPieceMap = maps.getOverlapMap(itemId, orientation);
	currrentPieceMap->reset();
	for (int i = 0; i < problem->count(); i++) {
		if (i == itemId) continue;
		currrentPieceMap->addVoronoi(problem->getNfps()->getRasterNoFitPolygon(problem->getItemType(i), solution.getOrientation(i), problem->getItemType(itemId), orientation), solution.getPosition(i), glsWeights->getWeight(itemId, i));
	}
	return currrentPieceMap;
}

void RasterTotalOverlapMapEvaluatorGLS::updateWeights(RasterPackingSolution &solution) {
	QVector<WeightIncrement> solutionOverlapValues;
	qreal maxOValue = 0;

	// Determine pair overlap values
	for (int itemId1 = 0; itemId1 < problem->count(); itemId1++)
	for (int itemId2 = 0; itemId2 < problem->count(); itemId2++) {
		if (itemId1 == itemId2) continue;
		qreal curOValue = problem->getDistanceValue(itemId1, solution.getPosition(itemId1), solution.getOrientation(itemId1),
			itemId2, solution.getPosition(itemId2), solution.getOrientation(itemId2));
		if (curOValue != 0) {
			solutionOverlapValues.append(WeightIncrement(itemId1, itemId2, (qreal)curOValue));
			if (curOValue > maxOValue) maxOValue = curOValue;
		}
	}

	// Divide vector by maximum
	std::for_each(solutionOverlapValues.begin(), solutionOverlapValues.end(), [&maxOValue](WeightIncrement &curW){curW.value = curW.value / maxOValue; });

	// Add to the current weight map
	glsWeights->updateWeights(solutionOverlapValues);
}

//  TODO: Update cache information!
void RasterTotalOverlapMapEvaluatorGLS::resetWeights() {
	glsWeights->reset(problem->count());
}