#include "packingproblem.h"
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QXmlStreamReader>
#include <QDebug>
#include <QMap>
#include <QFileInfo>

using namespace RASTERPACKING;

bool IsAtLeft(QPointF &v1, QPointF &v2)
{
    qreal res = v1.x()*v2.y()-v2.x()*v1.y();
    if(qFuzzyCompare ( 1 + 0.0, 1 + res ))
        // paralel vectors
        return (v1.x()*v2.x() + v1.y()*v1.y() < 0);
    if(res>0) return false;
    return true;
}

std::shared_ptr<Polygon> Polygon::getNofitPolygon(std::shared_ptr<Polygon> staticPolygon, std::shared_ptr<Polygon> orbitingPolygon) {
    QPointF v1, nv1, v2, nv2, dv1, dv2;

    int v1Index, nv1Index;
    int v2Index, nv2Index;
    int sCount, oCount;

    std::shared_ptr<Polygon> nofitResult = std::shared_ptr<Polygon>(new Polygon);
    sCount = staticPolygon->size();
    oCount = orbitingPolygon->size();
    // Take the first vertex on Obstacle
    v2 = staticPolygon->at(v2Index = 0);

    // We retrieve the incident edge on v2
    nv2 = staticPolygon->at(nv2Index = (v2Index+1) % sCount);
    dv2 = nv2 - v2;

    // Now, we try to find the apropriate vertex on the object
    //	to fit. The apropriate vertex is the source of first
    //	edge at left of the current edge on obstacle
    v1 = orbitingPolygon->at(v1Index = 0);

    nv1 = orbitingPolygon->at(nv1Index = (v1Index+1) % oCount);
    // Remember: All coordinates on ToFit INVERTED
    dv1 = v1 - nv1;

    // Edge on ToFit is at right, forward til first edge
    //  at right is found
    if(!IsAtLeft(dv1, dv2)) {
        do {
            v1 = nv1; v1Index = nv1Index;
            nv1 = orbitingPolygon->at(nv1Index = (nv1Index+1) % oCount);
            dv1 = v1 - nv1;
        } while(!IsAtLeft(dv1, dv2));
    }
    // Edge is at left. However, we don't know for
    //  sure if it is the last edge at right...
    //	backward til an edge at right is found, then return
    else {
        do {
            nv1 = v1; nv1Index = v1Index;
            if(v1Index == 0) v1Index = oCount;
            v1 = orbitingPolygon->at(v1Index = (v1Index-1) % oCount);
            //            v1 = orbitingPolygon->at(v1Index = (v1Index-1) % oCount);
            dv1 = v1 - nv1;
        } while(IsAtLeft(dv1, dv2));
        // then return one position...
        v1 = nv1; v1Index = nv1Index;
        nv1 = orbitingPolygon->at(nv1Index = (nv1Index+1) % oCount);
        dv1 = v1 - nv1;
    }

    // Origin found: Add the vertex to the dest polygon
    nofitResult->push_back(v2 - v1);

    // Get first edge from obstacle polygon
    v2 = nv2; v2Index = nv2Index;
    nv2 = staticPolygon->at(nv2Index = (nv2Index+1) % sCount);
    dv2 = nv2 - v2;

    nofitResult->push_back(v2 - v1);

    int n = oCount + sCount;
    for(int i=2;i<n;i++) {
        // Get edge from Obstacle polygon
        if(IsAtLeft(dv1, dv2)) {
            v2 = nv2; v2Index = nv2Index;
            nv2 = staticPolygon->at(nv2Index = (nv2Index+1) % sCount);
            dv2 = nv2 - v2;
        }
        // Get edge from Obstacle polygon
        else {
            v1 = nv1; v1Index = nv1Index;
            nv1 = orbitingPolygon->at(nv1Index = (nv1Index+1) % oCount);
            dv1 = v1 - nv1;
        }
        nofitResult->push_back(v2 - v1);
    }

    return nofitResult;
}

