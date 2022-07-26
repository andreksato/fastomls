#ifndef RASTERNOFITPOLYGON_H
#define RASTERNOFITPOLYGON_H
#include "memory"
#include <QHash>
#include <QImage>
#include <QPoint>
#include <vector>

namespace RASTERPACKING {class PackingProblem;}

namespace RASTERVORONOIPACKING {
    class RasterNoFitPolygon {
    public:
//        RasterNoFitPolygon(QImage _image, QPoint _origin, qreal _maxD) : origin(_origin), image(_image) , maxD(_maxD) {}
        RasterNoFitPolygon(QImage _image, QPoint _origin, qreal _maxD) : origin(_origin), maxD(_maxD) {setMatrix(_image);}
		~RasterNoFitPolygon() { delete[] matrix; }

        void setOrigin(QPoint origin) {this->origin = origin;}
//        void setImage(QImage image) {this->image = image;}
        QPoint getOrigin() {return this->origin;}
        int getOriginX() {return this->origin.x();}
        int getOriginY() {return this->origin.y();}
//        QImage getImage() {return this->image;}
        qreal getMaxD() {return this->maxD;}

        void setMatrix(QImage image);
		quint8 getPixel(int i, int j) { return matrix[j*w + i]; }
        //quint8 getPixel(int i, int j) {return matrix[j][i];}
        //int width() {return (int)matrix[0].size();}
        //int height() {return (int)matrix.size();}
		int width() {return w;}
		int height() {return h;}
		QRect boundingBox() { return QRect(-this->origin, QSize(w, h)); }

		int *getMatrix() { return matrix; }
		void setMatrix(int *_matrix) { matrix = _matrix; }

    private:
        QPoint origin;
//        QImage image;
        qreal maxD;
        //std::vector< std::vector<quint8> > matrix;
		int *matrix;
		int w, h;
    };

    class RasterNoFitPolygonSet
    {
    public:
        RasterNoFitPolygonSet();
        RasterNoFitPolygonSet(int numberOfOrientations);

        bool load(RASTERPACKING::PackingProblem &problem);
        void addRasterNoFitPolygon(int staticPieceTypeId, int staticAngleId, int orbitingPieceTypeId, int orbitingAngleId, std::shared_ptr<RasterNoFitPolygon> nfp);
        std::shared_ptr<RasterNoFitPolygon> getRasterNoFitPolygon(int staticPieceTypeId, int staticAngleId, int orbitingPieceTypeId, int orbitingAngleId);
        void eraseRasterNoFitPolygon(int staticPieceTypeId, int staticAngleId, int orbitingPieceTypeId, int orbitingAngleId);
        void clear();

    private:
        QHash<QPair<int,int>, std::shared_ptr<RasterNoFitPolygon>> Nfps;
        int numAngles;
    };
}
#endif // RASTERNOFITPOLYGON_H
