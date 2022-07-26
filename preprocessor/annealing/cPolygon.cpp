#include "cPolygon.h"
#include "../polybool/polybool.h"
#include "math.h"

using namespace POLYBOOLEAN;

#define D2I(x) ((pbINT)(floor(x+.5)))

// Trivial Minkowski convolution algorithm for *CONVEX* polygons
bool MinkoswkiObstructedArea(PAREA *obstacle, PAREA *obj, PAREA **R);

cPolygon::cPolygon(cVector points[], unsigned int count)
{
	PLINE2 *aux=NULL;
	unsigned int i;

	for(i=0;i<count;i++) {
		PLINE2::Incl(&aux, points[i]);
	}
	this->polygon = NULL;
	if(aux->Prepare())
		PAREA::InclPline(&this->polygon, aux);
}

std::shared_ptr<cPolygon> cPolygon::getNoFitPolygon(const std::shared_ptr<cPolygon> obstacle) const
{
	PAREA *result=NULL;
	PAREA *temp;
	
	PAREA *thisIterator;
	PAREA *obstIterator;
	
	// Must iterate through all sub-contours on both... Yeah, quadratic complexity :(

	// For all sub contours on this polygon...
	thisIterator = this->polygon;
	do {
		obstIterator=obstacle->polygon;		
		do {
			// Add new obstructed region to result
			PAREA *aux;			
			MinkoswkiObstructedArea(obstIterator, thisIterator, &aux);
			PAREA::Boolean(result, aux, &temp, PAREA::UN);
			PAREA::Del(&result);
			PAREA::Del(&aux);
			result = temp;
			obstIterator=obstIterator->f;
		} while(obstIterator!=obstacle->polygon);
		thisIterator = thisIterator->f;
	} while(thisIterator!=this->polygon);
	
    return std::shared_ptr<cPolygon>(new cPolygon(result));
}


void cPolygon::getBoundingBox(cVector *bottomLeft, cVector *topRight)
{
	PAREA *thisIterator;
	
	thisIterator = this->polygon;

	*bottomLeft = thisIterator->cntr->gMin;
	*topRight = thisIterator->cntr->gMax;
	
	// Do it for all sub countours 
	
	thisIterator = thisIterator->f;
	while(thisIterator!=this->polygon) {
		if(thisIterator->cntr->gMax.x > topRight->x) 
			topRight->x = thisIterator->cntr->gMax.x;

		if(thisIterator->cntr->gMax.y > topRight->y) 
			topRight->y = thisIterator->cntr->gMax.y;

		if(thisIterator->cntr->gMin.x < bottomLeft->x) 
			bottomLeft->x = thisIterator->cntr->gMin.x;

		if(thisIterator->cntr->gMin.y < bottomLeft->y) 
			bottomLeft->y = thisIterator->cntr->gMin.y;
	
		thisIterator = thisIterator->f;
	}
}

std::shared_ptr<cPolygon> cPolygon::getTranslated(const cVector &trans) const
{
	PAREA *thisIterator;
	PAREA *newPolygon = NULL;
	PLINE2 *aux;
	VNODE2 *vnode;
	int i, n;

	// For all sub contours on this polygon...
	thisIterator = this->polygon;
	do {
		n = thisIterator->cntr->Count;
		vnode = thisIterator->cntr->head;
		aux = NULL;
		for(i=0;i<n;i++) {
			PLINE2::Incl(&aux, vnode->g + trans);
            vnode = vnode->next;
        }
		aux->Prepare();
		PAREA::InclPline(&newPolygon, aux);

		// Modification to translate NFP edges and NFP vertices
		for (const PLINE2 * pline = thisIterator->NFP_lines; pline != NULL; pline = pline->next) {
			vnode = pline->head;
			PLINE2 *p;
			PLINE2::CreateNPFLine(&p,  vnode->g + trans,  vnode->next->g + trans);
			PAREA::InclPNFPline(&newPolygon, p);
		}

		for (const PLINE2 * pline = thisIterator->NFP_nodes; pline != NULL; pline = pline->next) {
			vnode = pline->head;
			PLINE2 *p;
			PLINE2::CreateNPFNode(&p,  vnode->g + trans);
			PAREA::InclPNFPnode(&newPolygon, p);
		}
		// End of Modification to translate NFP edges and NFP vertices

		thisIterator = thisIterator->f;
	} while(thisIterator!=this->polygon);
	
    std::shared_ptr<cPolygon> ans(new cPolygon(newPolygon));
    return ans;
}