QStringList Polygon::getXML() {
    QStringList commands;

    commands.push_back("writeStartElement"); commands.push_back("polygon");
    commands.push_back("writeAttribute"); commands.push_back("id"); commands.push_back(this->name);
    commands.push_back("writeAttribute"); commands.push_back("nVertices"); commands.push_back(QString::number(this->size()));
    commands.push_back("writeStartElement"); commands.push_back("lines");

    qreal xMin, xMax, yMin, yMax;
    const_iterator it = cbegin();
    xMin = (*it).x(); xMax = (*it).x();
    yMin = (*it).y(); yMax = (*it).y();

    for(int i = 0; i < size(); i++) {
        qreal x0, x1, y0, y1;
        x0 = at(i).x();
        y0 = at(i).y();

        if(i+1 == size()) {
            x1 = at(0).x();
            y1 = at(0).y();
        }
        else {
            x1 = at(i+1).x();
            y1 = at(i+1).y();
        }

        if(x0 < xMin) xMin = x0;
        if(x0 > xMax) xMax = x0;
        if(y0 < yMin) yMin = y0;
        if(y0 > yMax) yMax = y0;

        commands.push_back("writeStartElement"); commands.push_back("segment");
        commands.push_back("writeAttribute"); commands.push_back("n"); commands.push_back(QString::number(i+1));
        commands.push_back("writeAttribute"); commands.push_back("x0"); commands.push_back(QString::number(x0));
        commands.push_back("writeAttribute"); commands.push_back("x1"); commands.push_back(QString::number(x1));
        commands.push_back("writeAttribute"); commands.push_back("y0"); commands.push_back(QString::number(y0));
        commands.push_back("writeAttribute"); commands.push_back("y1"); commands.push_back(QString::number(y1));
        commands.push_back("writeEndElement"); // segment
    }

    commands.push_back("writeTextElement"); commands.push_back("xMin"); commands.push_back(QString::number(xMin));
    commands.push_back("writeTextElement"); commands.push_back("xMax"); commands.push_back(QString::number(xMax));
    commands.push_back("writeTextElement"); commands.push_back("yMin"); commands.push_back(QString::number(yMin));
    commands.push_back("writeTextElement"); commands.push_back("yMax"); commands.push_back(QString::number(yMax));
    commands.push_back("writeEndElement"); // lines
    commands.push_back("writeEndElement"); // polygon

    return commands;
}

qreal Polygon::getArea() {
	qreal area = 0.0;
	int i, j = size() - 1;

	for (i = 0; i < size(); i++) {
		area += (at(j).x() + at(i).x())*(at(j).y() - at(i).y());
		j = i;
	}

	return area*.5;
}

QStringList Container::getXML() {
    QStringList commands;

    commands.push_back("writeStartElement"); commands.push_back("piece");
    commands.push_back("writeAttribute"); commands.push_back("id"); commands.push_back(this->name);
    commands.push_back("writeAttribute"); commands.push_back("quantity"); commands.push_back(QString::number(this->multiplicity));
    commands.push_back("writeStartElement"); commands.push_back("component");
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->getPolygon()->getName());
    commands.push_back("writeAttribute"); commands.push_back("type"); commands.push_back("0");
    commands.push_back("writeAttribute"); commands.push_back("xOffset"); commands.push_back("0");
    commands.push_back("writeAttribute"); commands.push_back("yOffset"); commands.push_back("0");
    commands.push_back("writeEndElement"); // component
    commands.push_back("writeEndElement"); // piece

    return commands;
}

QStringList Piece::getXML() {
    QStringList commands;

    commands.push_back("writeStartElement"); commands.push_back("piece");
    commands.push_back("writeAttribute"); commands.push_back("id"); commands.push_back(this->name);
    commands.push_back("writeAttribute"); commands.push_back("quantity"); commands.push_back(QString::number(this->multiplicity));

    commands.push_back("writeStartElement"); commands.push_back("orientation");
    if(this->getOrientationsCount() == 0) {
        commands.push_back("writeStartElement"); commands.push_back("enumeration");
        commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back("0");
        commands.push_back("writeEndElement"); // enumeration
    }
    else {
        for(QVector<unsigned int>::const_iterator it = this->corbegin(); it != this->corend(); it++) {
            commands.push_back("writeStartElement"); commands.push_back("enumeration");
            commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back(QString::number(*it));
            commands.push_back("writeEndElement"); // enumeration
        }
    }
    commands.push_back("writeEndElement"); // orientation

    commands.push_back("writeStartElement"); commands.push_back("component");
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->getPolygon()->getName());
    commands.push_back("writeAttribute"); commands.push_back("type"); commands.push_back("0");
    commands.push_back("writeAttribute"); commands.push_back("xOffset"); commands.push_back("0");
    commands.push_back("writeAttribute"); commands.push_back("yOffset"); commands.push_back("0");
    commands.push_back("writeEndElement"); // component
    commands.push_back("writeEndElement"); // piece

    return commands;
}

