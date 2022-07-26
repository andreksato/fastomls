#include <stdio.h>

#include "cShape.h"
#include "cShapeInlines.h"
#include "cContainer.h"
#include "cRectangularContainer.h"
#include "problemInstance.h"
#include <vector>
#include <iostream>
#include <memory>
#include <string.h>
#include <QTextStream>
using namespace POLYBOOLEAN;
double obsArea;

std::shared_ptr<cRectangularContainer> readProblemInstance(QTextStream &f, std::vector<std::shared_ptr<cShape> > &shapes, float scale) {
	QString buff;
	pbINT x, y;
	int angle;
	GRID2 point;
	GRID2 basePoint;
	GRID2 bottomLeft;
	GRID2 topRight;
	PLINE2 *aux = NULL;
	bool basePointInit = false;
	PAREA **polygons = NULL;
	int *multiplicity = NULL;
	int anglesCount[100];
	int *angles = NULL;
	unsigned int polyCount = 0;
	unsigned int angleTotalCount = 0;
	unsigned int i;
	double area = 0;
	char dot;
	QTextStream buffStream;

	std::shared_ptr<cRectangularContainer> container;

	if (!f.readLineInto(&buff))
		// FIXME: End of file detected
		return nullptr;
	while (!f.atEnd()) {
		// Rectangular Container
		if (buff.contains("REC_CONTAINER")) {
			if (!f.readLineInto(&buff)) break;
			buffStream.setString(&buff); buffStream >> x >> dot >> y;
			if (buffStream.status() == QTextStream::Ok) {
				bottomLeft.x = (pbINT)((float)x*scale);
				bottomLeft.y = (pbINT)((float)y*scale);
			}

			if (!f.readLineInto(&buff)) break;
			buffStream.setString(&buff); buffStream >> x >> dot >> y;
			if (buffStream.status() == QTextStream::Ok) {
				topRight.x = (pbINT)((float)x*scale);
				topRight.y = (pbINT)((float)y*scale);
			}

			container = std::shared_ptr<cRectangularContainer>(new cRectangularContainer(bottomLeft, topRight));
			continue;
		}

		if (buff.contains("POLYGON")) {
			// alloc new polygon
			polyCount++;
			polygons = (PAREA **)realloc(polygons, (polyCount)*sizeof(*polygons));
			multiplicity = (int*)realloc(multiplicity, (polyCount)*sizeof(int));
			polygons[polyCount - 1] = NULL;
			multiplicity[polyCount - 1] = 1;
			if (buff.length()>8) {
				int mult = buff.right(buff.length() - 8).toInt();
				if (mult>0)
					multiplicity[polyCount - 1] = mult;
			}
			if (f.readLineInto(&buff)) {
				basePointInit = false;
		//		while (sscanf(buff, "%d, %d", &x, &y) == 2) {
				buffStream.setString(&buff); buffStream >> x >> dot >> y;
				while (buffStream.status() == QTextStream::Ok) {
					if (!basePointInit) {
						basePoint.x = x;
						basePoint.y = y;
						basePointInit = true;
					}
					point.x = x - basePoint.x;
					point.y = y - basePoint.y;
					point.x = (pbINT)((float)point.x*scale);
					point.y = (pbINT)((float)point.y*scale);
					PLINE2::Incl(&aux, point);
					if(!f.readLineInto(&buff)) break;
					buffStream.setString(&buff); buffStream >> x >> dot >> y;
					while (buffStream.status() == QTextStream::Ok) {
						point.x = x - basePoint.x;
						point.y = y - basePoint.y;
						point.x = (pbINT)((float)point.x*scale);
						point.y = (pbINT)((float)point.y*scale);
						PLINE2::Incl(&aux, point);
						if (!f.readLineInto(&buff)) break;
						buffStream.setString(&buff); buffStream >> x >> dot >> y;
					}
					if (aux) {
						aux->Prepare();
						if (!aux->IsOuter()) // make sure the polygon is outter
							aux->Invert();
						PAREA::InclPline(&polygons[polyCount - 1], aux);
						aux = NULL;
					}
					if (!f.readLineInto(&buff)) break;
					buffStream.setString(&buff); buffStream >> x >> dot >> y;
				}
			}

			// Read Angles
			int anglePolyCount = 0;
			if (buff.contains("ANGLES")) {
				if (f.readLineInto(&buff)) {
					QTextStream buffStream;
					buffStream.setString(&buff); buffStream >> angle;
					while (buffStream.status() == QTextStream::Ok) {
						angleTotalCount++;
						anglePolyCount++;
						angles = (int *)realloc(angles, (angleTotalCount)*sizeof(int));
						angles[angleTotalCount - 1] = angle;
						if (!f.readLineInto(&buff)) break;
						buffStream.setString(&buff); buffStream >> angle;
					}
				}
			}
			anglesCount[polyCount - 1] = anglePolyCount;

			continue;
		}

		if (!f.readLineInto(&buff)) break;
	}

	// Now convert the polygons to shapes

	std::shared_ptr<cPolygon> polyAux;

	int iangles = 0;
	for (i = 0; i<polyCount; i++) {
		polyAux = std::shared_ptr<cPolygon>(new cPolygon(polygons[i]));
		int *anglesAux = (int *)malloc(anglesCount[i] * sizeof(int));
		for (int j = 0; j < anglesCount[i]; j++, iangles++)
			anglesAux[j] = angles[iangles];
		bool d_rotFlag = false;
		if (anglesCount[i] > 0)
			d_rotFlag = true;
		// Add polygon to cache tracking
		//globalCache::addTrackedPolygon(polyAux);
		//		for(int j=0;j<multiplicity[i];j++) {
		shapes.push_back(std::shared_ptr<cShape>(
			new cShape(
			polyAux,
			polyAux->calcArea(), multiplicity[i],
			//					container,
			anglesAux,
			anglesCount[i],
			d_rotFlag)));
		//		}
		free(anglesAux);
	}

	free(polygons);
	free(multiplicity);
	free(angles);

	obsArea = area;
	return container;
	//	return new cAnnealingPackingSolution(container, &shapes.front(), (unsigned int)shapes.size(), params);
}