std::shared_ptr<cPolygon> cPolygon::getRotated(double theta) const
{
	PAREA *thisIterator;
	PAREA *newPolygon = NULL;
	PLINE2 *aux;
	VNODE2 *vnode;
	GRID2 newPoint;
	double c, s;
	int i, n;

	c = cos(theta);
	s = sin(theta);

	// For all sub contours on this polygon...
	thisIterator = this->polygon;
	do {
		n = thisIterator->cntr->Count;
		vnode = thisIterator->cntr->head;
		aux = NULL;
		for(i=0;i<n;i++) {
			newPoint.x = D2I(vnode->g.x * c - vnode->g.y * s);
			newPoint.y = D2I(vnode->g.x * s + vnode->g.y * c);
			PLINE2::Incl(&aux, newPoint);
            vnode = vnode->next;
        }
		aux->Prepare();
		PAREA::InclPline(&newPolygon, aux);
		thisIterator = thisIterator->f;
	} while(thisIterator!=this->polygon);
	
    std::shared_ptr<cPolygon> ans(new cPolygon(newPolygon));
    return ans;
}


std::shared_ptr<cPolygon> cPolygon::getScaled(double scale) const
{
	PAREA *thisIterator;
	PAREA *newPolygon = NULL;
	PLINE2 *aux;
	VNODE2 *vnode;
	GRID2 newPoint;
	int i, n;


	// For all sub contours on this polygon...
	thisIterator = this->polygon;
	do {
		n = thisIterator->cntr->Count;
		vnode = thisIterator->cntr->head;
		aux = NULL;
		for(i=0;i<n;i++) {
			newPoint.x = D2I(vnode->g.x * scale);
			newPoint.y = D2I(vnode->g.y * scale);
			PLINE2::Incl(&aux, newPoint);
            vnode = vnode->next;
        }
		aux->Prepare();

		PAREA::InclPline(&newPolygon, aux);
		thisIterator = thisIterator->f;
	} while(thisIterator!=this->polygon);
	
    std::shared_ptr<cPolygon> ans(new cPolygon(newPolygon));
    return ans;
}

std::shared_ptr<cPolygon> cPolygon::getInverse() const
{
	PAREA *thisIterator;
	PAREA *newPolygon = NULL;
	PLINE2 *aux;
	VNODE2 *vnode;
	GRID2 newPoint;
	int i, n;


	// For all sub contours on this polygon...
	thisIterator = this->polygon;
	do {
		n = thisIterator->cntr->Count;
		vnode = thisIterator->cntr->head;
		aux = NULL;
		for(i=0;i<n;i++) {
			newPoint.x = -vnode->g.x;
			newPoint.y = -vnode->g.y;
			PLINE2::Incl(&aux, newPoint);
            vnode = vnode->next;
        }
		aux->Prepare();

		PAREA::InclPline(&newPolygon, aux);
		thisIterator = thisIterator->f;
	} while(thisIterator!=this->polygon);
	
    std::shared_ptr<cPolygon> ans(new cPolygon(newPolygon));
    return ans;
}

void cPolygon::addNewSubPolygon(const std::shared_ptr<cPolygon> &poly)
{
	PAREA::InclParea(&this->polygon, poly->polygon->Copy());
}

