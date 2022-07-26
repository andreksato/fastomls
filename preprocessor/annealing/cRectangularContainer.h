#ifndef _CRECTANGULARCONTAINER_H_
#define _CRECTANGULARCONTAINER_H_

#include "cContainer.h"
#include "cVector.h"
// FIX
#include "../polybool/polybool.h"

class cRectangularContainer : public cContainer {

	protected:

		cVector bottomLeft;
		cVector topRight;
		POLYBOOLEAN::pbINT width;
		POLYBOOLEAN::pbINT height;
        std::shared_ptr<cPolygon> shape;
		double area;

		
	public:
		cRectangularContainer(const cVector &bottomLeft, const cVector &topRight) : 
		  bottomLeft(bottomLeft), topRight(topRight) {
			this->width = topRight.x - bottomLeft.x;
			this->height = topRight.y - bottomLeft.y;
			this->area = (double)(this->width)*(double)(this->height);
		  }

        virtual std::shared_ptr<cPolygon> getInnerFitPolygon(const std::shared_ptr<cPolygon> &shape);
        virtual std::shared_ptr<cPolygon> getShape();
		virtual double getRefValue() const;
};

#endif