std::shared_ptr<cRectangularContainer> readProblemInstance(FILE *f, std::vector<std::shared_ptr<cShape> > &shapes, float scale)
{
	char buff[256];
	//int x, y, angle;
	pbINT x, y;
	int angle;
	GRID2 point;
	GRID2 basePoint;
	GRID2 bottomLeft;
	GRID2 topRight;
	PLINE2 *aux=NULL;
	bool basePointInit = false;
	PAREA **polygons = NULL;
	int *multiplicity = NULL;
    int anglesCount[100];
	int *angles = NULL;
	unsigned int polyCount = 0;
	unsigned int angleTotalCount = 0;
	unsigned int i;
	double area = 0;

    std::shared_ptr<cRectangularContainer> container;

	if(fgets(buff, 256, f) == NULL)
		// FIXME: End of file detected
        return nullptr;
	while(!feof(f)) {
        // Rectangular Container
        if(strncmp(buff, "REC_CONTAINER", 13)==0) {

			if(fgets(buff, 256, f) && sscanf(buff, "%d, %d", &x, &y)==2) {
                bottomLeft.x = (pbINT)((float)x*scale);
                bottomLeft.y = (pbINT)((float)y*scale);
			}

			if(fgets(buff, 256, f) && sscanf(buff, "%d, %d", &x, &y)==2) {
                topRight.x = (pbINT)((float)x*scale);
                topRight.y = (pbINT)((float)y*scale);
			}

            // TODO
            container = std::shared_ptr<cRectangularContainer>(new cRectangularContainer(bottomLeft, topRight));
			//std::cout << "Area: " << container->getRefValue() << std::endl;
			continue;
		}

		if(strncmp(buff, "POLYGON", 7)==0) {
			// alloc new polygon
			polyCount++;
			polygons = (PAREA **)realloc(polygons,(polyCount)*sizeof(*polygons));
			multiplicity = (int*)realloc(multiplicity,(polyCount)*sizeof(int));
			polygons[polyCount-1] = NULL;
			multiplicity[polyCount-1] = 1;
			if(strlen(buff)>8) {
				int mult = atoi(buff+8);
				if(mult>0)
					multiplicity[polyCount-1] = mult;
			}
			if(fgets(buff, 256, f)) {
				basePointInit = false;
				while(sscanf(buff, "%d, %d", &x, &y)==2) {
					if(!basePointInit) {
						basePoint.x = x;
						basePoint.y = y;
						basePointInit = true;
					}
					point.x = x - basePoint.x;
					point.y = y - basePoint.y;
                    point.x = (pbINT)((float)point.x*scale);
                    point.y = (pbINT)((float)point.y*scale);
					PLINE2::Incl(&aux, point);
					if(!fgets(buff, 256, f)) break;
					while(sscanf(buff, "%d, %d", &x, &y)==2) {
						point.x = x - basePoint.x;
						point.y = y - basePoint.y;
                        point.x = (pbINT)((float)point.x*scale);
                        point.y = (pbINT)((float)point.y*scale);
						PLINE2::Incl(&aux, point);
						if(!fgets(buff, 256, f)) break;
					} 
					if(aux) {
						aux->Prepare();
						if (!aux->IsOuter()) // make sure the polygon is outter
							aux->Invert();
						PAREA::InclPline(&polygons[polyCount-1], aux);
						aux = NULL;
					}					
					if(!fgets(buff, 256, f)) break;
				}
			}

			// Read Angles
			int anglePolyCount = 0;
			if(strncmp(buff, "ANGLES", 6) == 0) {
				if(fgets(buff, 256, f)) {					
					while(sscanf(buff, "%d", &angle) == 1) {
						angleTotalCount++;
						anglePolyCount++;
						angles = (int *)realloc(angles, (angleTotalCount)*sizeof(int));
						angles[angleTotalCount-1] = angle;
						if(!fgets(buff, 256, f)) break;
					}
				}
			}
			anglesCount[polyCount - 1] = anglePolyCount;

			continue;
		}

		if(!fgets(buff, 256, f)) break;
	}

	// Now convert the polygons to shapes

    std::shared_ptr<cPolygon> polyAux;
	
	int iangles = 0;
	for(i=0;i<polyCount;i++) {
        polyAux = std::shared_ptr<cPolygon>(new cPolygon(polygons[i]));
		int *anglesAux = (int *)malloc(anglesCount[i]*sizeof(int));
		for(int j = 0; j < anglesCount[i]; j++, iangles++)
			anglesAux[j] = angles[iangles];
		bool d_rotFlag = false;
		if(anglesCount[i] > 0)
			d_rotFlag = true;
		// Add polygon to cache tracking
        //globalCache::addTrackedPolygon(polyAux);
//		for(int j=0;j<multiplicity[i];j++) {
            shapes.push_back(std::shared_ptr<cShape>(
				new cShape(
					polyAux, 
                    polyAux->calcArea(), multiplicity[i],
//					container,
					anglesAux, 
					anglesCount[i], 
                    d_rotFlag)));
//		}
		free(anglesAux);
	}

	free(polygons);
	free(multiplicity);
	free(angles);
	
	obsArea = area;
    return container;
//	return new cAnnealingPackingSolution(container, &shapes.front(), (unsigned int)shapes.size(), params);
}