inline bool IsAtLeft(GRID2 v1, GRID2 v2) 
{
	POLYBOOLEAN::pbDINT res = (POLYBOOLEAN::pbDINT)v1.x*v2.y-(POLYBOOLEAN::pbDINT)v2.x*v1.y;
	if(res>0) return false;
	if(res<0) return true;
	// paralel vectors
	return ((POLYBOOLEAN::pbDINT)v1.x*v2.x + (POLYBOOLEAN::pbDINT)v1.y*v1.y < 0);
}

// Assume polygons are CONVEX!!!!!!!!!!!!!
bool MinkoswkiObstructedArea(PAREA *obstacle, PAREA *obj, PAREA **R)
{
	int i, n;	// Main loop control
	
	PLINE2 *obstContour;
	PLINE2 *objContour;

	VNODE2 *v1, *nv1;
	VNODE2 *v2, *nv2;

	GRID2 dv1, dv2;

	// Result contour
	PLINE2 * pline = NULL; 
	
	*R = NULL;
	// Retrieve the obstacle contour
	obstContour = obstacle->cntr;

	// Verify if obstacle is hole-free
	if(obstContour->next)
		return false;

	// Verify if obstacle is counter-clockwise
	if((obstContour->Flags & PLINE2::ORIENT) == PLINE2::INV)
		return false;
	
	// Retrieve the object contour
	objContour = obj->cntr;

	// Verify if obj is hole-free
	if(objContour->next)
		return false;

	// Verify if obstacle is counter-clockwise
	if((objContour->Flags & PLINE2::ORIENT) == PLINE2::INV)
		return false;
	
	// Take the first vertex on Obstacle
	v2 = obstContour->head;
		
	// We retrieve the incident edge on v2
	nv2 = v2->next;
	dv2 = nv2->g - v2->g;
	
	// Now, we try to find the apropriate vertex on the object
	//	to fit. The apropriate vertex is the source of first 
	//	edge at left of the current edge on obstacle
	v1 = objContour->head;
			
	nv1 = v1->next;
	// Remember: All coordinates on ToFit INVERTED
	dv1 = v1->g - nv1->g;
		
	// Edge on ToFit is at right, forward til first edge
	//  at right is found 
	if(!IsAtLeft(dv1, dv2)) {
		do {
			v1 = nv1;
			nv1 = nv1->next;
			dv1 = v1->g - nv1->g;
		} while(!IsAtLeft(dv1, dv2));
	}
	// Edge is at left. However, we don't know for
	//  sure if it is the last edge at right...
	//	backward til an edge at right is found, then return
	else {
		do {
			nv1 = v1;
			v1 = v1->prev;
			dv1 = v1->g - nv1->g;
		} while(IsAtLeft(dv1, dv2));
		// then return one position...
		v1 = nv1;
		nv1 = nv1->next;
		dv1 = v1->g - nv1->g;
	}

	// Origin found: Add the vertex to the dest polygon
	PLINE2::Incl(&pline, v2->g - v1->g);
	
	// Get first edge from obstacle polygon
	v2 = nv2;
	nv2 = nv2->next;
	dv2 = nv2->g - v2->g;
	
	PLINE2::Incl(&pline, v2->g - v1->g);

	n = objContour->Count + obstContour->Count;
	for(i=2;i<n;i++) {
		// Get edge from Obstacle polygon
		if(IsAtLeft(dv1, dv2)) {
			v2 = nv2;
			nv2 = nv2->next;
			dv2 = nv2->g - v2->g;
		}
		// Get edge from Obstacle polygon
		else {
			v1 = nv1;
			nv1 = nv1->next;
			dv1 = v1->g - nv1->g;
		}
 		PLINE2::Incl(&pline, v2->g - v1->g);
	}
	pline->Prepare();
	PAREA::InclPline(R, pline); 
	
	return true;
}

// FIXME: Won't work with holes!!!!
double cPolygon::calcArea() const
{
	PAREA *thisIterator;
	double totalArea = 0;

	// For all sub contours on this polygon...
	thisIterator = this->polygon;
	do {
		totalArea += thisIterator->cntr->calcArea();
		thisIterator = thisIterator->f;
	} while(thisIterator!=this->polygon);
		
	return totalArea;
}
