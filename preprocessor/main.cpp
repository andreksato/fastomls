
#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QImage>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTime>
#include <iostream>
#include "colormap.h"
#include "parametersParser.h"
#include "packingproblem.h"
#include "cuda_runtime.h"
#include "cluster/clusterizator.h"

cudaError_t transform(int *S, float *D, int N, int M);

QImage getImageFromVec(int *S, int width, int height)
{
	QImage result(width, height, QImage::Format_Mono);
	result.setColor(1, qRgb(255, 255, 255));
	result.setColor(0, qRgb(255, 0, 0));
	result.fill(0);
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			result.setPixel(j, i, S[i * width + j]);
	return result;
}

void writePathSvg(QPolygonF &polygon, QTextStream &stream)
{
	QPolygonF::iterator it = polygon.begin();
	stream << "<path d=\"M" << (*it).x() << " " << (*it).y() << " ";
	for (++it; it != polygon.end(); it++)
		stream << "L" << (*it).x() << " " << (*it).y() << " ";
	stream << "Z\" />" << endl;
}

void printCluster(RASTERPACKING::PackingProblem &problem, QString fileName, std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> noFitPolygon, QList<QPointF> &displacements)
{
	// Get polygons
	QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itStatic = std::find_if(problem.pbegin(), problem.pend(), [noFitPolygon](std::shared_ptr<RASTERPACKING::Piece> s)
																				   { return s->getPolygon()->getName() == noFitPolygon->getStaticName(); });
	QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itOrbiting = std::find_if(problem.pbegin(), problem.pend(), [noFitPolygon](std::shared_ptr<RASTERPACKING::Piece> s)
																					 { return s->getPolygon()->getName() == noFitPolygon->getOrbitingName(); });
	if (itStatic == problem.pend() || itOrbiting == problem.pend())
	{
		qDebug() << "Could not read polygons from nfp" << noFitPolygon->getName();
		return;
	}
	std::shared_ptr<RASTERPACKING::Polygon> staticPolygon = (*itStatic)->getPolygon();
	std::shared_ptr<RASTERPACKING::Polygon> orbitingPolygon = (*itOrbiting)->getPolygon();
	// qDebug() << "Static has" << staticPolygon->length() << "vertices and orbiting has" << orbitingPolygon->length() << "vertices.";
	QPolygonF newStaticPolygon;
	newStaticPolygon << *staticPolygon;
	newStaticPolygon = QTransform().rotate(noFitPolygon->getStaticAngle()).map(newStaticPolygon);

	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly))
	{
		QTextStream stream(&file);
		stream << "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">" << endl;
		writePathSvg(newStaticPolygon, stream);
		foreach (QPointF pt, displacements)
		{
			QPolygonF newOrbitingPolygon;
			newOrbitingPolygon << *orbitingPolygon;
			newOrbitingPolygon = QTransform().translate(pt.x(), pt.y()).rotate(noFitPolygon->getOrbitingAngle()).map(newOrbitingPolygon);
			writePathSvg(newOrbitingPolygon, stream);
		}
		// QPolygonF::iterator it = newStaticPolygon.begin(); stream << "<path d=\"M" << (*it).x() << " " << (*it).y() << " "; for (++it; it != newStaticPolygon.end(); it++) stream << "L" << (*it).x() << " " << (*it).y() << " "; stream << "Z\" />" << endl;
		// it = newOrbitingPolygon.begin(); stream << "<path d=\"M" << (*it).x() << " " << (*it).y() << " "; for (++it; it != newOrbitingPolygon.end(); it++) stream << "L" << (*it).x() << " " << (*it).y() << " "; stream << "Z\" />" << endl;
		stream << "</svg>" << endl;
		file.close();
	}
}

