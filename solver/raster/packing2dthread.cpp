#include "packing2dthread.h"
#include <cmath>
#include <limits>
#include <QDebug>
#include <QTime>

#define MAXLOOPSPERLENGTH 5
#define UPDATEINTERVAL 0.5

void Packing2DThread::changeKeepAspectRatio(int &curLenght, int &curHeight, const qreal ratio) {
	qreal ratioSqr = sqrt(ratio);
	if (ratio > 1) {
		curLenght = std::ceil(ratioSqr * (qreal)curLenght);
		curHeight = std::ceil(ratioSqr * (qreal)curHeight);
		return;
	}
	int reducedLength, reducedHeight;
	if (!checkShrinkSizeConstraint(curLenght, curHeight, reducedLength, reducedHeight, ratioSqr)) return;
	curLenght = reducedLength;
	curHeight = reducedHeight;
}

void Packing2DThread::bagpipeChangeContainerDimensions(int &curLenght, int &curHeight) {
	qreal curArea = (qreal)curLenght * (qreal)curHeight;
	qreal ratioWmin = ((qreal)solver->getMinimumContainerWidth() * (qreal)solver->getMinimumContainerWidth()) / curArea;
	qreal ratioHmin = curArea / ((qreal)solver->getMinimumContainerHeight() * (qreal)solver->getMinimumContainerHeight());

	float ratio = ratioWmin + static_cast <float> (qrand()) / (static_cast <float> (RAND_MAX / (ratioHmin - ratioWmin)));

	curLenght = qRound(sqrt(curArea * ratio));
	curHeight = qRound(sqrt(curArea / ratio));
}

bool Packing2DThread::getShrinkedDimension(int dim, int &newDim, bool length) {
	// Get minimum dimension
	int minimumDimension = length ? solver->getMinimumContainerWidth() : solver->getMinimumContainerHeight();
	
	// Check if dimension is already minimum
	if (dim == minimumDimension) return false;
	
	// Get next dimension and check limit
	newDim = newDim < minimumDimension ? minimumDimension : newDim;
	return true;
}

void Packing2DThread::randomChangeContainerDimensions(int &curLenght, int &curHeight, const qreal ratio) {
	bool changeLenght = qrand() % 2 - 1;

	if (ratio > 1) {
		if(changeLenght) curLenght = std::ceil(ratio * (qreal)curLenght);
		else curHeight = std::ceil(ratio * (qreal)curHeight);
	}
	else{
		int reducedLength, reducedHeight;
		if (!checkShrinkSizeConstraint(curLenght, curHeight, reducedLength, reducedHeight, ratio)) return;
		if (changeLenght) curLenght = reducedLength;
		else curHeight = reducedHeight;
	}
}

void expandSmallerDimension(int &curLenght, int &curHeight, const qreal ratio) {
	curLenght = std::ceil(sqrt(ratio) * (qreal)curLenght);
	curHeight = std::ceil(sqrt(ratio) * (qreal)curHeight);
}

bool Packing2DThread::checkShrinkSizeConstraint(int &curLength, int &curHeight, int &reducedLength, int &reducedHeight, qreal ratio) {
	// Verify size constraints
	reducedLength = std::floor(ratio * (qreal)curLength);
	reducedHeight = std::floor(ratio * (qreal)curHeight);
	bool isPossibleToReduceLength = getShrinkedDimension(curLength, reducedLength, true);
	bool isPossibleToReduceHeight = getShrinkedDimension(curHeight, reducedHeight, false);
	if (!isPossibleToReduceLength && isPossibleToReduceHeight) curHeight = reducedHeight;
	if (!isPossibleToReduceHeight && isPossibleToReduceLength) curLength = reducedLength;
	if (!isPossibleToReduceLength || !isPossibleToReduceHeight) return false;
	return true;
}

