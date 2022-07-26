#include "cShape.h"
#include "cVector.h"
#include "cRectangularContainer.h"

std::shared_ptr<cPolygon> cRectangularContainer::getInnerFitPolygon(const std::shared_ptr<cPolygon> &shape)
{
    cVector bottomLeft, topRight;
    POLYBOOLEAN::PAREA *newPolygon = NULL;
    POLYBOOLEAN::PLINE2 *aux = NULL;

    shape->getBoundingBox(&bottomLeft, &topRight);

    //unsigned int BoundingBoxWidth = topRight.x - bottomLeft.x;
    //unsigned int BoundingBoxHeight = topRight.y - bottomLeft.y;
    POLYBOOLEAN::pbINT BoundingBoxWidth = topRight.x - bottomLeft.x;
    POLYBOOLEAN::pbINT BoundingBoxHeight = topRight.y - bottomLeft.y;

    if((BoundingBoxWidth==this->width) && (BoundingBoxHeight==this->height)) {
        POLYBOOLEAN::PLINE2::CreateNPFNode(&aux, this->bottomLeft - bottomLeft);
        POLYBOOLEAN::PAREA::InclPNFPnode(&newPolygon, aux);
        return std::shared_ptr<cPolygon>(new cPolygon(newPolygon));
    }

    if(BoundingBoxWidth==this->width) {
        POLYBOOLEAN::PLINE2::CreateNPFLine(&aux, this->bottomLeft - bottomLeft, cVector(
        this->bottomLeft.x - bottomLeft.x, this->topRight.y - topRight.y));
        POLYBOOLEAN::PAREA::InclPNFPline(&newPolygon, aux);
        return std::shared_ptr<cPolygon>(new cPolygon(newPolygon));
    }

    if(BoundingBoxHeight==this->height) {
        POLYBOOLEAN::PLINE2::CreateNPFLine(&aux, this->bottomLeft - bottomLeft, cVector(
        this->topRight.x - topRight.x, this->bottomLeft.y - bottomLeft.y));
        POLYBOOLEAN::PAREA::InclPNFPline(&newPolygon, aux);
        return std::shared_ptr<cPolygon>(new cPolygon(newPolygon));
    }

    if((BoundingBoxWidth > this->width) || (BoundingBoxHeight > this->height))
        return nullptr;


    // Bottom left corner
    POLYBOOLEAN::PLINE2::Incl(&aux, this->bottomLeft - bottomLeft);
    // Bottom right corner
    POLYBOOLEAN::PLINE2::Incl(&aux, cVector(
        this->topRight.x - topRight.x,
        this->bottomLeft.y - bottomLeft.y));
    // Top right corner
    POLYBOOLEAN::PLINE2::Incl(&aux, this->topRight - topRight);
    // Top left corner
    POLYBOOLEAN::PLINE2::Incl(&aux, cVector(
        this->bottomLeft.x - bottomLeft.x,
        this->topRight.y - topRight.y));

    aux->Prepare();
    POLYBOOLEAN::PAREA::InclPline(&newPolygon, aux);
    return std::shared_ptr<cPolygon>(new cPolygon(newPolygon));
}

std::shared_ptr<cPolygon> cRectangularContainer::getShape()
{
    if(this->shape == nullptr) {
		POLYBOOLEAN::PAREA *newPolygon = NULL;
		POLYBOOLEAN::PLINE2 *aux = NULL;

		// Bottom left corner
		POLYBOOLEAN::PLINE2::Incl(&aux, this->bottomLeft);
		// Bottom right corner
		POLYBOOLEAN::PLINE2::Incl(&aux, cVector(
			this->topRight.x, this->bottomLeft.y));
		// Top right corner
		POLYBOOLEAN::PLINE2::Incl(&aux, this->topRight);
		// Top left corner
		POLYBOOLEAN::PLINE2::Incl(&aux, cVector(
			this->bottomLeft.x, this->topRight.y));

		aux->Prepare();
		POLYBOOLEAN::PAREA::InclPline(&newPolygon, aux);

        this->shape = std::shared_ptr<cPolygon>(new cPolygon(newPolygon));
	}
	return this->shape;
}

double cRectangularContainer::getRefValue() const
{
	return this->area;
}
