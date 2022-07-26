#ifndef _CSHAPEINLINES_H_
#define _CSHAPEINLINES_H_

#include "cShape.h"

inline cShape::cShape(const cPolygon &p, double value, int multiplicity)
{
	this->id = newId++;
    this->originalPolygon = std::shared_ptr<cPolygon>(new cPolygon(p));
    this->scaled = this->originalPolygon;
    this->parameters = std::shared_ptr<cShapeWritableParameters>(new cShapeWritableParameters());
    this->placement = std::shared_ptr<cShapePlacement>(new cShapePlacement());
	this->originalValue = value;
    this->multiplicity = multiplicity;
}

inline cShape::cShape(const cPolygon &p, double value, int multiplicity, int *angles, int anglesCount, bool d_rotFlag)
{
	this->id = newId++;
    this->originalPolygon = std::shared_ptr<cPolygon>(new cPolygon(p));
    this->scaled = this->originalPolygon;
    this->parameters = std::shared_ptr<cShapeWritableParameters>(new cShapeWritableParameters(d_rotFlag));
    this->placement = std::shared_ptr<cShapePlacement>(new cShapePlacement());
	this->originalValue = value;
    this->multiplicity = multiplicity;
	//this->angles = angles;
	// FIXME: Needs better solution to avoid Memory Leak
	for(int i = 0; i < anglesCount; i++)
		this->angles.push_back(angles[i]);

	this->anglesCount = anglesCount;
}

inline cShape::cShape(const std::shared_ptr<cPolygon> &p, double value, int multiplicity, int *angles, int anglesCount, bool d_rotFlag)
{
	this->id = newId++;
	this->originalPolygon = p;
    this->scaled = this->originalPolygon;
    this->parameters = std::shared_ptr<cShapeWritableParameters>(new cShapeWritableParameters(d_rotFlag));
    this->placement = std::shared_ptr<cShapePlacement>(new cShapePlacement());
	this->originalValue = value;
    this->multiplicity = multiplicity;
	//this->angles = angles;
	// FIXME: Needs better solution to avoid Memory Leak
	for(int i = 0; i < anglesCount; i++)
		this->angles.push_back(angles[i]);
	this->anglesCount = anglesCount;
}


// Copy constructor
inline cShape::cShape(const std::shared_ptr<cShape> &shape) :
	// Reference copy	
	originalPolygon(shape->originalPolygon),
	rotated(shape->rotated),
	translated(shape->translated),
	// Value copy
	parameters(new cShapeWritableParameters(*shape->parameters)),
	placement(new cShapePlacement(*shape->placement)),
	originalValue(shape->originalValue),
	id(shape->id),
	angles(shape->angles),
	anglesCount(shape->anglesCount)
{
}

inline const std::shared_ptr<cPolygon> &cShape::getTranslatedPolygon() {
	// FIXME: Uncomment
	//if(this->translated.isNull())
		this->translated = this->getRotatedPolygon()->getTranslated(this->getPlacement()->getTranslation());
	return this->translated;
}

#endif
