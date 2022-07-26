#ifndef TOTALOVERLAPMAP_H
#define TOTALOVERLAPMAP_H

#include "rasternofitpolygon.h"
#include "rasterstrippackingparameters.h"
#include <QDebug>

namespace RASTERVORONOIPACKING {

    class TotalOverlapMap
    {
    public:
        TotalOverlapMap(std::shared_ptr<RasterNoFitPolygon> ifp);
        TotalOverlapMap(int width, int height);
		TotalOverlapMap(int width, int height, QPoint _reference);
		TotalOverlapMap(QRect &boundingBox);
		virtual ~TotalOverlapMap() { delete[] data; }

        void init(uint _width, uint _height);
        void reset();

		void setDimensions(int _width, int _height) {
			Q_ASSERT_X(_width > 0 && _height > 0, "TotalOverlapMap::shrink", "Item does not fit the container");
			if (_width > this->width || _height > this->height) {
				// Expanding the map buffer
				delete[] data;
				init(_width, _height);
			}
			this->width = _width; this->height = _height;
		}
		void setWidth(int _width) { setDimensions(_width, this->height); }
		void setRelativeWidth(int deltaPixels) { setWidth(this->originalWidth - deltaPixels); }
		void setRelativeDimensions(int deltaPixelsX, int deltaPixelsY) { setDimensions(this->originalWidth - deltaPixelsX, this->originalHeight - deltaPixelsY); }

        void setReferencePoint(QPoint _ref) {reference = _ref;}
        QPoint getReferencePoint() {return reference;}
        int getWidth() {return width;}
        int getHeight() {return height;}
		const int getOriginalWidth() {return originalWidth;}
		const int getOriginalHeight() { return originalHeight; }
		QRect getRect() { return QRect(-reference, QSize(width, height)); }
        float getValue(const QPoint &pt) {return getLocalValue(pt.x()+reference.x(),pt.y()+reference.y());}
        void setValue(const QPoint &pt, float value) {setLocalValue(pt.x()+reference.x(), pt.y()+reference.y(), value);}

        void addVoronoi(std::shared_ptr<RasterNoFitPolygon> nfp, QPoint pos);
        void addVoronoi(std::shared_ptr<RasterNoFitPolygon> nfp, QPoint pos, float weight);
		void addVoronoi(std::shared_ptr<RasterNoFitPolygon> nfp, QPoint pos, float weight, int zoomFactorInt);
		virtual float getMinimum(QPoint &minPt);

        #ifndef CONSOLE
            QImage getImage(); // For debug purposes
			QImage getZoomImage(int _width, int _height, QPoint &displacement); // For debug purposes
        #endif

		void setData(float *_data) { delete[] data; data = _data; }

    protected:
        float *data;
        int width;
        int height;
		const int originalWidth, originalHeight;

    private:
        float *scanLine(int y);
        float getLocalValue(int i, int j) {return data[j*width+i];}
        void setLocalValue(int i, int j, float value) {data[j*width+i] = value;}
        bool getLimits(QPoint relativeOrigin, int vmWidth, int vmHeight, QRect &intersection);
		bool getLimits(QPoint relativeOrigin, int vmWidth, int vmHeight, QRect &intersection, int zoomFactorInt);

        #ifdef QT_DEBUG
        int initialWidth;
        #endif
        QPoint reference;
    };

    class TotalOverlapMapSet
    {
    public:
        TotalOverlapMapSet();
        TotalOverlapMapSet(int numberOfOrientations);

        void addOverlapMap(int orbitingPieceId, int orbitingAngleId, std::shared_ptr<TotalOverlapMap> ovm);
        std::shared_ptr<TotalOverlapMap> getOverlapMap(int orbitingPieceId, int orbitingAngleId);

        QHash<int, std::shared_ptr<TotalOverlapMap>>::const_iterator cbegin() {return mapSet.cbegin();}
        QHash<int, std::shared_ptr<TotalOverlapMap>>::const_iterator cend() {return mapSet.cend();}

		void setShrinkVal(int val) { this->shrinkValX = val; }
		void setShrinkVal(int valX, int valY) { this->shrinkValX = valX; this->shrinkValY = valY; }
		int getShrinkValX() { return this->shrinkValX; }
		int getShrinkValY() { return this->shrinkValY; }
    private:
        QHash<int, std::shared_ptr<TotalOverlapMap>> mapSet;
		int numAngles, shrinkValX, shrinkValY;
    };

}
#endif // TOTALOVERLAPMAP_H