bool preProcessProblem(RASTERPACKING::PackingProblem &problem, PreProcessorParameters &params, QString outputPath, QString clusterInfo)
{
	QTime myTimer;
	qDebug() << "Innerfit polygons rasterization started.";
	myTimer.start();
	int numProcessed = 1;
	std::cout.precision(2);
	for (QList<std::shared_ptr<RASTERPACKING::InnerFitPolygon>>::const_iterator it = problem.cifpbegin(); it != problem.cifpend(); it++, numProcessed++)
	{
		std::shared_ptr<RASTERPACKING::Polygon> curPolygon = (*it)->getPolygon();

		QPoint referencePoint;

		// --> Rasterize polygon
		int width, height, *rasterCurPolygonVec;
		if (params.innerFitEpsilon < 0)
			rasterCurPolygonVec = curPolygon->getRasterImageVector(referencePoint, params.rasterScaleFactor, width, height);
		else
			rasterCurPolygonVec = curPolygon->getRasterBoundingBoxImageVector(referencePoint, params.rasterScaleFactor, params.innerFitEpsilon, width, height);
		QImage rasterCurPolygon = getImageFromVec(rasterCurPolygonVec, width, height);
		rasterCurPolygon.save(outputPath + "/" + curPolygon->getName() + ".png");

		std::shared_ptr<RASTERPACKING::RasterInnerFitPolygon> curRasterIFP(new RASTERPACKING::RasterInnerFitPolygon(*it));
		curRasterIFP->setScale(params.rasterScaleFactor);
		curRasterIFP->setReferencePoint(referencePoint);
		curRasterIFP->setFileName(curPolygon->getName() + ".png");
		problem.addRasterInnerfitPolygon(curRasterIFP);

		qreal progress = (qreal)numProcessed / (qreal)problem.getInnerfitPolygonsCount();
		std::cout << "\r"
				  << "Progress : [" << std::fixed << 100.0 * progress << "%] [";
		int k = 0;
		for (; k < progress * 56; k++)
			std::cout << "#";
		for (; k < 56; k++)
			std::cout << ".";
		std::cout << "]";

		delete[] rasterCurPolygonVec;
	}
	std::cout << std::endl;
	qDebug() << "Innerfit polygons rasterization finished." << problem.getInnerfitPolygonsCount() << "polygons processed in" << myTimer.elapsed() / 1000.0 << "seconds.";

	qDebug() << "Nofit polygons rasterization started.";
	myTimer.start();
	numProcessed = 1;
	std::cout.precision(2);
	QVector<QPair<int, int>> imageSizes;
	QStringList distTransfNames;
	QVector<int *> rasterPolygonVecs;
	for (QList<std::shared_ptr<RASTERPACKING::NoFitPolygon>>::const_iterator it = problem.cnfpbegin(); it != problem.cnfpend(); it++, numProcessed++)
	{
		std::shared_ptr<RASTERPACKING::Polygon> curPolygon = (*it)->getPolygon();

		QPoint referencePoint;

		// --> Rasterize polygon
		int width, height;
		// int *rasterCurPolygonVec = curPolygon->getRasterImageVector(referencePoint, params.rasterScaleFactor, width, height);
		int *rasterCurPolygonVec;
		if (!params.noOverlap)
			rasterCurPolygonVec = curPolygon->getRasterImageVector(referencePoint, params.rasterScaleFactor, width, height);
		else
			rasterCurPolygonVec = curPolygon->getRasterImageVectorWithContour(referencePoint, params.rasterScaleFactor, width, height);
		if (params.saveRaster)
		{
			QImage rasterCurPolygon = getImageFromVec(rasterCurPolygonVec, width, height);
			rasterCurPolygon.save(outputPath + "/r" + curPolygon->getName() + ".png");
		}
		imageSizes.push_back(QPair<int, int>(width, height));
		distTransfNames.push_back(curPolygon->getName() + ".png");
		rasterPolygonVecs.push_back(rasterCurPolygonVec);

		std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> curRasterNFP(new RASTERPACKING::RasterNoFitPolygon(*it));
		curRasterNFP->setScale(params.rasterScaleFactor);
		curRasterNFP->setReferencePoint(referencePoint);
		curRasterNFP->setFileName(curPolygon->getName() + ".png");
		problem.addRasterNofitPolygon(curRasterNFP);

		qreal progress = (qreal)numProcessed / (qreal)problem.getNofitPolygonsCount();
		std::cout << "\r"
				  << "Progress : [" << std::fixed << 100.0 * progress << "%] [";
		int k = 0;
		for (; k < progress * 56; k++)
			std::cout << "#";
		for (; k < 56; k++)
			std::cout << ".";
		std::cout << "]";
	}
	std::cout << std::endl;
	float totalArea = 0;
	std::for_each(imageSizes.begin(), imageSizes.end(), [&totalArea](QPair<int, int> &size)
				  { totalArea += (float)(size.first * size.second); });
	qDebug() << "Nofit polygons rasterization finished." << problem.getNofitPolygonsCount() << "polygons processed in" << myTimer.elapsed() / 1000.0 << "seconds. Total area:" << totalArea;

	qDebug() << "Max distance determination started.";
	myTimer.start();
	numProcessed = 1;
	float maxD = 0;
	for (QVector<int *>::const_iterator it = rasterPolygonVecs.begin(); it != rasterPolygonVecs.end(); it++, numProcessed++)
	{
		int width, height;
		width = imageSizes[numProcessed - 1].first;
		height = imageSizes[numProcessed - 1].second;
		int *rasterCurPolygonVec = *it;

		// --> Determine the distance transform and save temporary result
		float *distTransfCurPolygonVec = new float[height * width];
		cudaError_t cudaStatus = transform(rasterCurPolygonVec, distTransfCurPolygonVec, height, width);
		for (int c = 0; c < height * width; c++)
			if (distTransfCurPolygonVec[c] > maxD)
				maxD = distTransfCurPolygonVec[c];

		qreal progress = (qreal)numProcessed / (qreal)problem.getNofitPolygonsCount();
		std::cout << "\r"
				  << "Progress : [" << std::fixed << 100.0 * progress << "%] [";
		int k = 0;
		for (; k < progress * 56; k++)
			std::cout << "#";
		for (; k < 56; k++)
			std::cout << ".";
		std::cout << "]";

		delete[] distTransfCurPolygonVec;
	}
	std::cout << std::endl;
	qDebug() << "Max distance determination finished." << problem.getNofitPolygonsCount() << "polygons processed in" << myTimer.elapsed() / 1000.0 << "seconds";

	for (QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>>::iterator it = problem.rnfpbegin(); it != problem.rnfpend(); it++)
		(*it)->setMaxD(maxD);
	problem.save(outputPath + QDir::separator() + params.outputXMLName, clusterInfo);

	qDebug() << "Final distance transformation started.";
	myTimer.start();
	numProcessed = 1;
	bool dtok = true;
	for (QVector<int *>::const_iterator it = rasterPolygonVecs.begin(); it != rasterPolygonVecs.end(); it++, numProcessed++)
	{
		int width, height;
		width = imageSizes[numProcessed - 1].first;
		height = imageSizes[numProcessed - 1].second;
		int *rasterCurPolygonVec = *it;

		float *distTransfCurPolygonVec = new float[height * width];
		cudaError_t cudaStatus = transform(rasterCurPolygonVec, distTransfCurPolygonVec, height, width);

		QImage result(width, height, QImage::Format_Indexed8);
		setColormap(result);

		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
			{
				if (qFuzzyCompare((float)(1 + 0.0), (float)(1 + distTransfCurPolygonVec[i * width + j])))
				{
					result.setPixel(j, i, 0);
				}
				else
				{
					// int index = (int)( (distTransfCurPolygonVec[i*width+j]-sqrt(2))*254/(maxD-sqrt(2)) + 1);
					int index = (int)((distTransfCurPolygonVec[i * width + j] - 1) * 254 / (maxD - 1) + 1);
					if (index < 0 || index > 255)
					{
						result.setPixel(j, i, 255);
						dtok = false;
					}
					else
						result.setPixel(j, i, index);
				}
			}

		result.save(outputPath + "/" + distTransfNames.at(numProcessed - 1));

		qreal progress = (qreal)numProcessed / (qreal)problem.getNofitPolygonsCount();
		std::cout << "\r"
				  << "Progress : [" << std::fixed << 100.0 * progress << "%] [";
		int k = 0;
		for (; k < progress * 56; k++)
			std::cout << "#";
		for (; k < 56; k++)
			std::cout << ".";
		std::cout << "]";

		delete[] rasterCurPolygonVec;
		delete[] distTransfCurPolygonVec;
	}
	std::cout << std::endl;
	if (!dtok)
		qWarning() << "Warning: Some polygons were not correctly processed.";
	qDebug() << "Final distance transformation finished." << problem.getNofitPolygonsCount() << "polygons processed in" << myTimer.elapsed() / 1000.0 << "seconds";
	return true;
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	QCoreApplication::setApplicationName("Raster Packing Preprocessor");
	QCoreApplication::setApplicationVersion("0.1");

	// --> Parse command line arguments
	QCommandLineParser parser;
	parser.setApplicationDescription("Raster Voronoi nofit polygon generator.");
	PreProcessorParameters params;
	QString errorMessage;
	switch (parseCommandLine(parser, &params, &errorMessage))
	{
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

	// Transform source and destination paths to absolute
	QFileInfo inputFileInfo(params.inputFilePath);
	params.inputFilePath = inputFileInfo.absoluteFilePath();
	QDir outputDirDir(params.outputDir);
	params.outputDir = outputDirDir.absolutePath();
	QString originalOutputDir = outputDirDir.absolutePath() + QDir::separator() + "original";

	// Check if source and destination paths exist
	if (!QFile(params.inputFilePath).exists())
	{
		qCritical() << "Input file not found.";
		return 1;
	}
	if (!QDir(params.outputDir).exists())
	{
		qWarning() << "Destination folder does not exist. Creating it.";
		QDir(params.outputDir).mkpath(params.outputDir);
	}

	if (params.optionsFile != "")
	{
		if (!QFile(params.optionsFile).exists())
		{
			qWarning() << "Warning: options file not found. Parameters set to default.";
		}
		else
		{
			switch (parseOptionsFile(params.optionsFile, &params, &errorMessage))
			{
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
			QFileInfo optionsFileInfo(params.optionsFile);
			QDir::setCurrent(optionsFileInfo.absolutePath());
		}
	}

	// Check if cluster original destination path exist
	if ((!params.clusterRankings.isEmpty() || !params.clusterFile.isEmpty()) && !QDir(originalOutputDir).exists())
	{
		qWarning() << "Cluster original destination folder does not exist. Creating it.";
		QDir(originalOutputDir).mkpath(originalOutputDir);
	}

	// Check if cluster input files exist
	if (!params.clusterFile.isEmpty())
	{
		QString clusterInfoFileName = params.clusterFile;
		clusterInfoFileName = clusterInfoFileName.left(clusterInfoFileName.length() - 11);
		params.clusterInfoFile = clusterInfoFileName + "_clusterInfo.xml";
		params.clusterFile = QFileInfo(params.clusterFile).absoluteFilePath();
		params.clusterInfoFile = QFileInfo(params.clusterInfoFile).absoluteFilePath();
		if (!QFile(params.clusterFile).exists())
		{
			qCritical() << "Cluster file not found.";
			return 1;
		}
		if (!QFile(params.clusterInfoFile).exists())
		{
			qCritical() << "Cluster informations file not found.";
			return 1;
		}
	}

	bool processCluster = !params.clusterRankings.isEmpty() || !params.clusterFile.isEmpty();
	qDebug() << "Program execution started.";
	QTime myTimer, totalTimer;

	qDebug() << "Loading problem file...";
	RASTERPACKING::PackingProblem problem;
	myTimer.start();
	totalTimer.start();
	problem.load(params.inputFilePath, params.inputFileType, params.puzzleScaleFactor, params.scaleFixFactor);
	if (params.headerFile != "")
		problem.copyHeader(params.headerFile);
	qDebug() << "Problem file read." << problem.getNofitPolygonsCount() << "nofit polygons read in" << myTimer.elapsed() / 1000.0 << "seconds";
	preProcessProblem(problem, params, !processCluster ? params.outputDir : originalOutputDir, "");

	RASTERPACKING::PackingProblem clusterProblem;
	QString clusterInfo;
	if (!params.clusterRankings.isEmpty())
	{
		qDebug() << "Clustering finished problem.";
		qDebug() << "Determining clusters.";
		std::shared_ptr<CLUSTERING::Clusterizator> rasterClusterizator;
		if (params.clusterWeights.isEmpty())
			rasterClusterizator = std::shared_ptr<CLUSTERING::Clusterizator>(new CLUSTERING::Clusterizator(&problem));
		else
			rasterClusterizator = std::shared_ptr<CLUSTERING::Clusterizator>(new CLUSTERING::Clusterizator(&problem, params.clusterWeights[0], params.clusterWeights[1], params.clusterWeights[2]));
		QList<CLUSTERING::Cluster> clusters = rasterClusterizator->getBestClusters(params.clusterRankings);
		for (int i = 0; i < clusters.length(); i++)
		{
			QString clusterFileName = problem.getName() + "_" + clusters[i].noFitPolygon->getFileName().left(clusters[i].noFitPolygon->getFileName().length() - 4) + "_" + QString::number(params.clusterRankings[i] + 1) + "_" + QString::number(clusters[i].clusterValue) + ".svg";
			printCluster(problem, QDir(params.outputDir).filePath(clusterFileName), clusters[i].noFitPolygon, (QList<QPointF>() << clusters[i].orbitingPos));
		}

		qDebug() << "Generating clustered problem.";
		QList<QString> removedPieces;
		QString puzzleString = rasterClusterizator->getClusteredPuzzle(params.inputFilePath, clusters, removedPieces);

		myTimer.start();
		QTextStream puzzleStream(&puzzleString);
		clusterProblem.loadCFREFP(puzzleStream, params.puzzleScaleFactor, params.scaleFixFactor);
		clusterInfo = rasterClusterizator->getClusterInfo(clusterProblem, clusters, params.outputXMLName, removedPieces);
		if (params.headerFile != "")
			problem.copyHeader(params.headerFile);
		qDebug() << "Cluster problem file generated." << clusterProblem.getNofitPolygonsCount() << "nofit polygons read in" << myTimer.elapsed() / 1000.0 << "seconds";

		// Save output files
		QString clusterPuzzleFileName = outputDirDir.filePath(params.clusterPrefix + "_puzzle.txt");
		QFile fileClusterPuzzle(clusterPuzzleFileName);
		if (fileClusterPuzzle.open(QIODevice::WriteOnly))
			QTextStream(&fileClusterPuzzle) << puzzleString, fileClusterPuzzle.close();
		QString clusterInfoFileName = outputDirDir.filePath(params.clusterPrefix + "_clusterInfo.xml");
		QFile fileClusterInfo(clusterInfoFileName);
		if (fileClusterInfo.open(QIODevice::WriteOnly))
			QTextStream(&fileClusterInfo) << clusterInfo, fileClusterInfo.close();
	}
	else if (!params.clusterFile.isEmpty())
	{
		qDebug() << "Clustering finished problem.";
		qDebug() << "Reading clustered problem.";
		// Read puzzle cluster file
		myTimer.start();
		QFile file(params.clusterFile);
		if (!file.open(QIODevice::ReadOnly))
		{
			qWarning() << "Could not load cluster puzzle file. Finishing execution with error.";
			return 1;
		}
		QTextStream puzzleStream(&file);
		clusterProblem.loadCFREFP(puzzleStream, params.puzzleScaleFactor, params.scaleFixFactor);
		file.close();
		qDebug() << "Cluster problem file read." << clusterProblem.getNofitPolygonsCount() << "nofit polygons read in" << myTimer.elapsed() / 1000.0 << "seconds";

		// Read cluster info file
		QFile fileClusterInfo(params.clusterInfoFile);
		if (!fileClusterInfo.open(QIODevice::ReadOnly))
		{
			qWarning() << "Could not load cluster info file. Finishing execution with error.";
			return 1;
		}
		QTextStream infoStream(&fileClusterInfo);
		clusterInfo = infoStream.readAll();
		fileClusterInfo.close();
	}
	if (processCluster)
		// Pre-process cluster problems
		preProcessProblem(clusterProblem, params, params.outputDir, clusterInfo);

	qDebug() << "Program execution finished. Total time:" << totalTimer.elapsed() / 1000.0 << "seconds.";
	return 0;
}