QStringList NoFitPolygon::getXML() {
    QStringList commands;

    commands.push_back("writeStartElement"); commands.push_back("nfp");
    commands.push_back("writeStartElement"); commands.push_back("staticPolygon");
    commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back(QString::number(this->staticAngle));
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->staticName);
    commands.push_back("writeAttribute"); commands.push_back("mirror"); commands.push_back("none");
    commands.push_back("writeEndElement"); // staticPolygon
    commands.push_back("writeStartElement"); commands.push_back("orbitingPolygon");
    commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back(QString::number(this->orbitingAngle));
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->orbitingName);
    commands.push_back("writeAttribute"); commands.push_back("mirror"); commands.push_back("none");
    commands.push_back("writeEndElement"); // orbitingPolygon
    commands.push_back("writeStartElement"); commands.push_back("resultingPolygon");
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->name);
    commands.push_back("writeEndElement"); // resultingPolygon
    commands.push_back("writeEndElement"); // nfp

    return commands;
}

QStringList InnerFitPolygon::getXML() {
    QStringList commands;

    commands.push_back("writeStartElement"); commands.push_back("ifp");
    commands.push_back("writeStartElement"); commands.push_back("staticPolygon");
    commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back(QString::number(this->staticAngle));
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->staticName);
    commands.push_back("writeAttribute"); commands.push_back("mirror"); commands.push_back("none");
    commands.push_back("writeEndElement"); // staticPolygon
    commands.push_back("writeStartElement"); commands.push_back("orbitingPolygon");
    commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back(QString::number(this->orbitingAngle));
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->orbitingName);
    commands.push_back("writeAttribute"); commands.push_back("mirror"); commands.push_back("none");
    commands.push_back("writeEndElement"); // orbitingPolygon
    commands.push_back("writeStartElement"); commands.push_back("resultingPolygon");
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->name);
    commands.push_back("writeEndElement"); // resultingPolygon
    commands.push_back("writeEndElement"); // ifp

    return commands;
}

QStringList RasterNoFitPolygon::getXML() {
    QStringList commands;

    commands.push_back("writeStartElement"); commands.push_back("rnfp");
    commands.push_back("writeStartElement"); commands.push_back("staticPolygon");
    commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back(QString::number(this->staticAngle));
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->staticName);
    commands.push_back("writeAttribute"); commands.push_back("mirror"); commands.push_back("none");
    commands.push_back("writeEndElement"); // staticPolygon
    commands.push_back("writeStartElement"); commands.push_back("orbitingPolygon");
    commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back(QString::number(this->orbitingAngle));
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->orbitingName);
    commands.push_back("writeAttribute"); commands.push_back("mirror"); commands.push_back("none");
    commands.push_back("writeEndElement"); // orbitingPolygon
    commands.push_back("writeStartElement"); commands.push_back("resultingImage");
    commands.push_back("writeAttribute"); commands.push_back("path"); commands.push_back(this->fileName);
    commands.push_back("writeAttribute"); commands.push_back("scale"); commands.push_back(QString::number(this->scale));
    commands.push_back("writeAttribute"); commands.push_back("x0"); commands.push_back(QString::number(this->referencePoint.x()));
    commands.push_back("writeAttribute"); commands.push_back("y0"); commands.push_back(QString::number(this->referencePoint.y()));
	commands.push_back("writeAttribute"); commands.push_back("maxD"); commands.push_back(QString::number(this->maxD));
    commands.push_back("writeEndElement"); // resultingPolygon
    commands.push_back("writeEndElement"); // rnfp

    return commands;
}

QStringList RasterInnerFitPolygon::getXML() {
    QStringList commands;

    commands.push_back("writeStartElement"); commands.push_back("rifp");
    commands.push_back("writeStartElement"); commands.push_back("staticPolygon");
    commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back(QString::number(this->staticAngle));
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->staticName);
    commands.push_back("writeAttribute"); commands.push_back("mirror"); commands.push_back("none");
    commands.push_back("writeEndElement"); // staticPolygon
    commands.push_back("writeStartElement"); commands.push_back("orbitingPolygon");
    commands.push_back("writeAttribute"); commands.push_back("angle"); commands.push_back(QString::number(this->orbitingAngle));
    commands.push_back("writeAttribute"); commands.push_back("idPolygon"); commands.push_back(this->orbitingName);
    commands.push_back("writeAttribute"); commands.push_back("mirror"); commands.push_back("none");
    commands.push_back("writeEndElement"); // orbitingPolygon
    commands.push_back("writeStartElement"); commands.push_back("resultingImage");
    commands.push_back("writeAttribute"); commands.push_back("path"); commands.push_back(this->fileName);
    commands.push_back("writeAttribute"); commands.push_back("scale"); commands.push_back(QString::number(this->scale));
    commands.push_back("writeAttribute"); commands.push_back("x0"); commands.push_back(QString::number(this->referencePoint.x()));
    commands.push_back("writeAttribute"); commands.push_back("y0"); commands.push_back(QString::number(this->referencePoint.y()));
    commands.push_back("writeEndElement"); // resultingPolygon
    commands.push_back("writeEndElement"); // rnfp

    return commands;
}

