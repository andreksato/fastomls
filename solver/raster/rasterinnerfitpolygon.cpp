#include "rasterinnerfitpolygon.h"
using namespace RASTERVORONOIPACKING;

RasterInnerFitPolygon::RasterInnerFitPolygon()
{
}

RasterInnerFitPolygon::RasterInnerFitPolygon(QPoint bl, QPoint tr)
{
    this->bottomLeft = bl; this->topRight = tr;
}

RasterInnerFitPolygonSet::RasterInnerFitPolygonSet(int numberOfOrientations) : numAngles(numberOfOrientations) {}

void RasterInnerFitPolygonSet::addRasterInnerFitPolygon(int orbitingPieceId, int orbitingAngleId, std::shared_ptr<RasterInnerFitPolygon> ifp) {
    int orbitingKey =  orbitingPieceId*numAngles + orbitingAngleId;
    Ifps.insert(orbitingKey, ifp);
}

std::shared_ptr<RasterInnerFitPolygon> RasterInnerFitPolygonSet::getRasterNoFitPolygon(int orbitingPieceId, int orbitingAngleId) {
    int orbitingKey =  orbitingPieceId*numAngles + orbitingAngleId;
    return Ifps[orbitingKey];
}