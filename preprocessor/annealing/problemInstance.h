#ifndef	_PROBLEMINSTANCE_
#define _PROBLEMINSTANCE_

#include<QFile>
#include<memory>
#include <QTextStream>

class cShape;
class cContainer;

std::shared_ptr<cRectangularContainer> readProblemInstance(QTextStream &f, std::vector<std::shared_ptr<cShape> > &shapes, float scale = 1.0);
std::shared_ptr<cRectangularContainer> readProblemInstance(FILE *f, std::vector<std::shared_ptr<cShape> > &shapes, float scale=1.0);

#endif	// _PROBLEMINSTANCE_
