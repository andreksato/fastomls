#include "clusterizator.h"
#include <algorithm>
#include <QDir>

#define COMPRESS_W 0.899
#define INTERSECT_W 0.1
#define WIDTH_W 0.001

using namespace CLUSTERING;

QPair<qreal, int> getMinimumVal(QList<Cluster> maxClusters) {
	int minValIdx = 0;
	qreal minVal = maxClusters[0].clusterValue;

	if (maxClusters.size() == 1) return QPair<qreal, int>(minVal, minValIdx);
	for (int i = 1; i < maxClusters.size(); i++) {
		qreal curVal = maxClusters[i].clusterValue;
		if (curVal < minVal) minVal = curVal, minValIdx = i;
	}

	return QPair<qreal, int>(minVal, minValIdx);
}

QStringList transformPolygon(QStringList &pol, int angle, QPointF displacement) {
	QPolygonF newPol;
	bool firstPt = true;
	QPointF basePoint;
	QStringList ans;
	for (QStringList::iterator it = pol.begin(); it != pol.end(); it++) {
		if ((*it).isEmpty()) {
			// Adjust basepoint
			newPol = QTransform().translate(-basePoint.x(), -basePoint.y()).map(newPol);
			newPol = QTransform().translate(displacement.x(), displacement.y()).rotate(angle).map(newPol);
			foreach(QPointF pt, newPol) ans << QString::number(qRound(pt.x())) + ", " + QString::number(qRound(pt.y()));
			ans << "";
			newPol.clear();
			continue;
		}
		qreal x, y; char dot;
		QTextStream(&*it) >> x >> dot >> y;
		newPol << QPointF(x, y);
		if (firstPt) basePoint = QPointF(x, y), firstPt = false;
	}
	return ans;
}

Clusterizator::Clusterizator(RASTERPACKING::PackingProblem *_problem) : problem(_problem), weight_compression(COMPRESS_W), weight_intersection(INTERSECT_W), weight_width(WIDTH_W) {
	Clusterizator(_problem, COMPRESS_W, INTERSECT_W, WIDTH_W);
}

Clusterizator::Clusterizator(RASTERPACKING::PackingProblem *_problem, qreal compressW, qreal intersectW, qreal widthW) : problem(_problem), weight_compression(compressW), weight_intersection(intersectW), weight_width(widthW) {
	std::shared_ptr<RASTERPACKING::Container> container = *(problem->ccbegin());
	std::shared_ptr<RASTERPACKING::Polygon> pol = container->getPolygon();
	qreal minX = (*pol->begin()).x(); qreal minY = (*pol->begin()).y();
	qreal maxX = minX; qreal maxY = minY;
	std::for_each(pol->begin(), pol->end(), [&minX, &maxX, &minY, &maxY](QPointF pt){
		if (pt.x() < minX) minX = pt.x();
		if (pt.x() > maxX) maxX = pt.x();
		if (pt.y() < minY) minY = pt.y();
		if (pt.y() > maxY) maxY = pt.y();
	});
	containerWidth = maxX - minX; containerHeight = maxY - minY;
}

