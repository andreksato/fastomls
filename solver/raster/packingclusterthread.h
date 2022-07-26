#ifndef PACKINGCLUSTERTHREAD_H
#define PACKINGCLUSTERTHREAD_H

#include "packingthread.h"

class PackingClusterThread : public PackingThread {
	Q_OBJECT
public:
	PackingClusterThread(QObject *parent = 0) {};
	~PackingClusterThread() {};

	void setSolver(std::shared_ptr<RASTERVORONOIPACKING::RasterStripPackingSolver> _solver) {
		PackingThread::setSolver(_solver);
		clusterSolver = std::dynamic_pointer_cast<RASTERVORONOIPACKING::RasterStripPackingClusterSolver>(_solver);
	}

signals:
	void unclustered(const RASTERVORONOIPACKING::RasterPackingSolution &solution, int length, qreal elapsed);

protected:
	std::shared_ptr<RASTERVORONOIPACKING::RasterStripPackingClusterSolver> clusterSolver;

	void run();
};

#endif // PACKINGCLUSTERTHREAD_H
