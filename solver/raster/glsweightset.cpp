#include "glsweightset.h"
#include "rasterpackingproblem.h"
#include <QImage>
#include <QDebug>
#ifndef CONSOLE
	#include "colormap.h"
#endif

using namespace RASTERVORONOIPACKING;

void GlsWeightSet::init(int numItems) {
    weights.clear();
    for(int itemId1 = 0; itemId1 < numItems; itemId1++)
        for(int itemId2 = 0; itemId2 < numItems; itemId2++)
            if(itemId1 != itemId2)
                addWeight(itemId1, itemId2, 1.0);
}

qreal GlsWeightSet::getWeight(int itemId1, int itemId2) {
    Q_ASSERT_X(itemId1 != itemId2, "GlsWeightSet::getWeigth", "Cannot verify collision with itself");
    return weights[QPair<int,int>(itemId1, itemId2)];
}

void GlsWeightSet::addWeight(int itemId1,int itemId2, qreal weight) {
    Q_ASSERT_X(itemId1 != itemId2, "GlsWeightSet::getWeigth", "Cannot verify collision with itself");
    weights.insert(QPair<int,int>(itemId1, itemId2), weight);
}

void GlsWeightSet::updateWeights(QVector<WeightIncrement> &increments) {
    std::for_each(increments.begin(), increments.end(), [this](WeightIncrement &inc){
        weights[QPair<int,int>(inc.id1, inc.id2)] += inc.value;
    });
}

#ifndef CONSOLE
    QImage GlsWeightSet::getImage(int numItems) {
        QImage image(numItems, numItems, QImage::Format_Indexed8);
		image.fill(0);
        setColormap(image, false);

        qreal minW = 1.0;
        qreal maxW = 1.0;

        for(int itemId1 = 0; itemId1 < numItems; itemId1++)
            for(int itemId2 = 0; itemId2 < numItems; itemId2++)
                if(itemId1 != itemId2) {
                    qreal curW = getWeight(itemId1, itemId2);
                    if(curW < minW) minW = curW;
                    if(curW > maxW) maxW = curW;
                }

		for (int itemId2 = 0; itemId2 < numItems; itemId2++) {
			uchar *imageLine = (uchar *)image.scanLine(itemId2);
			for (int itemId1 = 0; itemId1 < numItems; itemId1++, imageLine++) {
				if (itemId1 != itemId2) {
					qreal curW = getWeight(itemId1, itemId2);
					int index = qRound(255 * (curW - minW) / (maxW - minW));
					//image.setPixel(itemId1, itemId2, index);
					*imageLine = index;
				}
			}
		}
                //else image.setPixel(itemId1, itemId2, 0);
        return image;
    }
#endif

void GlsWeightSet::reset(int numItems) {
    for(int itemId1 = 0; itemId1 < numItems; itemId1++)
        for(int itemId2 = 0; itemId2 < numItems; itemId2++)
            weights[QPair<int,int>(itemId1, itemId2)] = 1.0;
}