QString Clusterizator::getClusteredPuzzle(QString original, QList<Cluster> &clusters, QList<QString> &removedPieces) {
	// Parse original puzzle to obtain convex representation of items
	SimplifiedPuzzleProblem puzzle(original);
	// Create polygon map
	QMap<QString, int> polygonsMap; int i = 0;
	for (QList<std::shared_ptr<RASTERPACKING::Piece>>::const_iterator it = problem->cpbegin(); it != problem->cpend(); it++, i++) polygonsMap.insert((*it)->getPolygon()->getName(), i);
	// Process clustered items
	int clusterid = 0;
	QList<int> itemsToRemove;
	foreach(Cluster curCluster, clusters) {
		// Get Pieces
		QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itStatic = std::find_if(problem->pbegin(), problem->pend(), [&curCluster](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == curCluster.noFitPolygon->getStaticName(); });
		QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itOrbiting = std::find_if(problem->pbegin(), problem->pend(), [&curCluster](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == curCluster.noFitPolygon->getOrbitingName(); });
		// Create new piece
		std::shared_ptr<RASTERPACKING::Piece> curPiece(new RASTERPACKING::Piece("cluster" + QString::number(clusterid++), 1));
		// Determine orientations
		QList<unsigned int> clusterOrientations;
		std::copy((*itStatic)->orbegin(), (*itStatic)->orend(), std::back_inserter(clusterOrientations));
		std::copy_if((*itOrbiting)->orbegin(), (*itOrbiting)->orend(), std::back_inserter(clusterOrientations), [&clusterOrientations](int or) {return std::find(clusterOrientations.cbegin(), clusterOrientations.cend(), or) == clusterOrientations.cend(); });
		qSort(clusterOrientations);
		foreach(int or, clusterOrientations) curPiece->addOrientation(or);
		QStringList newAngles;
		foreach(int or, clusterOrientations) newAngles << QString::number(or);
		// Get polygon
		int staticId = polygonsMap[(*itStatic)->getPolygon()->getName()];
		int orbitingId = polygonsMap[(*itOrbiting)->getPolygon()->getName()];
		QStringList staticPolygonStr = transformPolygon(puzzle.items[staticId], curCluster.noFitPolygon->getStaticAngle(), QPointF(0, 0));
		QStringList orbitingPolygonStr = transformPolygon(puzzle.items[orbitingId], curCluster.noFitPolygon->getOrbitingAngle(), curCluster.orbitingPos);
		// Create new item
		puzzle.items.push_back(QStringList() << staticPolygonStr << orbitingPolygonStr);
		if (newAngles.length() == 1) puzzle.angles.push_back(QStringList()); 
		else puzzle.angles.push_back(newAngles);
		puzzle.multiplicities.push_back(1);
		// Create map to erase old pieces
		puzzle.multiplicities[staticId]--; puzzle.multiplicities[orbitingId]--;
		if (puzzle.multiplicities[staticId] == 0) {
			itemsToRemove.push_back(staticId);
			removedPieces.push_back((*itStatic)->getName());
		}
		if (puzzle.multiplicities[orbitingId] == 0) {
			itemsToRemove.push_back(orbitingId);
			removedPieces.push_back((*itOrbiting)->getName());
		}
	}
	// Remove items
	qSort(itemsToRemove);
	int indexGap = 0;
	QList<int> singleClusterOriginalIds; for (int i = 0; i < puzzle.items.length(); i++) singleClusterOriginalIds << i;
	foreach(int id, itemsToRemove) {
		int gappedId = id - indexGap++;
		puzzle.multiplicities.removeAt(gappedId);
		puzzle.items.removeAt(gappedId);
		puzzle.angles.removeAt(gappedId);
		singleClusterOriginalIds.removeAt(gappedId);
	}
	// Generate string
	QString puzzleString;
	QTextStream puzzleStream(&puzzleString);
	puzzle.getPuzzleStream(puzzleStream);
	return puzzleString;
}

