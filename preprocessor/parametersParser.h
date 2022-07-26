#ifndef PARSECOMMANDLINE_H
#define PARSECOMMANDLINE_H

class QCommandLineParser;
class QString;

struct PreProcessorParameters {
	PreProcessorParameters() : noOverlap(true) {}

    QString inputFilePath;
    QString inputFileType;
    qreal puzzleScaleFactor; // Nofit polygon precision
    QString outputDir;
    qreal rasterScaleFactor; // Rasterization precision
	qreal scaleFixFactor; // Correction scale factor for CFREFP problems
    bool saveRaster;
    QString outputXMLName;
    QString outputFormat;
    QString headerFile;
    QString optionsFile;
	qreal innerFitEpsilon; // Epsilon for ifp rasterization
	bool noOverlap; // Create nfps with contours
	QList<int> clusterRankings;
	QString clusterPrefix, clusterFile, clusterInfoFile;
	QList<qreal> clusterWeights;
};

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, PreProcessorParameters *params, QString *errorMessage);
CommandLineParseResult parseOptionsFile(QString fileName, PreProcessorParameters *params, QString *errorMessage);

#endif // PARSECOMMANDLINE_H
