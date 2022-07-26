#include <QCommandLineParser>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "packingParametersParser.h"

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, ConsolePackingArgs *params, QString *errorMessage)
{
    bool zoomedInputFileSet = false;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    parser.addPositionalArgument("source","Input problem file path.");
    const QCommandLineOption nameZoomedInput(QStringList() << "zoom", "Zoomed problem input: file name or explicity zoom value.", "name");
    parser.addOption(nameZoomedInput);
    const QCommandLineOption nameOutputTXT(QStringList() << "result", "The output result statistics file name.", "name");
    parser.addOption(nameOutputTXT);
    const QCommandLineOption nameOutputXML(QStringList() << "layout", "The output layout XML file name.", "name");
    parser.addOption(nameOutputXML);
	const QCommandLineOption boolOutputSeed("appendseed", "Automatically append seed value to output file names.");
	parser.addOption(boolOutputSeed);

    const QCommandLineOption typeMethod("method", "Raster packing method choices: default, gls.", "type");
    parser.addOption(typeMethod);
    const QCommandLineOption typeInitialSolution("initial", "Initial solution choices: random, bottomleft.", "type");
    parser.addOption(typeInitialSolution);
    const QCommandLineOption valueMaxWorseSolutions("nmo", "Maximum number of non-best solutions.", "value");
    parser.addOption(valueMaxWorseSolutions);
    const QCommandLineOption valueTimeLimit("duration", "Time limit in seconds.", "value");
    parser.addOption(valueTimeLimit);
	const QCommandLineOption valueRatios("ratios", "Ratios for container increase / decrease given in rdec;rinc form.", "value;value");
	parser.addOption(valueRatios);
	const QCommandLineOption valueIterationsLimit("maxits", "Maximum number of iterations.", "value");
	parser.addOption(valueIterationsLimit);
    const QCommandLineOption valueLenght("length", "Container lenght.", "value");
    parser.addOption(valueLenght);
	const QCommandLineOption boolStripPacking("strippacking", "Strip packing version.");
	parser.addOption(boolStripPacking);
	const QCommandLineOption valueNumThreads("parallel", "Number of parallel executions of the algorithm.", "value");
	parser.addOption(valueNumThreads);
	const QCommandLineOption valueCluster("clusterfactor", "Time fraction for cluster executuion.", "value");
	parser.addOption(valueCluster);
	const QCommandLineOption valueRectangularPacking("rectpacking", "Rectangular packing version. Choices: square, random, cost, bagpipe", "value");
	parser.addOption(valueRectangularPacking);

    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    if (!parser.parse(QCoreApplication::arguments())) {
        *errorMessage = parser.errorText();
        return CommandLineError;
    }

    if (parser.isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser.isSet(helpOption))
        return CommandLineHelpRequested;

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        *errorMessage = "Argument missing: 'source'.";
        return CommandLineError;
    }
    if (positionalArguments.size() > 1) {
        *errorMessage = "Too many arguments specified.";
        return CommandLineError;
    }
    params->inputFilePath = positionalArguments.at(0);

    if (parser.isSet(nameZoomedInput)) {
        const QString inputName = parser.value(nameZoomedInput);
		bool ok;
		const qreal zoomValue = inputName.toDouble(&ok);
		if (!ok || zoomValue < 0) {
			*errorMessage = "Bad zoom value.";
			return CommandLineError;
		}
		params->zoomValue = zoomValue;
    }
	else params->zoomValue = -1.0;

    if (parser.isSet(nameOutputTXT)) {
        const QString outputName = parser.value(nameOutputTXT);
        params->outputTXTFile = outputName;
    }
    else params->outputTXTFile = "outlog.dat";

    if (parser.isSet(nameOutputXML)) {
        const QString outputName = parser.value(nameOutputXML);
        params->outputXMLFile = outputName;
    }
    else params->outputXMLFile = "bestSol.xml";
	params->appendSeedToOutputFiles = parser.isSet(boolOutputSeed);

    if (parser.isSet(typeMethod)) {
        const QString methodType = parser.value(typeMethod).toLower();
        if(methodType != "default" && methodType != "gls") {
            *errorMessage = "Invalid method type! Avaible methods: 'default', 'gls'.";
            return CommandLineError;
        }
        if(methodType == "default") params->methodType = Method_Default;
        if(methodType == "gls") params->methodType = Method_Gls;
    }
    else {
        qWarning() << "Warning: Method not specified, set to default (no zoom, no gls).";
        params->methodType = Method_Default;
    }

    if (parser.isSet(typeInitialSolution)) {
        const QString solutionType = parser.value(typeInitialSolution).toLower();
		if (solutionType != "random" && solutionType != "bottomleft") {
            *errorMessage = "Invalid initial solution type! Avaible methods: 'random, bottomleft'.";
            return CommandLineError;
        }
        if(solutionType == "random") params->initialSolutionType = Solution_Random;
		if (solutionType == "bottomleft") params->initialSolutionType = Bottom_Left;
    }
    else params->initialSolutionType = Solution_Random;

    if (parser.isSet(valueMaxWorseSolutions)) {
        const QString nmoString = parser.value(valueMaxWorseSolutions);
        bool ok;
        const int nmo = nmoString.toInt(&ok);
        if(ok && nmo > 0) params->maxWorseSolutionsValue = nmo;
        else {
            *errorMessage = "Bad Nmo value.";
            return CommandLineError;
        }
    }
    else {
        qWarning() << "Warning: Nmo not found, set to default (200).";
        params->maxWorseSolutionsValue = 200;
    }

    if (parser.isSet(valueTimeLimit)) {
        const QString secsString = parser.value(valueTimeLimit);
        bool ok;
        const int timeLimit = secsString.toInt(&ok);
        if(ok && timeLimit > 0) params->timeLimitValue = timeLimit;
        else {
            *errorMessage = "Bad time limit value (must be expressed in seconds).";
            return CommandLineError;
        }
    }
    else {
        qWarning() << "Warning: Time limit not found, set to default (600s).";
        params->timeLimitValue = 600;
    }

	if (parser.isSet(valueIterationsLimit)) {
		const QString secsString = parser.value(valueIterationsLimit);
		bool ok;
		const int itLimit = secsString.toInt(&ok);
		if (ok && itLimit > 0) params->iterationsLimitValue = itLimit;
		else {
			*errorMessage = "Bad iteration limit value (must be expressed in seconds).";
			return CommandLineError;
		}
	}
	else {
		params->iterationsLimitValue = 0;
	}

    if (parser.isSet(valueLenght)) {
        const QString lengthString = parser.value(valueLenght);
        bool ok;
        const float containerLenght = lengthString.toFloat(&ok);
        if(ok && containerLenght > 0) {
            params->containerLenght = containerLenght;
            params->originalContainerLenght = false;
        }
        else {
            *errorMessage = "Bad container lenght value.";
            return CommandLineError;
        }
    }
    else params->originalContainerLenght = true;

	if (parser.isSet(boolStripPacking)) params->stripPacking = true;
	else  params->stripPacking = false;

	if (parser.isSet(valueRectangularPacking)) {
		params->rectangularPacking = true;
		const QString methodType = parser.value(valueRectangularPacking).toLower();
		if (methodType != "square" && methodType != "random" && methodType != "cost" && methodType != "bagpipe") {
			*errorMessage = "Invalid initial rectangular method type! Avaible methods: 'square', 'random', 'cost' and 'bagpipe'.";
			return CommandLineError;
		}
		if (methodType == "square") params->rectMehod = SQUARE;
		if (methodType == "random") params->rectMehod = RANDOM_ENCLOSED;
		if (methodType == "cost") params->rectMehod = COST_EVALUATION;
		if (methodType == "bagpipe") params->rectMehod = BAGPIPE;
	}
	else params->rectangularPacking = false;

	if (parser.isSet(valueNumThreads)) {
		const QString threadsString = parser.value(valueNumThreads);
		bool ok;
		const int nthreads = threadsString.toInt(&ok);
		if (ok && threadsString > 0) params->numThreads = nthreads;
		else {
			*errorMessage = "Bad parallel value.";
			return CommandLineError;
		}
	}
	else {
		params->numThreads = 1;
	}

	if (parser.isSet(valueCluster)) {
		const QString clusterString = parser.value(valueCluster);
		bool ok;
		const float clusterFactor = clusterString.toFloat(&ok);
		if (ok && clusterFactor >= 0 && clusterFactor <=1.0) {
			params->clusterFactor = clusterFactor;
		}
		else {
			*errorMessage = "Bad cluster factor value.";
			return CommandLineError;
		}
	}
	else params->clusterFactor = -1.0;

	if (parser.isSet(valueRatios)) {
		const QString ratiosStr = parser.value(valueRatios);
		QStringList ratiosList = ratiosStr.split(";");
		bool ok;
		qreal rdec = ratiosList[0].toDouble(&ok);
		qreal rinc = ratiosList[1].toDouble(&ok);
		if (ok && rdec > 0 && rinc > 0) {
			params->rdec = rdec;
			params->rinc = rinc;
		}
		else {
			*errorMessage = "Bad ratio value.";
			return CommandLineError;
		}
	}
	else {
		params->rdec = -1.0; params->rinc = -1.0;
	}

    return CommandLineOk;
}