QString Clusterizator::getClusterInfo(RASTERPACKING::PackingProblem &clusterProblem, QList<Cluster> &clusters, QString outputXMLName, QList<QString> &removedPieces) {
	QList<QString> originalPieces;
	for (QList<std::shared_ptr<RASTERPACKING::Piece>>::const_iterator it = problem->cpbegin(); it != problem->cpend(); it++) originalPieces.push_back((*it)->getName());
	int originalPieceCount = originalPieces.length();
	originalPieces.erase(std::remove_if(originalPieces.begin(), originalPieces.end(), [&removedPieces](QString piece) { return removedPieces.contains(piece); }), originalPieces.end());

	// Create clustersInfo file
	QString clusterInfo;
	QXmlStreamWriter stream(&clusterInfo);
	stream.setAutoFormatting(true);
	stream.writeStartElement("clusters");
	QString originalOutputDir = "original"; originalOutputDir = originalOutputDir + "/" + outputXMLName;
	stream.writeAttribute("originalProblem", originalOutputDir);
	QList<std::shared_ptr<RASTERPACKING::Piece>>::const_iterator it = clusterProblem.cpbegin();
	for (int i = 0; i < originalPieceCount - removedPieces.length(); i++) {
		stream.writeStartElement("cluster");
		stream.writeAttribute("id", (*it)->getName());
		stream.writeStartElement("piece");
		QString originalPieceClusterName = originalPieces[i];
		stream.writeAttribute("id", originalPieceClusterName); stream.writeAttribute("angle", "0"); stream.writeAttribute("xOffset", "0"); stream.writeAttribute("yOffset", "0");
		stream.writeEndElement();
		stream.writeEndElement();
		it++;
	}
	foreach(Cluster curCluster, clusters) {
		QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itStatic = std::find_if(problem->pbegin(), problem->pend(), [&curCluster](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == curCluster.noFitPolygon->getStaticName(); });
		QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itOrbiting = std::find_if(problem->pbegin(), problem->pend(), [&curCluster](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == curCluster.noFitPolygon->getOrbitingName(); });
		stream.writeStartElement("cluster");
		stream.writeAttribute("id", (*it)->getName());
		// Write static piece
		stream.writeStartElement("piece");
		stream.writeAttribute("id", (*itStatic)->getName()); stream.writeAttribute("angle", QString::number(curCluster.noFitPolygon->getStaticAngle())); stream.writeAttribute("xOffset", "0"); stream.writeAttribute("yOffset", "0");
		stream.writeEndElement();
		// Write orbiting piece
		stream.writeStartElement("piece");
		stream.writeAttribute("id", (*itOrbiting)->getName()); stream.writeAttribute("angle", QString::number(curCluster.noFitPolygon->getOrbitingAngle())); stream.writeAttribute("xOffset", QString::number(curCluster.orbitingPos.x())); stream.writeAttribute("yOffset", QString::number(curCluster.orbitingPos.y()));
		stream.writeEndElement();
		// Close cluster
		stream.writeEndElement();
		it++;
	}
	stream.writeEndElement();

	return clusterInfo;
}

void Clusterizator::getClusteredProblem(RASTERPACKING::PackingProblem &problem, QList<Cluster> &clusters) {
	// Process clustered items
	int clusterid = 0;
	foreach(Cluster curCluster, clusters) {
		// Get Pieces
		QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itStatic = std::find_if(problem.pbegin(), problem.pend(), [&curCluster](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == curCluster.noFitPolygon->getStaticName(); });
		QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itOrbiting = std::find_if(problem.pbegin(), problem.pend(), [&curCluster](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == curCluster.noFitPolygon->getOrbitingName(); });
		// Create new piece
		std::shared_ptr<RASTERPACKING::Piece> curPiece(new RASTERPACKING::Piece("cluster" + QString::number(clusterid++), 1));
		// Determine orientations
		QList<unsigned int> clusterOrientations;
		std::copy((*itStatic)->orbegin(), (*itStatic)->orend(), std::back_inserter(clusterOrientations));
		std::copy_if((*itOrbiting)->orbegin(), (*itOrbiting)->orend(), std::back_inserter(clusterOrientations), [&clusterOrientations](int or) {return std::find(clusterOrientations.cbegin(), clusterOrientations.cend(), or) == clusterOrientations.cend(); });
		qSort(clusterOrientations);
		foreach(int or, clusterOrientations) curPiece->addOrientation(or);
		curPiece->setPolygon((*itStatic)->getPolygon());
		problem.addPiece(curPiece);
		// Erase old pieces
		if ((*itStatic)->getMultiplicity() == 1) problem.erasep(itStatic);
		else (*itStatic)->setMultiplicity((*itStatic)->getMultiplicity() - 1);
		if ((*itOrbiting)->getMultiplicity() == 1) problem.erasep(itOrbiting);
		else (*itOrbiting)->setMultiplicity((*itOrbiting)->getMultiplicity() - 1);
	}
}

