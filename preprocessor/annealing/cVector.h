#ifndef _CVECTOR_H_
#define _CVECTOR_H_


// Forward declarations
class cVector;

#include "../polybool/polybool.h"

class cVector : public POLYBOOLEAN::GRID2 {
	public:
		cVector() : POLYBOOLEAN::GRID2() {}
		cVector(const POLYBOOLEAN::GRID2 &grid) : POLYBOOLEAN::GRID2(grid) {}
		
		cVector(POLYBOOLEAN::pbINT x, POLYBOOLEAN::pbINT y) {
			this->x = x;
			this->y = y;
		}
		
		cVector operator -(const cVector &v) {
			return cVector((POLYBOOLEAN::GRID2)(*this) - (POLYBOOLEAN::GRID2)v);
		}
};

#endif	// _CVECTOR_H_