enum ReadStates {NEUTRAL, BOARDS_READING, LOT_READING};

Container::Container(QString _name, int _multiplicity) {
    this->name = _name;
    this->multiplicity = _multiplicity;
}

Piece::Piece(QString _name, int _multiplicity) {
    this->name = _name;
    this->multiplicity = _multiplicity;
}

bool PackingProblem::copyHeader(QString fileName) {
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << "Error: Cannot read file"
                    << ": " << qPrintable(file.errorString());
        return false;
    }

    QXmlStreamReader xml;
    xml.setDevice(&file);
    while (!xml.atEnd()) {
        xml.readNext();

        if(xml.name()=="name" && xml.tokenType() == QXmlStreamReader::StartElement) this->name = xml.readElementText();
        if(xml.name()=="author" && xml.tokenType() == QXmlStreamReader::StartElement) this->author = xml.readElementText();
        if(xml.name()=="date" && xml.tokenType() == QXmlStreamReader::StartElement) this->date = xml.readElementText();
        if(xml.name()=="description" && xml.tokenType() == QXmlStreamReader::StartElement) this->description = xml.readElementText();
    }

    if (xml.hasError()) {
        // do error handling
    }

    file.close();
    return true;
}

bool PackingProblem::load(QString fileName) {
	folder = QFileInfo(fileName).absolutePath();
    QFile file(fileName);
     if (!file.open(QFile::ReadOnly | QFile::Text)) {
         qCritical() << "Error: Cannot read file"
                     << ": " << qPrintable(file.errorString());
         return false;
     }

     QXmlStreamReader xml;

     std::shared_ptr<Piece> curPiece;
     std::shared_ptr<Container> curContainer;
     QMap<QString, std::shared_ptr<PackingComponent>> pieceNameMap;
     QMap<QString, std::shared_ptr<Polygon> > polygonsTempSet;
     std::shared_ptr<Polygon> curPolygon;
     std::shared_ptr<GeometricTool> curGeometricTool;
     //    PLINE2 *curContour = NULL;
     ReadStates curState = NEUTRAL;
	 maxLength = 0, maxWidth = 0;
	 QRectF itemBoundingBox;

     xml.setDevice(&file);
     while (!xml.atEnd()) {
         xml.readNext();

         if(xml.name()=="name" && xml.tokenType() == QXmlStreamReader::StartElement) this->setName(xml.readElementText());
         if(xml.name()=="author" && xml.tokenType() == QXmlStreamReader::StartElement) this->setAuthor(xml.readElementText());
         if(xml.name()=="date" && xml.tokenType() == QXmlStreamReader::StartElement) this->setDate(xml.readElementText());
         if(xml.name()=="description" && xml.tokenType() == QXmlStreamReader::StartElement) this->setDescription(xml.readElementText());

         // Determine state
         if(xml.name()=="boards" && xml.tokenType() == QXmlStreamReader::StartElement) curState = BOARDS_READING;
         if(xml.name()=="lot" && xml.tokenType() == QXmlStreamReader::StartElement) curState = LOT_READING;
         if( (xml.name()=="boards" || xml.name()=="lot") && xml.tokenType() == QXmlStreamReader::EndElement) curState = NEUTRAL;

         // Read new piece
         if(xml.name()=="piece" && xml.tokenType() == QXmlStreamReader::StartElement) {
             if(curState == BOARDS_READING) {
                 curContainer = std::shared_ptr<Container>(new Container(xml.attributes().value("id").toString(), xml.attributes().value("quantity").toInt()));
                 this->addContainer(curContainer);
             }
             if(curState == LOT_READING) {
                 curPiece = std::shared_ptr<Piece>(new Piece(xml.attributes().value("id").toString(), xml.attributes().value("quantity").toInt()));
                 this->addPiece(curPiece);
             }
         }

         // Add piece general information
         if(xml.name()=="component" && xml.tokenType() == QXmlStreamReader::StartElement) {
             if(curState == BOARDS_READING)
                 pieceNameMap.insert(xml.attributes().value("idPolygon").toString(), curContainer);
             if(curState == LOT_READING)
                 pieceNameMap.insert(xml.attributes().value("idPolygon").toString(), curPiece);
         }

         // Add piece orientation
         if(xml.name()=="enumeration" && xml.tokenType() == QXmlStreamReader::StartElement) {
             curPiece->addOrientation(xml.attributes().value("angle").toInt());
         }

         // Read new polygon
         if(xml.name()=="polygon" && xml.tokenType() == QXmlStreamReader::StartElement) {
             // TODO
             curPolygon = std::shared_ptr<Polygon>(new Polygon(xml.attributes().value("id").toString()));
             // Associate with corresponding piece
             QString polygonName = curPolygon->getName(); //xml.attributes().value("id").toString();
             QMap<QString, std::shared_ptr<PackingComponent>>::iterator it;
             if( (it = pieceNameMap.find(polygonName)) != pieceNameMap.end()) {
                 (*it)->setPolygon(curPolygon);
             }
             else polygonsTempSet.insert(curPolygon->getName(), curPolygon);
         }

         // Add vertex to contour
         if(xml.name()=="segment" && xml.tokenType() == QXmlStreamReader::StartElement) {
             QPointF vertex;
             vertex.setX(xml.attributes().value("x0").toDouble());
             vertex.setY(xml.attributes().value("y0").toDouble());
			 //// Update bounding box
			 //if (curPolygon->isEmpty()) { itemBoundingBox.setBottomLeft(vertex); itemBoundingBox.setTopRight(vertex); }
			 //else {
				// if (vertex.x() < itemBoundingBox.left()) itemBoundingBox.setLeft(vertex.x());
				// if (vertex.x() > itemBoundingBox.right()) itemBoundingBox.setRight(vertex.x());
				// if (vertex.y() < itemBoundingBox.top()) itemBoundingBox.setTop(vertex.y());
				// if (vertex.y() > itemBoundingBox.bottom()) itemBoundingBox.setBottom(vertex.y());
			 //}
			 // Insert vertex into polygon
             curPolygon->push_back(vertex);
         }

		 //// Process finished polygon
		 //if (xml.name() == "polygon" && xml.tokenType() == QXmlStreamReader::EndElement) {
			// if(itemBoundingBox.width() > maxLength) maxLength = itemBoundingBox.width();
			// if(itemBoundingBox.height() > maxWidth) maxWidth = itemBoundingBox.height();
		 //}

		 // Add Bounding Box Information
		 if (xml.name() == "xMin" && xml.tokenType() == QXmlStreamReader::StartElement) curPolygon->setBoundingBoxMinX(xml.readElementText().toInt());
		 if (xml.name() == "xMax" && xml.tokenType() == QXmlStreamReader::StartElement) curPolygon->setBoundingBoxMaxX(xml.readElementText().toInt());
		 if (xml.name() == "yMin" && xml.tokenType() == QXmlStreamReader::StartElement) curPolygon->setBoundingBoxMinY(xml.readElementText().toInt());
		 if (xml.name() == "yMax" && xml.tokenType() == QXmlStreamReader::StartElement) 
			 curPolygon->setBoundingBoxMaxY(xml.readElementText().toInt());

         // Process nofit polygon informations
         if(xml.name()=="nfp" && xml.tokenType() == QXmlStreamReader::StartElement) {
             curGeometricTool = std::shared_ptr<NoFitPolygon>(new NoFitPolygon);
         }
         if(xml.name()=="ifp" && xml.tokenType() == QXmlStreamReader::StartElement) {
             curGeometricTool = std::shared_ptr<InnerFitPolygon>(new InnerFitPolygon);
         }
         if(xml.name()=="staticPolygon" && xml.tokenType() == QXmlStreamReader::StartElement) {
             curGeometricTool->setStaticAngle(xml.attributes().value("angle").toInt());
             curGeometricTool->setStaticName(xml.attributes().value("idPolygon").toString());
         }
         if(xml.name()=="orbitingPolygon" && xml.tokenType() == QXmlStreamReader::StartElement) {
             curGeometricTool->setOrbitingAngle(xml.attributes().value("angle").toInt());
             curGeometricTool->setOrbitingName(xml.attributes().value("idPolygon").toString());
         }
         if(xml.name()=="resultingPolygon" && xml.tokenType() == QXmlStreamReader::StartElement) {
             curGeometricTool->setName(xml.attributes().value("idPolygon").toString());
         }
         if(xml.name()=="nfp" && xml.tokenType() == QXmlStreamReader::EndElement)
             this->addNoFitPolygon(std::static_pointer_cast<NoFitPolygon>(curGeometricTool));
         if(xml.name()=="ifp" && xml.tokenType() == QXmlStreamReader::EndElement)
             this->addInnerFitPolygon(std::static_pointer_cast<InnerFitPolygon>(curGeometricTool));

         // Process raster nofit polygon informations
         if(xml.name()=="rnfp" && xml.tokenType() == QXmlStreamReader::StartElement)
             curGeometricTool = std::shared_ptr<RasterNoFitPolygon>(new RasterNoFitPolygon);
         if(xml.name()=="resultingImage" && xml.tokenType() == QXmlStreamReader::StartElement) {
             std::static_pointer_cast<RasterGeometricTool>(curGeometricTool)->setFileName(xml.attributes().value("path").toString());
             std::static_pointer_cast<RasterGeometricTool>(curGeometricTool)->setScale(xml.attributes().value("scale").toFloat());
             std::static_pointer_cast<RasterGeometricTool>(curGeometricTool)->setMaxD(xml.attributes().value("maxD").toFloat());
             std::static_pointer_cast<RasterGeometricTool>(curGeometricTool)->setReferencePoint(QPoint(xml.attributes().value("x0").toInt(),xml.attributes().value("y0").toInt()));
         }
         if(xml.name()=="rnfp" && xml.tokenType() == QXmlStreamReader::EndElement)
             this->addRasterNofitPolygon(std::static_pointer_cast<RasterNoFitPolygon>(curGeometricTool));

         // Process raster innerfit polygon informations
         if(xml.name()=="rifp" && xml.tokenType() == QXmlStreamReader::StartElement)
             curGeometricTool = std::shared_ptr<RasterInnerFitPolygon>(new RasterInnerFitPolygon);
         if(xml.name()=="rifp" && xml.tokenType() == QXmlStreamReader::EndElement)
             this->addRasterInnerfitPolygon(std::static_pointer_cast<RasterInnerFitPolygon>(curGeometricTool));
     }

     if (xml.hasError()) {
         // do error handling
     }

     for(QList<std::shared_ptr<NoFitPolygon>>::const_iterator it = this->cnfpbegin(); it != this->cnfpend(); it++) {
         QMap<QString, std::shared_ptr<Polygon> >::iterator resultingPolygonIt = polygonsTempSet.find((*it)->getName());
         if( resultingPolygonIt != polygonsTempSet.end())
             (*it)->setPolygon(*resultingPolygonIt);
         else return false; // do error handling
     }

     for(QList<std::shared_ptr<InnerFitPolygon>>::const_iterator it = this->cifpbegin(); it != this->cifpend(); it++) {
         QMap<QString, std::shared_ptr<Polygon> >::iterator resultingPolygonIt = polygonsTempSet.find((*it)->getName());
         if( resultingPolygonIt != polygonsTempSet.end())
             (*it)->setPolygon(*resultingPolygonIt);
         else return false; // do error handling
     }

	 int maxOrientations = 0;
	 // Fing largest dimensions
	 for(auto piece : pieces) {
		 auto polIt = piece->getPolygon()->begin();
		 itemBoundingBox.setBottomLeft(*polIt); 
		 itemBoundingBox.setTopRight(*polIt);
		 for (; polIt != piece->getPolygon()->end(); polIt++) {
			 QPointF vertex = *polIt;
			 if (vertex.x() < itemBoundingBox.left()) itemBoundingBox.setLeft(vertex.x());
			 if (vertex.x() > itemBoundingBox.right()) itemBoundingBox.setRight(vertex.x());
			 if (vertex.y() < itemBoundingBox.top()) itemBoundingBox.setTop(vertex.y());
			 if (vertex.y() > itemBoundingBox.bottom()) itemBoundingBox.setBottom(vertex.y());
		 }
		 if (itemBoundingBox.width() > maxLength) maxLength = itemBoundingBox.width();
		 if (itemBoundingBox.height() > maxWidth) maxWidth = itemBoundingBox.height();
		 maxOrientations = std::max(piece->getOrientationsCount(), maxOrientations);
	 }
	 if (maxOrientations == 4) {
		 maxLength = std::max(maxLength, maxWidth);
		 maxWidth = std::max(maxLength, maxWidth);
	 }

     file.close();
     return true;
}