QList<Cluster> Clusterizator::getBestClusters(QList<int> rankings) {
	int numClusters = (*std::max_element(rankings.cbegin(), rankings.cend())) + 1;
	QList<Cluster> bestClusters = getBestClusters(numClusters);
	std::sort(bestClusters.begin(), bestClusters.end(), [](const Cluster & a, const Cluster & b) -> bool {return a.clusterValue > b.clusterValue; });
	QList<Cluster> chosenClusters;
	foreach(int rank, rankings) chosenClusters << bestClusters[rank];
	return chosenClusters;
}

QList<Cluster> Clusterizator::getBestClusters(int numClusters) {
	QList<Cluster> bestClusters;
	QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>>::iterator it = problem->rnfpbegin();
	QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>> pairNoFitPolygons1;
	while (bestClusters.length() < numClusters && it != problem->rnfpend()) {
		pairNoFitPolygons1.clear();
		getNextPairNfpList(pairNoFitPolygons1, it);
		QPointF maxPoint; qreal maxVal;
		std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> nfp = getMaximumPairCluster(pairNoFitPolygons1, maxPoint, maxVal);
		Cluster newCluster(nfp, maxPoint, maxVal);
		//if (checkValidClustering(bestClusters, newCluster)) bestClusters.push_back(newCluster);
		insertNewCluster(bestClusters, newCluster, numClusters);
	}
	QPair<qreal, int> minMaxClusterVal = getMinimumVal(bestClusters);
	while (it != problem->rnfpend()) {
		pairNoFitPolygons1.clear();
		getNextPairNfpList(pairNoFitPolygons1, it);
		QPointF clusterPos;
		qreal clusterVal;
		std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> nfp = getMaximumPairCluster(pairNoFitPolygons1, clusterPos, clusterVal);
		if (clusterVal > minMaxClusterVal.first) {
			//Cluster removedCluster = bestClusters.takeAt(minMaxClusterVal.second);
			//Cluster newCluster(nfp, clusterPos, clusterVal);
			//if (checkValidClustering(bestClusters, newCluster)) bestClusters.push_back(newCluster);
			//else bestClusters.push_back(removedCluster);
			//minMaxClusterVal = getMinimumVal(bestClusters);
			Cluster newCluster(nfp, clusterPos, clusterVal);
			insertNewCluster(bestClusters, newCluster, numClusters);
			minMaxClusterVal = getMinimumVal(bestClusters);
		}
	}

	return QList<Cluster>() << bestClusters;
}

void Clusterizator::getNextPairNfpList(QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>> &noFitPolygons, QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>>::iterator &curIt) {
	QString staticPiece = (*curIt)->getStaticName();
	QString orbitingPiece = (*curIt)->getOrbitingName();

	QString curStaticPiece(staticPiece), curOrbitingPiece(orbitingPiece);
	while (curStaticPiece == staticPiece && curOrbitingPiece == orbitingPiece) {
		noFitPolygons.push_back(*curIt);
		curIt++;
		if (curIt == problem->rnfpend()) break;
		curStaticPiece = (*curIt)->getStaticName();
		curOrbitingPiece = (*curIt)->getOrbitingName();
	}
}

std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> Clusterizator::getMaximumPairCluster(QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>> noFitPolygons, QPointF &displacement, qreal &value) {
	QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>>::iterator it = noFitPolygons.begin();
	QPointF maxPos;
	std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> maxNfp = *it;

	qreal maxVal = getMaximumClusterPosition(*it, maxPos);
	for (; it != noFitPolygons.end(); it++) {
		QPointF curPos;
		qreal curVal = getMaximumClusterPosition(*it, curPos);
		if (curVal > maxVal) {
			maxPos = curPos;
			maxVal = curVal;
			maxNfp = *it;
		}
	}
	displacement.setX(maxPos.x()); displacement.setY(maxPos.y());
	value = maxVal;

	return maxNfp;
}

