#include "rasterpackingsolution.h"
#include "rasterpackingproblem.h"
#include <QXmlStreamWriter>
#include <QFile>

using namespace RASTERVORONOIPACKING;

RasterPackingSolution::RasterPackingSolution()
{
}

RasterPackingSolution::RasterPackingSolution(int numItems)
{
	reset(numItems);
}

void RasterPackingSolution::reset(int numItems) {
	placements.clear();
	for (int i = 0; i < numItems; i++)
		placements.append(RasterItemPlacement());
}

QDebug operator<<(QDebug dbg, const RasterPackingSolution &c)
{
    for(int i = 0; i < c.getNumItems(); i++) {
        dbg.nospace() << "[Item:" << i << ", Pos:(" << c.getPosition(i).x() << "," << c.getPosition(i).y() << "), Angle:" << c.getOrientation(i) << "]\n";
    }

    return dbg.space();
}

bool RasterPackingSolution::save(QString fileName, std::shared_ptr<RasterPackingProblem> problem, qreal length, bool printSeed, uint seed) {
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Error: Cannot create output file" << fileName << ": " << qPrintable(file.errorString());
        return false;
    }

    QXmlStreamWriter stream;
    stream.setDevice(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("solution");
    for(int i = 0; i < placements.size(); i++) {
        stream.writeStartElement("placement");
        stream.writeAttribute("boardNumber", "1");
        stream.writeAttribute("x", QString::number(placements.at(i).getPos().x()/problem->getScale()));
        stream.writeAttribute("y", QString::number(placements.at(i).getPos().y()/problem->getScale()));
        stream.writeAttribute("idboard", problem->getContainerName());
        stream.writeAttribute("idPiece", problem->getItem(i)->getPieceName());
        stream.writeAttribute("angle", QString::number(problem->getItem(i)->getAngleValue(placements.at(i).getOrientation())));
        stream.writeAttribute("mirror", "none");
        stream.writeEndElement(); // placement
    }
    stream.writeStartElement("extraInfo");
    stream.writeTextElement("length", QString::number(length));
    stream.writeTextElement("scale", QString::number(problem->getScale()));
    if(printSeed) stream.writeTextElement("seed", QString::number(seed));
    stream.writeEndElement(); // extraInfo
    stream.writeEndElement(); // solution

    file.close();

    return true;
}

bool RasterPackingSolution::exportToPgf(QString fileName, std::shared_ptr<RasterPackingProblem> problem, int length, int height) {
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly)) return false;
	QTextStream stream(&file);
	stream << "\\begin{tikzpicture}" << endl;
	stream << "\\draw (0,0) -- (" << length / problem->getScale() << ", 0) -- (" << length / problem->getScale() << "," << height / problem->getScale() << ") -- (0," << height / problem->getScale() << ") -- cycle;" << endl;
	for (int i = 0; i < placements.size(); i++) {
		std::shared_ptr<RasterPackingItem> curItem = problem->getItem(i);
		QPolygonF::iterator it = curItem->getPolygon()->begin();
		stream << "\\draw[shift ={(" << placements.at(i).getPos().x() / problem->getScale() << "," << placements.at(i).getPos().y() / problem->getScale() << ")}, rotate=" << problem->getItem(i)->getAngleValue(placements.at(i).getOrientation()) << "] ";
		stream << "(" << (*it).x() << "," << (*it).y() << ")";
		for (it++; it != curItem->getPolygon()->end(); it++) {
			stream << " -- (" << (*it).x() << "," << (*it).y() << ")";
		}
		stream << " -- cycle;" << endl;
	}
	stream << "\\end{tikzpicture}" << endl;
	file.close();
	return true;
}

bool RasterPackingSolution::save(QXmlStreamWriter &stream, std::shared_ptr<RasterPackingProblem> problem, qreal length, bool printSeed, uint seed) {
	stream.writeStartElement("solution");
	for (int i = 0; i < placements.size(); i++) {
		stream.writeStartElement("placement");
		stream.writeAttribute("boardNumber", "1");
		stream.writeAttribute("x", QString::number(placements.at(i).getPos().x() / problem->getScale()));
		stream.writeAttribute("y", QString::number(placements.at(i).getPos().y() / problem->getScale()));
		stream.writeAttribute("idboard", problem->getContainerName());
		stream.writeAttribute("idPiece", problem->getItem(i)->getPieceName());
		stream.writeAttribute("angle", QString::number(problem->getItem(i)->getAngleValue(placements.at(i).getOrientation())));
		stream.writeAttribute("mirror", "none");
		stream.writeEndElement(); // placement
	}
	stream.writeStartElement("extraInfo");
	stream.writeTextElement("length", QString::number(length));
	stream.writeTextElement("scale", QString::number(problem->getScale()));
	if (printSeed) stream.writeTextElement("seed", QString::number(seed));
	stream.writeEndElement(); // extraInfo
	stream.writeEndElement(); // solution

	return true;
}

bool RasterPackingSolution::save(QXmlStreamWriter &stream, std::shared_ptr<RasterPackingProblem> problem, qreal length, qreal height, int iteration, bool printSeed, uint seed) {
	stream.writeStartElement("solution");
	for (int i = 0; i < placements.size(); i++) {
		stream.writeStartElement("placement");
		stream.writeAttribute("boardNumber", "1");
		stream.writeAttribute("x", QString::number(placements.at(i).getPos().x() / problem->getScale()));
		stream.writeAttribute("y", QString::number(placements.at(i).getPos().y() / problem->getScale()));
		stream.writeAttribute("idboard", problem->getContainerName());
		stream.writeAttribute("idPiece", problem->getItem(i)->getPieceName());
		stream.writeAttribute("angle", QString::number(problem->getItem(i)->getAngleValue(placements.at(i).getOrientation())));
		stream.writeAttribute("mirror", "none");
		stream.writeEndElement(); // placement
	}
	stream.writeStartElement("extraInfo");
	stream.writeTextElement("length", QString::number(length));
	stream.writeTextElement("height", QString::number(height));
	stream.writeTextElement("area", QString::number(length*height));
	stream.writeTextElement("iteration", QString::number(iteration));
	stream.writeTextElement("scale", QString::number(problem->getScale()));
	if (printSeed) stream.writeTextElement("seed", QString::number(seed));
	stream.writeEndElement(); // extraInfo
	stream.writeEndElement(); // solution

	return true;
}