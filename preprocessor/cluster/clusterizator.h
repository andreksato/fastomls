#ifndef CLUSTERIZATOR_H
#define CLUSTERIZATOR_H

#include "packingproblem.h"

namespace CLUSTERING {

	struct Cluster {
		Cluster(std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> _noFitPolygon, QPointF _orbitingPos, qreal _clusterValue) : noFitPolygon(_noFitPolygon), orbitingPos(_orbitingPos), clusterValue(_clusterValue) {}
		std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> noFitPolygon;
		QPointF orbitingPos;
		qreal clusterValue;
	};

	class Clusterizator {
	public:
		Clusterizator(RASTERPACKING::PackingProblem *_problem);
		Clusterizator(RASTERPACKING::PackingProblem *_problem, qreal compressW, qreal intersectW, qreal widthW);
		~Clusterizator() {}
		QList<Cluster> getBestClusters(int numClusters);
		QList<Cluster> getBestClusters(QList<int> rankings);
		void getClusteredProblem(RASTERPACKING::PackingProblem &problem, QList<Cluster> &clusters);
		QString getClusteredPuzzle(QString original, QList<Cluster> &clusters, QList<QString> &removedPieces);
		QString getClusterInfo(RASTERPACKING::PackingProblem &clusterProblem, QList<Cluster> &clusters, QString outputXMLName, QList<QString> &removedPieces);

	private:
		void getNextPairNfpList(QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>> &noFitPolygons, QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>>::iterator &curIt);
		std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> getMaximumPairCluster(QList<std::shared_ptr<RASTERPACKING::RasterNoFitPolygon>> noFitPolygons, QPointF &displacement, qreal &value);
		qreal getMaximumClusterPosition(std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> noFitPolygon, QPointF &displacement);
		QList<QPointF> getContourPlacements(std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> noFitPolygon);
		void printCluster(QString fileName, std::shared_ptr<RASTERPACKING::RasterNoFitPolygon> noFitPolygon, QList<QPointF> &displacements);
		qreal getClusterFunction(RASTERPACKING::Polygon &polygon1, RASTERPACKING::Polygon &polygon2, QPointF displacement);
		int checkValidClustering(QList<Cluster> &minClusters, Cluster &candidateCluster);
		bool Clusterizator::checkValidClustering(QList<Cluster> &clusterList);
		void insertNewCluster(QList<Cluster> &minClusters, Cluster &candidateCluster, int numClusters);
		RASTERPACKING::PackingProblem *problem;
		qreal containerWidth, containerHeight;
		qreal weight_compression, weight_intersection, weight_width;

		class SimplifiedPuzzleProblem {
		public:
			SimplifiedPuzzleProblem(QString fileName);
			void save(QString fileName);
			void getPuzzleStream(QTextStream &stream);
			QStringList container;
			QList<unsigned int> multiplicities;
			QList<QStringList> items;
			QList<QStringList> angles;
		};
	};
}

#endif // CLUSTERIZATOR_H