qreal Clusterizator::getMaximumClusterPosition(std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> noFitPolygon, QPointF &displacement) {
	// Get candidate displacements
	QList<QPointF> candidates = getContourPlacements(noFitPolygon);

	// Get polygons
	QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itStatic = std::find_if(problem->pbegin(), problem->pend(), [noFitPolygon](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == noFitPolygon->getStaticName(); });
	QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator itOrbiting = std::find_if(problem->pbegin(), problem->pend(), [noFitPolygon](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == noFitPolygon->getOrbitingName(); });
	std::shared_ptr<RASTERPACKING::Polygon> staticPolygon = (*itStatic)->getPolygon();
	std::shared_ptr<RASTERPACKING::Polygon> orbitingPolygon = (*itOrbiting)->getPolygon();

	// Convert to qt polygon
	//QPolygonF newStaticPolygon(*staticPolygon);
	//newStaticPolygon = QTransform().rotate(noFitPolygon->getStaticAngle()).map(newStaticPolygon);
	RASTERPACKING::Polygon rpNewStaticPolygon = QTransform().rotate(noFitPolygon->getStaticAngle()).map(*staticPolygon);

	// Test with all orbiting positions
	qreal maxClusterVal = -1.0;
	QPointF maxClusterPt;
	foreach(QPointF pt, candidates) {
		//QPolygonF newOrbitingPolygon; newOrbitingPolygon << *orbitingPolygon;
		//newOrbitingPolygon = QTransform().translate(pt.x(), pt.y()).rotate(noFitPolygon->getOrbitingAngle()).map(newOrbitingPolygon);
		//RASTERPACKING::Polygon rpNewOrbitingPolygon; rpNewOrbitingPolygon << newOrbitingPolygon;
		RASTERPACKING::Polygon rpNewOrbitingPolygon = QTransform().translate(pt.x(), pt.y()).rotate(noFitPolygon->getOrbitingAngle()).map(*orbitingPolygon);
		qreal curVal = getClusterFunction(rpNewStaticPolygon, rpNewOrbitingPolygon, pt);
		if (curVal > maxClusterVal) {
			maxClusterPt = pt;
			maxClusterVal = curVal;
		}
	}

	//printCluster("MinBBAreaCluster.svg",noFitPolygon, (QList<QPointF>() << minCLusterPt));
	displacement.setX(maxClusterPt.x()); displacement.setY(maxClusterPt.y());
	return maxClusterVal;
}

QList<QPointF> Clusterizator::getContourPlacements(std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> noFitPolygon) {
	//qDebug() << "Detrmining grid contour points for no-fi tpolygon:" << noFitPolygon->getName() << "for static item" << noFitPolygon->getStaticName() << "with orientation" << noFitPolygon->getStaticAngle() << " and orbiting item" << noFitPolygon->getOrbitingName() << "with orientation" << noFitPolygon->getOrbitingAngle();
	QString fileName = QDir(problem->getFolder()).filePath(noFitPolygon->getFileName());
	QImage curImage(fileName); 
	//curImage.save("testebefore.png");
	QList<QPoint> dilatedPoints;
	for (int i = 0; i<curImage.width(); i++){
		for (int j = 0; j<curImage.height(); j++){
			if (curImage.pixelIndex(i, j) > 0){
				if (i>0 && curImage.pixelIndex(i - 1, j) == 0) dilatedPoints.push_back(QPoint(i - 1, j));
				if (j>0 && curImage.pixelIndex(i, j - 1) == 0) dilatedPoints.push_back(QPoint(i, j - 1));
				if (i + 1<curImage.width() && curImage.pixelIndex(i + 1, j) == 0) dilatedPoints.push_back(QPoint(i + 1, j));
				if (j + 1<curImage.height() && curImage.pixelIndex(i, j + 1) == 0) dilatedPoints.push_back(QPoint(i, j + 1));
			}
		}
	}
	//curImage.fill(0); foreach(QPoint pt, dilatedPoints) curImage.setPixel(pt, 1); curImage.save("testeafter.png");

	QList<QPointF> realDisplacements;
	foreach(QPoint pt, dilatedPoints) realDisplacements << QPointF((qreal)(pt.x() - noFitPolygon->getReferencePoint().x()) / noFitPolygon->getScale(), (qreal)(pt.y() - noFitPolygon->getReferencePoint().y()) / noFitPolygon->getScale());
	//printCluster(noFitPolygon, realDisplacements);

	return realDisplacements;
}

