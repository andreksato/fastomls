#ifndef PACKINGPARAMETERSPARSER_H
#define PACKINGPARAMETERSPARSER_H

class QCommandLineParser;
class QString;

enum RasterPackingMethods {Method_Default, Method_Gls};
enum InitialSolutionGenerator {Solution_Random, Bottom_Left};
enum RectangularMethod { SQUARE, RANDOM_ENCLOSED, COST_EVALUATION, BAGPIPE };

struct ConsolePackingArgs {
	ConsolePackingArgs() {}

    QString inputFilePath;
    QString outputTXTFile;
    QString outputXMLFile;

    RasterPackingMethods methodType;
    InitialSolutionGenerator initialSolutionType;
	RectangularMethod rectMehod;
	qreal zoomValue;
    int maxWorseSolutionsValue;
    int timeLimitValue;
	int iterationsLimitValue;
    qreal containerLenght;
	qreal clusterFactor;
	qreal rdec, rinc;

    bool originalContainerLenght;
	bool stripPacking;
	bool rectangularPacking;
	bool appendSeedToOutputFiles;

	int numThreads;
};

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, ConsolePackingArgs *params, QString *errorMessage);
CommandLineParseResult parseOptionsFile(QString fileName, ConsolePackingArgs *params, QString *errorMessage);

#endif // PACKINGPARAMETERSPARSER_H