void processXMLCommands(QStringList &commands, QXmlStreamWriter &stream) {
    for(QStringList::Iterator it = commands.begin(); it != commands.end(); it++) {
        QString curCommand = *it;
        if(curCommand == "writeStartElement")
            stream.writeStartElement(*(++it));
        else if(curCommand == "writeAttribute") {
            QString attName = *(++it);
            stream.writeAttribute(attName, *(++it));
        }
        else if(curCommand == "writeTextElement") {
            QString attName = *(++it);
            stream.writeTextElement(attName, *(++it));
        }
        else if(curCommand == "writeEndElement")
            stream.writeEndElement();
    }
}

bool PackingProblem::save(QString fileName, QString clusterInfo) {
	folder = QFileInfo(fileName).absolutePath();
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Error: Cannot create output file" << fileName << ": " << qPrintable(file.errorString());
        return false;
    }

    QXmlStreamWriter stream;
    stream.setDevice(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("nesting");
    stream.writeAttribute("xmlns", "http://globalnest.fe.up.pt/nesting");
    stream.writeTextElement("name", this->name);
    stream.writeTextElement("author", this->author);
    stream.writeTextElement("date", this->date);
    stream.writeTextElement("description", this->description);
    stream.writeTextElement("verticesOrientation", "clockwise");
    stream.writeTextElement("coordinatesOrigin", "up-left");

    // --> Write problem
    stream.writeStartElement("problem");
    stream.writeStartElement("boards");
    for(QList<std::shared_ptr<Container>>::const_iterator it = this->ccbegin(); it != this->ccend(); it++) {
        QStringList containerCommand = (*it)->getXML();
        processXMLCommands(containerCommand, stream);
    }
    stream.writeEndElement(); // boards
    stream.writeStartElement("lot");
    for(QList<std::shared_ptr<Piece>>::const_iterator it = this->cpbegin(); it != this->cpend(); it++) {
        QStringList pieceCommand = (*it)->getXML();
        processXMLCommands(pieceCommand, stream);
    }
    stream.writeEndElement(); // lot
    stream.writeEndElement(); // problem

    // --> Write polygons
    // FIXME: Duplicated polygons?
    stream.writeStartElement("polygons");
    for(QList<std::shared_ptr<Container>>::const_iterator it = this->ccbegin(); it != this->ccend(); it++) {
        QStringList containerPolCommand = (*it)->getPolygon()->getXML();
        processXMLCommands(containerPolCommand, stream);
    }
    for(QList<std::shared_ptr<Piece>>::const_iterator it = this->cpbegin(); it != this->cpend(); it++) {
        QStringList piecePolCommand = (*it)->getPolygon()->getXML();
        processXMLCommands(piecePolCommand, stream);
    }
    for(QList<std::shared_ptr<NoFitPolygon>>::const_iterator it = this->cnfpbegin(); it != this->cnfpend(); it++) {
        QStringList nfpPolCommand = (*it)->getPolygon()->getXML();
        processXMLCommands(nfpPolCommand, stream);
    }
    for(QList<std::shared_ptr<InnerFitPolygon>>::const_iterator it = this->cifpbegin(); it != this->cifpend(); it++) {
        QStringList ifpPolCommand = (*it)->getPolygon()->getXML();
        processXMLCommands(ifpPolCommand, stream);
    }
    stream.writeEndElement(); // polygons

    // --> Write nfps and ifps
    stream.writeStartElement("nfps");
    for(QList<std::shared_ptr<NoFitPolygon>>::const_iterator it = this->cnfpbegin(); it != this->cnfpend(); it++) {
        QStringList nfpCommand = (*it)->getXML();
        processXMLCommands(nfpCommand, stream);
    }
    stream.writeEndElement(); // nfps
    stream.writeStartElement("ifps");
    for(QList<std::shared_ptr<InnerFitPolygon>>::const_iterator it = this->cifpbegin(); it != this->cifpend(); it++) {
        QStringList ifpCommand = (*it)->getXML();
        processXMLCommands(ifpCommand, stream);
    }
    stream.writeEndElement(); // ifps

    // --> Write raster results
    if(!this->rasterNofitPolygons.empty()) {
        stream.writeStartElement("raster");
        for(QList<std::shared_ptr<RasterNoFitPolygon>>::const_iterator it = this->crnfpbegin(); it != this->crnfpend(); it++) {
            QStringList rnfpCommand = (*it)->getXML();
            processXMLCommands(rnfpCommand, stream);
        }
		for (QList<std::shared_ptr<RasterInnerFitPolygon>>::const_iterator it = this->crifpbegin(); it != this->crifpend(); it++) {
			QStringList rifpCommand = (*it)->getXML();
			processXMLCommands(rifpCommand, stream);
		}
        stream.writeEndElement(); // raster
    }

	// Write cluster info (optional). FIXME: Temporary fix.
	if (clusterInfo != "") saveClusterInfo(stream, clusterInfo);

    stream.writeEndElement(); // nesting
    stream.writeEndDocument();
    file.close();

    return true;
}

