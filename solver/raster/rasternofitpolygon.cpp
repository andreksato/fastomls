#include "rasternofitpolygon.h"
#include "packingproblem.h"
#include<QImage>
#include<QPoint>
using namespace RASTERVORONOIPACKING;

RasterNoFitPolygonSet::RasterNoFitPolygonSet() {
    numAngles = 4;
}

RasterNoFitPolygonSet::RasterNoFitPolygonSet(int numberOfOrientations) : numAngles(numberOfOrientations) {}

void RasterNoFitPolygonSet::addRasterNoFitPolygon(int staticPieceTypeId, int staticAngleId, int orbitingPieceTypeId, int orbitingAngleId, std::shared_ptr<RasterNoFitPolygon> nfp) {
    int staticKey =  staticPieceTypeId*numAngles + staticAngleId;
    int orbitingKey =  orbitingPieceTypeId*numAngles + orbitingAngleId;
    Nfps.insert(QPair<int,int>(staticKey, orbitingKey), nfp);
}

std::shared_ptr<RasterNoFitPolygon> RasterNoFitPolygonSet::getRasterNoFitPolygon(int staticPieceTypeId, int staticAngleId, int orbitingPieceTypeId, int orbitingAngleId) {
    int staticKey =  staticPieceTypeId*numAngles + staticAngleId;
    int orbitingKey =  orbitingPieceTypeId*numAngles + orbitingAngleId;
    return Nfps[QPair<int,int>(staticKey, orbitingKey)];
}

void RasterNoFitPolygonSet::eraseRasterNoFitPolygon(int staticPieceTypeId, int staticAngleId, int orbitingPieceTypeId, int orbitingAngleId) {
    int staticKey =  staticPieceTypeId*numAngles + staticAngleId;
    int orbitingKey =  orbitingPieceTypeId*numAngles + orbitingAngleId;
    Nfps.remove(QPair<int,int>(staticKey, orbitingKey));
}

void RasterNoFitPolygonSet::clear() {
    Nfps.clear();
}

void RasterNoFitPolygon::setMatrix(QImage image) {
    //matrix = std::vector< std::vector<quint8> >(image.height(), std::vector<quint8>(image.width()));
	matrix = new int[image.width() * image.height()];
	
    for(int j = 0; j < image.height(); j++)
        for(int i = 0; i < image.width(); i++)
			matrix[j*image.width() + i] = image.pixelIndex(i, j);

	w = image.width(); h = image.height();
//    for(int pixelY = 0; pixelY < image.height(); pixelY++) {
//        uchar *scanLine = (uchar *)image.scanLine(pixelY);
//        for(int pixelX = 0; pixelX < image.width(); pixelX++, scanLine++)
//                matrix[pixelY][pixelX] = *scanLine;
//    }
}