void Packing2DThread::costShrinkContainerDimensions(int &curLenght, int &curHeight, RASTERVORONOIPACKING::RasterPackingSolution &currentSolution, const qreal ratio) {
	RASTERVORONOIPACKING::RasterPackingSolution tempSolution = currentSolution;

	// Verify size constraints
	int reducedLength, reducedHeight;
	if (!checkShrinkSizeConstraint(curLenght, curHeight, reducedLength, reducedHeight, ratio)) return;
	// FIXME: What if both are not reduceable??

	// Estimate cost in X
	this->solver->setContainerDimensions(reducedLength, curHeight, tempSolution);
	qreal costX = this->solver->getGlobalOverlap(tempSolution);

	// Reset container and solution
	this->solver->setContainerDimensions(curLenght, curHeight, tempSolution);
	tempSolution = currentSolution;

	// Estimate cost in Y
	this->solver->setContainerDimensions(curLenght, reducedHeight, tempSolution);
	qreal costY = this->solver->getGlobalOverlap(tempSolution);

	// Determine new height and length
	if (costX > costY) curHeight = reducedHeight;
	else curLenght = reducedLength;
}

void Packing2DThread::run() {

	switch (parameters.getRectangularPackingMethod()) {
	case RASTERVORONOIPACKING::SQUARE:
		runSquare();
		break;
	case RASTERVORONOIPACKING::RANDOM_ENCLOSED: case RASTERVORONOIPACKING::COST_EVALUATION: case RASTERVORONOIPACKING::BAGPIPE:
		runRectangle();
		break;
	}
	quit();
}

void Packing2DThread::runSquare() {
	m_abort = false;
	seed = QDateTime::currentDateTime().toTime_t();
	qsrand(seed);
	int itNum = 0;
	int totalItNum = 0;
	int worseSolutionsCount = 0;
	bool success = false;
	int curDim;
	qreal rdec = parameters.getRdec(); qreal rinc = parameters.getRinc();
	qreal areaDec = sqrt(1 - rdec), areaInc = sqrt(1 + rinc);
	qreal minOverlap = std::numeric_limits<qreal>::max();
	qreal curOverlap = minOverlap;
	QPair<RASTERVORONOIPACKING::RasterPackingSolution, int> lastFeasibleSolution, bestSolution;
	solver->resetWeights();
	int numLoops = 1;

	// Determine time to finish
	QDateTime finalTime = QDateTime::currentDateTime();
	finalTime = finalTime.addSecs(parameters.getTimeLimit());

	// Generate initial bottom left solution and determine the initial length
	solver->generateBottomLeftSolution(threadSolution, RASTERVORONOIPACKING::BL_SQUARE);
	curDim = solver->getCurrentWidth();
	lastFeasibleSolution = QPair<RASTERVORONOIPACKING::RasterPackingSolution, int>(threadSolution, curDim);
	bestSolution = lastFeasibleSolution;
	// Create solution snapshot
	ExecutionSolutionInfo minSuccessfullSol(curDim, curDim, 0, 1, seed);
	emit minimumLenghtUpdated(threadSolution, minSuccessfullSol);
	// Execution the first container reduction
	curDim = qRound(areaDec * solver->getCurrentWidth());
	solver->setContainerDimensions(curDim, curDim, threadSolution);

	minOverlap = solver->getGlobalOverlap(threadSolution);
	itNum++; totalItNum++;
	emit solutionGenerated(threadSolution, ExecutionSolutionInfo(curDim, curDim, 1, seed));

	qreal nextUpdateTime = QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 - UPDATEINTERVAL;
	while (QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 > 0 && (parameters.getIterationsLimit() == 0 || totalItNum < parameters.getIterationsLimit()) && !m_abort) {
		if (m_abort) break;
		while (worseSolutionsCount < parameters.getNmo() && QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 > 0 && (parameters.getIterationsLimit() == 0 || totalItNum < parameters.getIterationsLimit()) && !m_abort) {
			if (m_abort) break;
			solver->performLocalSearch(threadSolution);
			solver->updateWeights(threadSolution);
			curOverlap = solver->getGlobalOverlap(threadSolution);
			if (curOverlap < minOverlap || qFuzzyCompare(1.0 + 0.0, 1.0 + curOverlap)) {
				minOverlap = curOverlap;
				if (qFuzzyCompare(1.0 + 0.0, 1.0 + curOverlap)) { success = true; break; }
				worseSolutionsCount = 0;
			}
			else worseSolutionsCount++;
			if (QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 < nextUpdateTime) {
				nextUpdateTime = nextUpdateTime - UPDATEINTERVAL;
				emit statusUpdated(curDim, totalItNum, worseSolutionsCount, curOverlap, minOverlap, (parameters.getTimeLimit() * 1000 - QDateTime::currentDateTime().msecsTo(finalTime)) / 1000.0);
				emit solutionGenerated(threadSolution, ExecutionSolutionInfo(curDim, curDim, totalItNum, seed));
				emit weightsChanged();
			}
			itNum++; totalItNum++;
		}
		// Create solution snapshot
		if (!parameters.isFixedLength()) {
			// Reduce or expand container
			if (success) {
				emit minimumLenghtUpdated(threadSolution, ExecutionSolutionInfo(curDim, curDim, getTimeStamp(parameters.getTimeLimit(), finalTime), totalItNum, seed));
				numLoops = 1;
				if (solver->getCurrentWidth() < bestSolution.second) {
					minSuccessfullSol = ExecutionSolutionInfo(solver->getCurrentWidth(), solver->getCurrentWidth(), getTimeStamp(parameters.getTimeLimit(), finalTime), totalItNum, seed);
					bestSolution = QPair<RASTERVORONOIPACKING::RasterPackingSolution, int>(threadSolution, solver->getCurrentWidth());
				}
				curDim = qRound(areaDec * solver->getCurrentWidth());
				lastFeasibleSolution = QPair<RASTERVORONOIPACKING::RasterPackingSolution, int>(threadSolution, curDim);
			}
			else if (numLoops >= MAXLOOPSPERLENGTH) {
				numLoops = 1;
				if (solver->getCurrentWidth() == minSuccessfullSol.length - 1) {
					solver->generateRandomSolution(threadSolution);
				}
				else {
					curDim = std::ceil(areaInc * solver->getCurrentWidth());
					if (curDim >= minSuccessfullSol.length) {
						curDim = (curDim + minSuccessfullSol.length) / 2;
						if (curDim >= minSuccessfullSol.length) curDim = minSuccessfullSol.length - 1;
					}
				}
			}
			else numLoops++;

			solver->setContainerDimensions(curDim, curDim, threadSolution);
			success = false;
			minOverlap = solver->getGlobalOverlap(threadSolution);
		}
		itNum = 0; worseSolutionsCount = 0; solver->resetWeights();
		emit weightsChanged();
	}
	if (m_abort) { qDebug() << "Aborted!"; quit(); }
	solver->setContainerWidth(bestSolution.second, bestSolution.first);
	emit finishedExecution(bestSolution.first, minSuccessfullSol, totalItNum, curOverlap, minOverlap, getTimeStamp(parameters.getTimeLimit(), finalTime));
	quit();
}