qreal PackingProblem::getTotalItemsArea() {
	qreal area = 0.0;
	for (QList<std::shared_ptr<Piece>>::iterator it = pbegin(); it != pend(); it++) {
		area += (*it)->getMultiplicity()*(*it)->getPolygon()->getArea();
	}
	return -area;
}

bool PackingProblem::loadClusterInfo(QString fileName) {
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		qCritical() << "Error: Cannot read file"
			<< ": " << qPrintable(file.errorString());
		return false;
	}

	CLUSTERING::Cluster currentCluster;
	QXmlStreamReader xml;
	xml.setDevice(&file);
	while (!xml.atEnd() && (xml.name() != "clusters" || xml.tokenType() != QXmlStreamReader::StartElement)) xml.readNext();
	originalProblem = QDir(QFileInfo(fileName).path()).filePath(xml.attributes().value("originalProblem").toString());

	QString clusterName;
	while (!xml.atEnd() && (xml.name() != "clusters" || xml.tokenType() != QXmlStreamReader::EndElement)) {
		xml.readNext();
		if (xml.name() == "cluster" && xml.tokenType() == QXmlStreamReader::StartElement) {
			currentCluster.clear();
			clusterName = xml.attributes().value("id").toString();
		}
		if (xml.name() == "cluster" && xml.tokenType() == QXmlStreamReader::EndElement) {
			clusteredPieces.insert(clusterName, currentCluster);
		}

		if (xml.name() == "piece" && xml.tokenType() == QXmlStreamReader::StartElement) {
			currentCluster.push_back(CLUSTERING::ClusterPiece(xml.attributes().value("id").toString(), xml.attributes().value("angle").toInt(), 
			QPointF(xml.attributes().value("xOffset").toFloat(), xml.attributes().value("yOffset").toFloat())));
		}
	}

	if (xml.hasError()) {
		// do error handling
		return false;
	}

	file.close();

	if (clusteredPieces.isEmpty()) return false;
	return true;
}