qreal area(QRectF &rect) {
	return rect.width()*rect.height();
}

qreal Clusterizator::getClusterFunction(RASTERPACKING::Polygon &polygon1, RASTERPACKING::Polygon &polygon2, QPointF displacement) {
	// Bounding Box Area
	QRectF bb1 = polygon1.boundingRect();
	QRectF bb2 = polygon2.boundingRect();
	qreal top = bb1.top() > bb2.top() ? bb2.top() : bb1.top();
	qreal right = bb1.right() > bb2.right() ? bb1.right() : bb2.right();
	qreal bottom = bb1.bottom() > bb2.bottom() ? bb1.bottom() : bb2.bottom();
	qreal left = bb1.left() > bb2.left() ? bb2.left() : bb1.left();
	qreal bbArea = (bottom - top) * (right - left);
	qreal totalArea = qAbs(polygon1.getArea()) + qAbs(polygon2.getArea());
	//qreal compressionRate = bbArea / (area(bb1) + area(bb2));
	qreal compressionRate = totalArea / bbArea;
	// Intersection Area
	QRectF intersection = bb1.intersected(bb2);
	//qreal intArea = -area(intersection)/area(bb1);
	qreal intArea = area(intersection) / (area(bb1) + area(bb2));
	// Width
	//qreal width = (right - left) / (bb1.right() - bb1.left());
	//qreal width = qMin(bb1.left() - bb1.right(), bb2.left() - bb2.right()) / (left - right);
	qreal width = (right - left) / containerHeight;

	return weight_compression*compressionRate + weight_intersection*intArea + weight_width*width;
}

void Clusterizator::insertNewCluster(QList<Cluster> &minClusters, Cluster &candidateCluster, int numClusters) {
	if (minClusters.length() < numClusters) {
		QList<Cluster> currentClusterList(minClusters);
		currentClusterList << candidateCluster;
		if (checkValidClustering(currentClusterList)) {
			minClusters.push_back(candidateCluster);
			return;
		}
	}
	qreal bestDiff = 1.0;
	int exchangedClusterId = -1;
	for (int curClusterId = 0; curClusterId < minClusters.length(); curClusterId++) {
		QList<Cluster> currentClusterList = QList<Cluster>() << candidateCluster;
		for (size_t i = 0; i < minClusters.length(); i++)  {
			if (i != curClusterId) currentClusterList.push_back(minClusters[i]);
		}
		if (!checkValidClustering(currentClusterList)) continue;
		qreal curDiff = minClusters[curClusterId].clusterValue - candidateCluster.clusterValue;
		if (curDiff < 0 && curDiff < bestDiff) {
			bestDiff = curDiff;
			exchangedClusterId = curClusterId;
		}
	}
	if (exchangedClusterId == -1) return;
	// Exchange cluster
	minClusters.removeAt(exchangedClusterId);
	minClusters.push_back(candidateCluster);
}

bool Clusterizator::checkValidClustering(QList<Cluster> &clusterList) {
	QMap<QString, int> pieceCounts;
	foreach(Cluster curCluster, clusterList) {
		QString staticPiece = curCluster.noFitPolygon->getStaticName();
		QString orbitingPiece = curCluster.noFitPolygon->getOrbitingName();
		pieceCounts[staticPiece] = ++pieceCounts[staticPiece];
		pieceCounts[orbitingPiece] = ++pieceCounts[orbitingPiece];
	}
	// Check if multiplicity of pieces
	foreach(QString pieceName, pieceCounts.keys()) {
		QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator it = std::find_if(problem->pbegin(), problem->pend(), [pieceName](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == pieceName; });
		//qDebug() << pieceCounts[pieceName] << "units of piece" << pieceName << ". Multiplicity is" << (*itStatic)->getMultiplicity();
		if (pieceCounts[pieceName] > (*it)->getMultiplicity()) {
			return false;
		}
	}
	return true;
}

