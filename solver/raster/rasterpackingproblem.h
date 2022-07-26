#ifndef RASTERPACKINGPROBLEM_H
#define RASTERPACKINGPROBLEM_H
#include <memory>
#include <QVector>
#include "rasternofitpolygon.h"
#include "rasterinnerfitpolygon.h"
#include "raster/rasterpackingsolution.h"
#include <QDebug>
class QString;
namespace RASTERPACKING {class PackingProblem; class RasterNoFitPolygon;}

class MainWindow;

namespace RASTERVORONOIPACKING {
    class RasterPackingItem {
    public:
		RasterPackingItem(unsigned int _id, unsigned int pType, unsigned int aCount, std::shared_ptr<QPolygonF> pol) : id(_id), pieceType(pType), angleCount(aCount), polygon(pol) {}
        ~RasterPackingItem() {}

        void setId(unsigned int _id) {this->id = _id;}
        unsigned int getId() {return this->id;}
        void setPieceType(unsigned int pType) {this->pieceType = pType;}
        unsigned int getPieceType() {return this->pieceType;}
        void setAngleCount(unsigned int aCount) {this->angleCount = aCount;}
        unsigned int getAngleCount() {return this->angleCount;}
		std::shared_ptr<QPolygonF> getPolygon() { return this->polygon; }

        void setPieceName(QString pName) {this->pieceName = pName;}
        QString getPieceName() {return this->pieceName;}
        void addAngleValue(int angle) {this->angleValues.push_back(angle);}
        int getAngleValue(int id) {return this->angleValues.at(id);}
		int getOrientationFromAngle(int angle) { return this->angleValues.indexOf(angle); }
		void setBoundingBox(int _minX, int _maxX, int _minY, int _maxY) {
			this->minX = _minX; this->maxX = _maxX; this->minY = _minY; this->maxY = _maxY;
		}
		void getBoundingBox(int &_minX, int &_maxX, int &_minY, int &_maxY) {
			_minX = this->minX; _maxX = this->maxX; _minY = this->minY; _maxY = this->maxY;
		}
		int getMaxX(int orientation) {
			if (getAngleValue(orientation) == 0) return this->maxX;
			if (getAngleValue(orientation) == 90) return -this->minY;
			if (getAngleValue(orientation) == 180) return -this->minX;
			if (getAngleValue(orientation) == 270) return this->maxY;
			return 0; // FIXME: Implement continuous rotations?
		}
		int getMaxY(int orientation) {
			if (getAngleValue(orientation) == 0) return this->maxY;
			if (getAngleValue(orientation) == 90) return this->maxX;
			if (getAngleValue(orientation) == 180) return -this->minY;
			if (getAngleValue(orientation) == 270) return -this->minX;
			return 0; // FIXME: Implement continuous rotations?
		}

    private:
        unsigned int id;
        unsigned int pieceType;
        unsigned int angleCount;
		int minX, maxX, minY, maxY;

        QString pieceName;
        QVector<int> angleValues;
		std::shared_ptr<QPolygonF> polygon; // For output purposes
    };

    class RasterPackingProblem
    {
    public:
        RasterPackingProblem();
        RasterPackingProblem(RASTERPACKING::PackingProblem &problem);
        ~RasterPackingProblem() {}

    public:
        virtual bool load(RASTERPACKING::PackingProblem &problem);
        std::shared_ptr<RasterPackingItem> getItem(int id) {return items[id];}
		QVector<std::shared_ptr<RasterPackingItem>>::iterator ibegin() { return items.begin(); }
		QVector<std::shared_ptr<RasterPackingItem>>::iterator iend() { return items.end(); }

        std::shared_ptr<RasterNoFitPolygonSet> getIfps() {return innerFitPolygons;}
        std::shared_ptr<RasterNoFitPolygonSet> getNfps() {return noFitPolygons;}
        int count() {return items.size();}
        int getItemType(int id) {return items[id]->getPieceType();}
        int getContainerWidth() {return containerWidth;}
		int getContainerHeight() { return containerHeight; }
		int getMaxWidth() { return this->maxWidth; }
		int getMaxHeight() { return this->maxHeight; }
        QString getContainerName() {return containerName;}
        qreal getScale() {return scale;}

		// Processing of nfp values
		qreal getDistanceValue(int itemId1, QPoint pos1, int orientation1, int itemId2, QPoint pos2, int orientation2);
		bool areOverlapping(int itemId1, QPoint pos1, int orientation1, int itemId2, QPoint pos2, int orientation2);
		void getIfpBoundingBox(int itemId, int orientation, int &bottomLeftX, int &bottomLeftY, int &topRightX, int &topRightY);

    protected:
        int containerWidth, containerHeight;
		int maxWidth, maxHeight;
        QString containerName;
        unsigned int maxOrientations;
        QVector<std::shared_ptr<RasterPackingItem>> items;
        std::shared_ptr<RasterNoFitPolygonSet> noFitPolygons;
        std::shared_ptr<RasterNoFitPolygonSet> innerFitPolygons;
        qreal scale;

	private:
		int getNfpIndexedValue(int itemId1, QPoint pos1, int orientation1, int itemId2, QPoint pos2, int orientation2);
		qreal getNfpValue(int itemId1, QPoint pos1, int orientation1, int itemId2, QPoint pos2, int orientation2, bool &isZero);
    };

	struct RasterPackingClusterItem {
		RasterPackingClusterItem(QString _pieceName, int _id, int _angle, QPoint _offset) : pieceName(_pieceName), id(_id), angle(_angle), offset(_offset) {}
		QString pieceName;
		int id;
		int angle;
		QPoint offset;
	};
	typedef QList<RasterPackingClusterItem> RasterPackingCluster;

	class RasterPackingClusterProblem : public RasterPackingProblem
	{
	friend class MainWindow;
	public:
		RasterPackingClusterProblem() : RasterPackingProblem() {};
		RasterPackingClusterProblem(RASTERPACKING::PackingProblem &problem) : RasterPackingProblem(problem) {};
		~RasterPackingClusterProblem() {}

		bool load(RASTERPACKING::PackingProblem &problem);
		std::shared_ptr<RASTERVORONOIPACKING::RasterPackingProblem> getOriginalProblem() { return this->originalProblem; }
		void convertSolution(RASTERVORONOIPACKING::RasterPackingSolution &solution);

	private:
		QMap<int, RasterPackingCluster> clustersMap;
		std::shared_ptr<RASTERVORONOIPACKING::RasterPackingProblem> originalProblem;
	};
}
#endif // RASTERPACKINGPROBLEM_H