bool PackingProblem::saveClusterInfo(QXmlStreamWriter &stream, QString clusterInfo) {
	//QFile file(clusterInfoFname);
	//if (!file.open(QFile::ReadOnly | QFile::Text)) {
	//	qCritical() << "Error: Cannot read file"
	//		<< ": " << qPrintable(file.errorString());
	//	return false;
	//}

	CLUSTERING::Cluster currentCluster;
	//QXmlStreamReader xml;
	//xml.setDevice(&file);
	QXmlStreamReader xml(clusterInfo);
	while (!xml.atEnd() && (xml.name() != "clusters" || xml.tokenType() != QXmlStreamReader::StartElement)) xml.readNext();
	stream.writeStartElement("clusters");
	stream.writeAttribute("originalProblem", xml.attributes().value("originalProblem").toString());
	while (!xml.atEnd() && (xml.name() != "clusters" || xml.tokenType() != QXmlStreamReader::EndElement)) {
		xml.readNext();
		if (xml.name() == "cluster" && xml.tokenType() == QXmlStreamReader::StartElement) {
			stream.writeStartElement("cluster");
			stream.writeAttribute("id", xml.attributes().value("id").toString());
		}
		if (xml.name() == "cluster" && xml.tokenType() == QXmlStreamReader::EndElement) stream.writeEndElement(); // cluster

		if (xml.name() == "piece" && xml.tokenType() == QXmlStreamReader::StartElement) {
			stream.writeStartElement("piece");
			stream.writeAttribute("id", xml.attributes().value("id").toString());
			stream.writeAttribute("angle", xml.attributes().value("angle").toString());
			stream.writeAttribute("xOffset", xml.attributes().value("xOffset").toString());
			stream.writeAttribute("yOffset", xml.attributes().value("yOffset").toString());
			stream.writeEndElement(); // piece
		}
	}
	stream.writeEndElement(); // clusters

	if (xml.hasError()) {
		// do error handling
		return false;
	}

	//file.close();

	return true;
}