void Packing2DThread::runRectangle() {
	m_abort = false;
	seed = QDateTime::currentDateTime().toTime_t();
	qsrand(seed);
	int itNum = 0;
	int totalItNum = 0;
	int worseSolutionsCount = 0;
	bool success = false;
	int curLenght, curHeight;
	qreal rdec = parameters.getRdec(); qreal rinc = parameters.getRinc();
	const qreal areaDec = 1 - rdec, areaInc = 1 + rinc;
	qreal minOverlap = std::numeric_limits<qreal>::max();
	qreal curOverlap = minOverlap;
	QPair<RASTERVORONOIPACKING::RasterPackingSolution, int> bestSolution;
	solver->resetWeights();
	int numLoops = 1;

	// Determine time to finish
	QDateTime finalTime = QDateTime::currentDateTime();
	finalTime = finalTime.addSecs(parameters.getTimeLimit());

	// Generate initial bottom left solution and determine the initial length
	solver->generateBottomLeftSolution(threadSolution, RASTERVORONOIPACKING::BL_RECTANGULAR);
	curLenght = solver->getCurrentWidth(); curHeight = solver->getCurrentHeight();
	bestSolution = QPair<RASTERVORONOIPACKING::RasterPackingSolution, int>(threadSolution, curLenght * curHeight);
	ExecutionSolutionInfo minSuccessfullSol(curLenght, curHeight, 0, 1, seed);
	// Create solution snapshot
	emit minimumLenghtUpdated(threadSolution, minSuccessfullSol);
	// Execution the first container reduction
	randomChangeContainerDimensions(curLenght, curHeight, areaDec);
	solver->setContainerDimensions(curLenght, curHeight, threadSolution);

	minOverlap = solver->getGlobalOverlap(threadSolution);
	itNum++; totalItNum++;
	emit solutionGenerated(threadSolution, ExecutionSolutionInfo(curLenght, curHeight, 1, seed));

	qreal nextUpdateTime = QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 - UPDATEINTERVAL;
	while (QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 > 0 && (parameters.getIterationsLimit() == 0 || totalItNum < parameters.getIterationsLimit()) && !m_abort) {
		if (m_abort) break;
		while (worseSolutionsCount < parameters.getNmo() && QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 > 0 && (parameters.getIterationsLimit() == 0 || totalItNum < parameters.getIterationsLimit()) && !m_abort) {
			if (m_abort) break;
			solver->performLocalSearch(threadSolution);
			solver->updateWeights(threadSolution);
			curOverlap = solver->getGlobalOverlap(threadSolution);
			if (curOverlap < minOverlap || qFuzzyCompare(1.0 + 0.0, 1.0 + curOverlap)) {
				minOverlap = curOverlap;
				if (qFuzzyCompare(1.0 + 0.0, 1.0 + curOverlap)) { success = true; break; }
				worseSolutionsCount = 0;
			}
			else worseSolutionsCount++;
			if (QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 < nextUpdateTime) {
				nextUpdateTime = nextUpdateTime - UPDATEINTERVAL;
				emit statusUpdated(curLenght, totalItNum, worseSolutionsCount, curOverlap, minOverlap, (parameters.getTimeLimit() * 1000 - QDateTime::currentDateTime().msecsTo(finalTime)) / 1000.0);
				emit solutionGenerated(threadSolution, ExecutionSolutionInfo(curLenght, curHeight, totalItNum, seed));
				emit weightsChanged();
			}
			itNum++; totalItNum++;
		}
		// Reduce or expand container
		int currentArea = solver->getCurrentWidth() * solver->getCurrentHeight();
		if (success) {
			numLoops = 1;
			if (currentArea < bestSolution.second) {
				minSuccessfullSol = ExecutionSolutionInfo(curLenght, curHeight, getTimeStamp(parameters.getTimeLimit(), finalTime), totalItNum, seed);
				bestSolution = QPair<RASTERVORONOIPACKING::RasterPackingSolution, int>(threadSolution, currentArea);
				emit minimumLenghtUpdated(threadSolution, ExecutionSolutionInfo(curLenght, curHeight, getTimeStamp(parameters.getTimeLimit(), finalTime), totalItNum, seed));
			}
			if (parameters.getRectangularPackingMethod() == RASTERVORONOIPACKING::RANDOM_ENCLOSED) randomChangeContainerDimensions(curLenght, curHeight, areaDec);
			else if (parameters.getRectangularPackingMethod() == RASTERVORONOIPACKING::COST_EVALUATION) costShrinkContainerDimensions(curLenght, curHeight, threadSolution, areaDec);
			else if (parameters.getRectangularPackingMethod() == RASTERVORONOIPACKING::BAGPIPE) changeKeepAspectRatio(curLenght, curHeight, areaDec);
		}
		else if (numLoops >= MAXLOOPSPERLENGTH) {
			numLoops = 1;
			if (parameters.getRectangularPackingMethod() == RASTERVORONOIPACKING::RANDOM_ENCLOSED) randomChangeContainerDimensions(curLenght, curHeight, areaInc);
			else if (parameters.getRectangularPackingMethod() == RASTERVORONOIPACKING::COST_EVALUATION) expandSmallerDimension(curLenght, curHeight, areaInc);
			else if (parameters.getRectangularPackingMethod() == RASTERVORONOIPACKING::BAGPIPE) {
				if (areaInc * (qreal)curLenght * (qreal)curHeight > minSuccessfullSol.area)	bagpipeChangeContainerDimensions(curLenght, curHeight);
				else changeKeepAspectRatio(curLenght, curHeight, areaInc);
			}
		}
		else numLoops++;

		solver->setContainerDimensions(curLenght, curHeight, threadSolution);
		success = false;
		minOverlap = solver->getGlobalOverlap(threadSolution);

		itNum = 0; worseSolutionsCount = 0; solver->resetWeights();
		emit weightsChanged();
	}
	if (m_abort) { qDebug() << "Aborted!"; quit(); }
	solver->setContainerDimensions(curLenght, curHeight, threadSolution);
	emit finishedExecution(bestSolution.first, minSuccessfullSol, totalItNum, curOverlap, minOverlap, (parameters.getTimeLimit() * 1000 - QDateTime::currentDateTime().msecsTo(finalTime)) / 1000.0);
}
