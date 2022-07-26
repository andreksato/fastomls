#ifndef _CPOLYGON_H_
#define _CPOLYGON_H_

// Forward declarations
class cPolygon;

#include <memory>
#include "cVector.h"
#include "../polybool/polybool.h"
#include "../polybool/pbio.h"

class cPolygon {
	protected:

		static unsigned long newId;
		unsigned long id;

		POLYBOOLEAN::PAREA	*polygon;
		// TEMP
		//int savenum;

		// Boolean operations
		//	Subtraction
        POLYBOOLEAN::PAREA *rawGetSubtraction(const std::shared_ptr<cPolygon> &poly) {

			POLYBOOLEAN::PAREA *result;
			POLYBOOLEAN::PAREA::Boolean(this->polygon, poly->polygon, &result, POLYBOOLEAN::PAREA::SB);
			return result;
		}

		// Union
        POLYBOOLEAN::PAREA *rawGetUnionWith(const std::shared_ptr<cPolygon> &poly) {
			POLYBOOLEAN::PAREA *result;
			POLYBOOLEAN::PAREA::Boolean(this->polygon, poly->polygon, &result, POLYBOOLEAN::PAREA::UN);
			return result;
		}

	public:

		// Raw constructor from a PAREA pointer
		cPolygon(POLYBOOLEAN::PAREA	*poly) {
			this->polygon = poly;
		}
		
		// void constructor
		cPolygon() {
			this->polygon = NULL;
		}

		cPolygon(cVector points[], unsigned int count);

		// Copy constructor
		cPolygon(const cPolygon &p) {
			if(p.polygon!=NULL)
				this->polygon = p.polygon->Copy();
			else
				this->polygon = NULL;
        }

		void getBoundingBox(cVector *bottomLeft, cVector *topRight);

        std::shared_ptr<cPolygon> getTranslated(const cVector &trans) const;
        std::shared_ptr<cPolygon> getRotated(double rot) const;
        std::shared_ptr<cPolygon> getInverse() const;

        std::shared_ptr<cPolygon> getNoFitPolygon(const std::shared_ptr<cPolygon> obstacle) const;
		
        std::shared_ptr<cPolygon> getUnionWith(const std::shared_ptr<cPolygon> &poly){
            return std::shared_ptr<cPolygon>(new cPolygon(this->rawGetUnionWith(poly)));
		}

        std::shared_ptr<cPolygon> getSubtraction(const std::shared_ptr<cPolygon> &poly) {
            return std::shared_ptr<cPolygon>(new cPolygon(this->rawGetSubtraction(poly)));
		}

        std::shared_ptr<cPolygon> getScaled(double scale) const;


		bool isEmpty() const {
			return polygon==NULL;
		}

        void addPolygon(const std::shared_ptr<cPolygon> &poly)
		{
			POLYBOOLEAN::PAREA *result = this->rawGetUnionWith(poly);
			POLYBOOLEAN::PAREA::Del(&this->polygon);
			this->polygon = result;
		}

        void removePolygon(const std::shared_ptr<cPolygon> &poly) {
			POLYBOOLEAN::PAREA *result = this->rawGetSubtraction(poly);
			POLYBOOLEAN::PAREA::Del(&this->polygon);
			this->polygon = result;
		}

        void addNewSubPolygon(const std::shared_ptr<cPolygon> &poly);

		~cPolygon() {
			POLYBOOLEAN::PAREA::Del(&this->polygon);
		}

		double calcArea() const;
		int getMaxX() {
			int maxX = this->polygon->cntr->gMax.x;
			POLYBOOLEAN::PAREA * pa = this->polygon;
			do {
				for (POLYBOOLEAN::PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
					if(pline->gMax.x > maxX)
						maxX = pline->gMax.x;
			}while ((pa = pa->f) != this->polygon);	
			return maxX;
		}
		int getMinX() {
			int minX = this->polygon->cntr->gMin.x;
			POLYBOOLEAN::PAREA * pa = this->polygon;
			do {
				for (POLYBOOLEAN::PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
					if(pline->gMin.x < minX)
						minX = pline->gMin.x;
			}while ((pa = pa->f) != this->polygon);	
			return minX;
		}
		int getMaxY() {
			int maxY = this->polygon->cntr->gMax.y;
			POLYBOOLEAN::PAREA * pa = this->polygon;
			do {
				for (POLYBOOLEAN::PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
					if(pline->gMax.y > maxY)
						maxY = pline->gMax.y;
			}while ((pa = pa->f) != this->polygon);	
			return maxY;
		}
		int getMinY() {
			int minY = this->polygon->cntr->gMin.y;
			POLYBOOLEAN::PAREA * pa = this->polygon;
			do {
				for (POLYBOOLEAN::PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
					if(pline->gMin.y < minY)
						minY = pline->gMin.y;
			}while ((pa = pa->f) != this->polygon);	
			return minY;
		}
		// Todo: Add a true inner data access!
		POLYBOOLEAN::PAREA	*getInnerData() {
			return this->polygon;
		}
};



#endif	// _CPOLYGON_H_ 
