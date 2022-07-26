#include "packingthread.h"
#include <limits>
#include <QDebug>
#include <QTime>

#define MAXLOOPSPERLENGTH 5
#define UPDATEINTERVAL 2

PackingThread::~PackingThread() {
    m_abort = true;
    wait();
}
void PackingThread::setInitialSolution(RASTERVORONOIPACKING::RasterPackingSolution &initialSolution) {
    threadSolution = RASTERVORONOIPACKING::RasterPackingSolution(initialSolution.getNumItems());
    for(int i = 0; i < initialSolution.getNumItems(); i++) {
        threadSolution.setPosition(i, initialSolution.getPosition(i));
        threadSolution.setOrientation(i, initialSolution.getOrientation(i));
    }
}

qreal PackingThread::getTimeStamp(int timeLimit, QDateTime &finalTime) {
	qreal ans = (timeLimit * 1000 - QDateTime::currentDateTime().msecsTo(finalTime)) / 1000.0;
	return ans;
}

void PackingThread::run()
{
	m_abort = false;
	seed = QDateTime::currentDateTime().toTime_t();
	qsrand(seed);
    int itNum = 0;
    int totalItNum = 0;
    int worseSolutionsCount = 0;
	bool success = false;
	int curLength = solver->getCurrentWidth();
	int containerHeight = solver->getCurrentHeight();
	ExecutionSolutionInfo minSuccessfullSol(curLength, 0, seed);
	qreal curRealLength = (qreal)minSuccessfullSol.length;
	qreal rdec = parameters.getRdec(); qreal rinc = parameters.getRinc();
	qreal minOverlap = std::numeric_limits<qreal>::max();
	qreal curOverlap = minOverlap;
    RASTERVORONOIPACKING::RasterPackingSolution bestSolution = threadSolution;
    solver->resetWeights();
	int numLoops = 1;

	// Determine time to finish
	QDateTime finalTime = QDateTime::currentDateTime();
	finalTime = finalTime.addSecs(parameters.getTimeLimit());

	// Generate initial solution
	if (parameters.getInitialSolMethod() == RASTERVORONOIPACKING::RANDOMFIXED) solver->generateRandomSolution(threadSolution);
	if (parameters.getInitialSolMethod() == RASTERVORONOIPACKING::BOTTOMLEFT)  {
		// Generate initial bottom left solution and determine the initial length
		solver->generateBottomLeftSolution(threadSolution);
		curLength = solver->getCurrentWidth();
		minSuccessfullSol = ExecutionSolutionInfo(curLength, (qreal)curLength * (qreal)containerHeight, 0.0, 1, seed);
		emit minimumLenghtUpdated(threadSolution, minSuccessfullSol);
		
		// Execution the first container reduction
		curRealLength = (1.0 - rdec)*(qreal)solver->getCurrentWidth();
		curLength = qRound(curRealLength);
		solver->setContainerWidth(curLength, threadSolution);
	}
	minOverlap = solver->getGlobalOverlap(threadSolution);
	itNum++; totalItNum++;
	emit solutionGenerated(threadSolution, ExecutionSolutionInfo(minSuccessfullSol.length, 1, seed));

	qreal nextUpdateTime = QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 - UPDATEINTERVAL;
	while (QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 > 0 && (parameters.getIterationsLimit() == 0 || totalItNum < parameters.getIterationsLimit()) && !m_abort) {
		if (m_abort) break;
		while (worseSolutionsCount < parameters.getNmo() && QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 > 0 && (parameters.getIterationsLimit() == 0 || totalItNum < parameters.getIterationsLimit()) && !m_abort) {
			if(m_abort) break;
			solver->performLocalSearch(threadSolution);
			solver->updateWeights(threadSolution);
			curOverlap = solver->getGlobalOverlap(threadSolution);
			if(curOverlap < minOverlap) {
				minOverlap = curOverlap;
				if(parameters.isFixedLength()) bestSolution = threadSolution; // Best solution for the minimum overlap problem
				if (qFuzzyCompare(1.0 + 0.0, 1.0 + curOverlap)) { success = true; break; }
				worseSolutionsCount = 0;
			}
			else worseSolutionsCount++;
			if (QDateTime::currentDateTime().msecsTo(finalTime) / 1000.0 < nextUpdateTime) {
				nextUpdateTime = nextUpdateTime - UPDATEINTERVAL;
				emit statusUpdated(curLength, totalItNum, worseSolutionsCount, curOverlap, minOverlap, (parameters.getTimeLimit()*1000-QDateTime::currentDateTime().msecsTo(finalTime))/1000.0);
				emit solutionGenerated(threadSolution, ExecutionSolutionInfo(curLength, totalItNum, seed));
				emit weightsChanged();
			}
			itNum++; totalItNum++;
		}
		if (!parameters.isFixedLength()) {
			// Reduce or expand container
			if(success) {
				numLoops = 1;
				bestSolution = threadSolution; minSuccessfullSol = ExecutionSolutionInfo(curLength, (qreal)curLength * (qreal)containerHeight, getTimeStamp(parameters.getTimeLimit(), finalTime), totalItNum, seed);
				curRealLength = (1.0 - rdec)*(qreal)solver->getCurrentWidth();
				curLength = qRound(curRealLength);
				emit minimumLenghtUpdated(bestSolution, minSuccessfullSol);
			}
			else if (numLoops >= MAXLOOPSPERLENGTH) {
				numLoops = 1;
				if (qRound((1.0 + rinc)*curRealLength) >= minSuccessfullSol.length) {
					curRealLength = (curRealLength + minSuccessfullSol.length) / 2;
					if (qRound(curRealLength) == minSuccessfullSol.length)
						curRealLength = minSuccessfullSol.length - 1;
					if (curLength == qRound(curRealLength)) solver->generateRandomSolution(threadSolution);
					curLength = qRound(curRealLength);
				}
				else {
					curRealLength = (1.0 + rinc)*curRealLength;
					curLength = qRound(curRealLength);
				}
			}
			else numLoops++;

			solver->setContainerWidth(curLength, threadSolution);
			success = false;
			minOverlap = solver->getGlobalOverlap(threadSolution);
		}
		else {
			if (success) break;
			else {
				if (numLoops > MAXLOOPSPERLENGTH) { solver->generateRandomSolution(threadSolution); numLoops = 1; }
				else numLoops++;
			}
		}
		itNum = 0; worseSolutionsCount = 0; solver->resetWeights();
		//emit weightsChanged();
	}
	if (m_abort) {qDebug() << "Aborted!"; quit();}
	solver->setContainerWidth(minSuccessfullSol.length, bestSolution);
	emit finishedExecution(bestSolution, minSuccessfullSol, totalItNum, curOverlap, minOverlap, getTimeStamp(parameters.getTimeLimit(), finalTime));
	quit();
}

void PackingThread::abort() {
	m_abort = true;
	wait();
}