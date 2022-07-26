#ifndef RASTERINNERFITPOLYGON_H
#define RASTERINNERFITPOLYGON_H
#include <QPoint>
#include <QHash>
#include <memory>

namespace RASTERVORONOIPACKING {
    class RasterInnerFitPolygon
    {
    public:
        RasterInnerFitPolygon();
        RasterInnerFitPolygon(QPoint bl, QPoint tr);

        QPoint getBottomLeft() {return bottomLeft;}
        QPoint getTopRight() {return topRight;}

    private:
        QPoint bottomLeft, topRight;
    };

    class RasterInnerFitPolygonSet
    {
    public:
        RasterInnerFitPolygonSet() {}
        RasterInnerFitPolygonSet(int numberOfOrientations);

        void addRasterInnerFitPolygon(int orbitingPieceId, int orbitingAngleId, std::shared_ptr<RasterInnerFitPolygon> ifp);
        std::shared_ptr<RasterInnerFitPolygon> getRasterNoFitPolygon(int orbitingPieceId, int orbitingAngleId);

    private:
        QHash<int, std::shared_ptr<RasterInnerFitPolygon>> Ifps;
        int numAngles;
    };
}
#endif // RASTERINNERFITPOLYGON_H