//}
//int Clusterizator::checkValidClustering(QList<Cluster> &minClusters, Cluster &candidateCluster) {
//	QMap<QString, int> pieceCounts;
//	QMap<QString, QList<int>> clusterPieces;
//	for (int i = 0; i < minClusters.length(); i++) {
//		Cluster curCluster = minClusters[i];
//	//foreach(Cluster curCluster, minClusters) {
//		QString staticPiece = curCluster.noFitPolygon->getStaticName();
//		QString orbitingPiece = curCluster.noFitPolygon->getOrbitingName();
//		pieceCounts[staticPiece] = ++pieceCounts[staticPiece];
//		pieceCounts[orbitingPiece] = ++pieceCounts[orbitingPiece];
//		clusterPieces[staticPiece].push_back(i); clusterPieces[orbitingPiece].push_back(i);
//		i++;
//	}
//	QString staticPiece = candidateCluster.noFitPolygon->getStaticName();
//	QString orbitingPiece = candidateCluster.noFitPolygon->getOrbitingName();
//	pieceCounts[staticPiece] = ++pieceCounts[staticPiece];
//	pieceCounts[orbitingPiece] = ++pieceCounts[orbitingPiece];
//
//	// Check if multiplicity of pieces
//	foreach(QString pieceName, pieceCounts.keys()) {
//		QList<std::shared_ptr<RASTERPACKING::Piece>>::iterator it = std::find_if(problem->pbegin(), problem->pend(), [pieceName](std::shared_ptr<RASTERPACKING::Piece> s) { return s->getPolygon()->getName() == pieceName; });
//		//qDebug() << pieceCounts[pieceName] << "units of piece" << pieceName << ". Multiplicity is" << (*itStatic)->getMultiplicity();
//		if (pieceCounts[pieceName] > (*it)->getMultiplicity()) {
//			return 0;
//		}
//	}
//
//	return 2;
//}

Clusterizator::SimplifiedPuzzleProblem::SimplifiedPuzzleProblem(QString fileName) {
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QTextStream in(&file);
	QString line;
	// Read container
	while (!line.contains("REC_CONTAINER") && !in.atEnd()) line = in.readLine();
	container << in.readLine(); container << in.readLine(); container << in.readLine();

	// Read items
	while (!line.contains("POLYGON") && !in.atEnd()) line = in.readLine();
	while (!in.atEnd()) {
		// Read multiplicity
		multiplicities << line.replace("POLYGON", "").remove(" ").toInt();

		// Read polygon
		QStringList curPolygon;
		while (!line.contains("ANGLES") && !line.contains("POLYGON") && !in.atEnd()) {
			line = in.readLine();
			if (!line.contains("#") && !line.contains("ANGLES") && !line.contains("POLYGON")) curPolygon << line;
		}
		items << curPolygon;

		// Read angles
		if (line.contains("ANGLES")) {
			QStringList curAngles;
			while (!line.contains("POLYGON") && !in.atEnd()) {
				line = in.readLine();
				if (!line.contains("#") && !line.contains("POLYGON")) curAngles << line;
			}
			if(curAngles.last().isEmpty()) curAngles.pop_back();
			angles << curAngles;
		}
		else angles << QStringList();
	}
	file.close();
}

void Clusterizator::SimplifiedPuzzleProblem::save(QString fileName) {
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

	QTextStream stream(&file);
	getPuzzleStream(stream);
	file.close();
}

void Clusterizator::SimplifiedPuzzleProblem::getPuzzleStream(QTextStream &stream) {
	stream << "//Generated by Raster Packing Pre Processor" << endl << endl << "REC_CONTAINER" << endl;
	for (QStringList::Iterator it = container.begin(); it != container.end(); ++it) stream << *it << endl;

	for (int i = 0; i < multiplicities.length(); i++) {
		stream << "#PIECE " << QString::number(i + 1) << endl;
		stream << "POLYGON " << QString::number(multiplicities[i]) << endl;
		for (QStringList::Iterator it = items[i].begin(); it != items[i].end(); ++it) stream << *it << endl;
		if (!angles[i].isEmpty()) {
			stream << "ANGLES" << endl;
			for (QStringList::Iterator it = angles[i].begin(); it != angles[i].end(); ++it) stream << *it << endl;
			stream << endl;
		}
	}
}