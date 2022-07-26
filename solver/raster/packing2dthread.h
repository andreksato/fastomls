#ifndef PACKING2DTHREAD_H
#define PACKING2DTHREAD_H

#include "packingthread.h"

class Packing2DThread : public PackingThread {
	Q_OBJECT
public:
	Packing2DThread(QObject *parent = 0) {};
	~Packing2DThread() {};
	
protected:
	void run();

private:
	void runSquare();
	void runRectangle();
	void costShrinkContainerDimensions(int &curLenght, int &curHeight, RASTERVORONOIPACKING::RasterPackingSolution &currentSolution, const qreal ratio);
	void randomChangeContainerDimensions(int &curLenght, int &curHeight, const qreal ratio);
	void bagpipeChangeContainerDimensions(int &curLenght, int &curHeight);
	void changeKeepAspectRatio(int &curLenght, int &curHeight, const qreal ratio);
	bool getShrinkedDimension(int dim, int &newDim, bool length);
	bool checkShrinkSizeConstraint(int &curLength, int &curHeight, int &reducedLength, int &reducedHeight, qreal ratio);
};

#endif // PACKING2DTHREAD_H
