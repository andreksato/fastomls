#ifndef PACKINGTHREAD_H
#define PACKINGTHREAD_H

#include <memory>
#include "rasterpackingsolution.h"
#include "rasterstrippackingsolver.h"

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

struct ExecutionSolutionInfo {
	ExecutionSolutionInfo() { twodim = false; }
	ExecutionSolutionInfo(int _length, int _iteration, uint _seed) : length(_length), iteration(_iteration), timestamp(0.0), seed(_seed) { twodim = false; }
	ExecutionSolutionInfo(int _length, int _height, int _iteration, uint _seed) : length(_length), height(_height), iteration(_iteration), timestamp(0.0), seed(_seed) { twodim = true; }
	ExecutionSolutionInfo(int _length, qreal _timestamp, int _iteration, uint _seed) : length(_length), timestamp(_timestamp), iteration(_iteration), seed(_seed) { twodim = false; }
	ExecutionSolutionInfo(int _length, qreal _area, qreal _timestamp, int _iteration, uint _seed) : length(_length), area(_area), timestamp(_timestamp), iteration(_iteration), seed(_seed) { twodim = false; }
	ExecutionSolutionInfo(int _length, int _height, qreal _timestamp, int _iteration, uint _seed) : length(_length), height(_height), timestamp(_timestamp), iteration(_iteration), seed(_seed) { area = _length * _height; twodim = true; }
	int length, height;
	qreal area;
	qreal timestamp;
	int iteration;
	bool twodim;
	uint seed;
};

class PackingThread : public QThread
{
    Q_OBJECT
public:
	PackingThread(QObject *parent = 0) {};
    ~PackingThread();

    void setInitialSolution(RASTERVORONOIPACKING::RasterPackingSolution &initialSolution);
    virtual void setSolver(std::shared_ptr<RASTERVORONOIPACKING::RasterStripPackingSolver> _solver) {
		solver =_solver;
		threadSolution = RASTERVORONOIPACKING::RasterPackingSolution(solver->getNumItems());
	}
	void setParameters(RASTERVORONOIPACKING::RasterStripPackingParameters &_parameters) { parameters.Copy(_parameters); }
	uint getSeed() { return seed; }

protected:
	virtual void run();

signals:
	// Basic signals
	void statusUpdated(int curLength, int totalItNum, int worseSolutionsCount, qreal curOverlap, qreal minOverlap, qreal elapsed);
	void minimumLenghtUpdated(const RASTERVORONOIPACKING::RasterPackingSolution &solution, const ExecutionSolutionInfo &info);
	void finishedExecution(const RASTERVORONOIPACKING::RasterPackingSolution &solution, const ExecutionSolutionInfo &info, int totalItNum, qreal curOverlap, qreal minOverlap, qreal elapsed);
	// GUI signals
	void solutionGenerated(const RASTERVORONOIPACKING::RasterPackingSolution &solution, const ExecutionSolutionInfo &info);
	void weightsChanged();

public slots:
	void abort();

protected:
	qreal getTimeStamp(int timeLimit, QDateTime &finalTime);
	uint seed; 
	bool m_abort; 
	RASTERVORONOIPACKING::RasterStripPackingParameters parameters;
	RASTERVORONOIPACKING::RasterPackingSolution threadSolution;
	std::shared_ptr<RASTERVORONOIPACKING::RasterStripPackingSolver> solver;
};

#endif // PACKINGTHREAD_H
