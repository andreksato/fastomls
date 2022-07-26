#ifndef _CCONTAINER_H_
#define _CCONTAINER_H_


// Forward declarations
class cContainer;

#include "cShape.h"
#include "cPolygon.h"
#include <memory>

class cContainer{
	public:
        virtual std::shared_ptr<cPolygon> getShape() = 0;
        virtual std::shared_ptr<cPolygon> getInnerFitPolygon(const std::shared_ptr<cPolygon> &shape) = 0;
		virtual double getRefValue() const = 0;		
};


#endif	// _CCONTAINER_H_
