#ifndef CONSOLEPACKINGLOADER_H
#define CONSOLEPACKINGLOADER_H

#include <QObject>
#include <QString>
#include "raster/packingthread.h"
#include "raster/packing2dthread.h"

namespace RASTERVORONOIPACKING { class RasterStripPackingParameters; }

struct GpuMemoryRequirements {
	GpuMemoryRequirements() {};

	size_t totalIfpMemory;
	size_t totalNfpMemory;
	size_t maxSingleIfpMemory;
};

class ConsolePackingLoader : public QObject
{
	Q_OBJECT

public:
	explicit ConsolePackingLoader(QObject *parent = 0);

	void setParameters(QString inputFilePath, QString outputTXTFile, QString outputXMLFile, RASTERVORONOIPACKING::RasterStripPackingParameters &algorithmParams, bool appendSeed = false);
	void run();

public slots :
	void printExecutionStatus(int curLength, int totalItNum, int worseSolutionsCount, qreal  curOverlap, qreal minOverlap, qreal elapsed);
	void saveMinimumResult(const RASTERVORONOIPACKING::RasterPackingSolution &solution, const ExecutionSolutionInfo &info);
	void saveFinalResult(const RASTERVORONOIPACKING::RasterPackingSolution &bestSolution, const ExecutionSolutionInfo &info, int totalIt, qreal  curOverlap, qreal minOverlap, qreal totalTime);
	void updateUnclusteredProblem(const RASTERVORONOIPACKING::RasterPackingSolution &solution, int length, qreal elapsed);
	void threadFinished();

signals:
	void quitApp();

private:
	bool loadInputFile(QString inputFilePath, std::shared_ptr<RASTERVORONOIPACKING::RasterPackingProblem> *problem);
	void writeNewLength(int length, int totalItNum, qreal elapsed, uint threadSeed);
	void writeNewLength2D(const ExecutionSolutionInfo &info, int totalItNum, qreal elapsed, uint threadSeed);
	void saveXMLSolution(const RASTERVORONOIPACKING::RasterPackingSolution &solution, const ExecutionSolutionInfo &info);
	RASTERVORONOIPACKING::RasterStripPackingParameters algorithmParamsBackup;
	std::shared_ptr<RASTERVORONOIPACKING::RasterPackingProblem> problem;
	QString outputTXTFile, outputXMLFile;
	QMap<int, QString*> outlogContents;
	bool appendSeedToOutputFiles;
	int numProcesses;
	QVector<std::shared_ptr<PackingThread>> threadVector;
	QVector<QPair<std::shared_ptr<RASTERVORONOIPACKING::RasterPackingSolution>, ExecutionSolutionInfo>> solutionsCompilation;
	QMap<uint, QList<ExecutionSolutionInfo>> solutionInfoHistory;
	qreal totalArea;
	int containerWidth;
};

#endif // CONSOLEPACKINGLOADER_H
