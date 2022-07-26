#include <QCoreApplication>

#include "packingproblem.h"
#include "raster/rasterpackingproblem.h"
#include "raster/rasterpackingsolution.h"
#include "raster/rasterstrippackingsolver.h"
#include "raster/rasterstrippackingparameters.h"
#include "raster/packingthread.h"
#include "packingParametersParser.h"
#include "consolepackingloader.h"
#include <QDebug>
#include <QDir>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <memory>

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	QCoreApplication::setApplicationName("Raster Packing Console");
	QCoreApplication::setApplicationVersion("0.1");
	ConsolePackingLoader packingLoader;
	QObject::connect(&packingLoader, SIGNAL(quitApp()), &app, SLOT(quit()));

	// --> Parse command line arguments
	QCommandLineParser parser;
	parser.setApplicationDescription("Raster Packing console version.");
	ConsolePackingArgs params;
	RASTERVORONOIPACKING::RasterStripPackingParameters algorithmParams;
	QString errorMessage;
	switch (parseCommandLine(parser, &params, &errorMessage)) {
	case CommandLineOk:
		break;
	case CommandLineError:
		fputs(qPrintable(errorMessage), stderr);
		fputs("\n\n", stderr);
		fputs(qPrintable(parser.helpText()), stderr);
		return 1;
	case CommandLineVersionRequested:
		printf("%s %s\n", qPrintable(QCoreApplication::applicationName()),
			qPrintable(QCoreApplication::applicationVersion()));
		return 0;
	case CommandLineHelpRequested:
		parser.showHelp();
		Q_UNREACHABLE();
	}

	// Transform source path to absolute
	QFileInfo inputFileInfo(params.inputFilePath);
	params.inputFilePath = inputFileInfo.absoluteFilePath();
	// Check if source paths exist
	if (!QFile(params.inputFilePath).exists()) {
		qCritical() << "Input file not found.";
		return 1;
	}

	// Configure parameters
	switch (params.methodType) {
	case Method_Default: algorithmParams.setHeuristic(RASTERVORONOIPACKING::NONE); break;
	case Method_Gls: algorithmParams.setHeuristic(RASTERVORONOIPACKING::GLS); break;
	}
	algorithmParams.setFixedLength(!params.stripPacking);
	if (params.rdec > 0 && params.rinc > 0) algorithmParams.setResizeChangeRatios(params.rdec, params.rinc);
	algorithmParams.setTimeLimit(params.timeLimitValue); algorithmParams.setNmo(params.maxWorseSolutionsValue);
	algorithmParams.setIterationsLimit(params.iterationsLimitValue);
	if (params.initialSolutionType == Solution_Random) {
		algorithmParams.setInitialSolMethod(RASTERVORONOIPACKING::RANDOMFIXED);
		algorithmParams.setInitialLenght(params.containerLenght);
	}
	else if (params.initialSolutionType == Bottom_Left){
		algorithmParams.setInitialSolMethod(RASTERVORONOIPACKING::BOTTOMLEFT);
	}
	algorithmParams.setClusterFactor(params.clusterFactor);
	algorithmParams.setRectangularPacking(params.rectangularPacking);
	switch (params.rectMehod) {
	case SQUARE: algorithmParams.setRectangularPackingMethod(RASTERVORONOIPACKING::SQUARE); break;
	case RANDOM_ENCLOSED: algorithmParams.setRectangularPackingMethod(RASTERVORONOIPACKING::RANDOM_ENCLOSED); break;
	case COST_EVALUATION: algorithmParams.setRectangularPackingMethod(RASTERVORONOIPACKING::COST_EVALUATION); break;
	case BAGPIPE: algorithmParams.setRectangularPackingMethod(RASTERVORONOIPACKING::BAGPIPE); break;
	}
	algorithmParams.setSearchScale(params.zoomValue);

	packingLoader.setParameters(params.inputFilePath, params.outputTXTFile, params.outputXMLFile, algorithmParams, params.appendSeedToOutputFiles);

	if (params.numThreads > 1) qDebug() << "Multithreaded version:" << params.numThreads << "parallel executions.";
	for (int i = 0; i < params.numThreads; i++) {
		packingLoader.run();
		QThread::sleep(1);
	}
    return app.exec();
}
