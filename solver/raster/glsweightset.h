#ifndef GLSWEIGHTSET_H
#define GLSWEIGHTSET_H

#include <memory>
#include <QtGlobal>
#include <QHash>
#include <QVector>

class QImage;

namespace RASTERVORONOIPACKING {
    class RasterPackingSolution;
    class RasterPackingProblem;

    struct WeightIncrement {
        WeightIncrement() {}
        WeightIncrement(int _id1, int _id2, qreal _value) {
            id1 = _id1; id2 = _id2; value = _value;
        }
        int id1, id2;
        qreal value;
    };

    class GlsWeightSet
    {
    public:
        GlsWeightSet(int numItems) {init(numItems);}

        void init(int numItems);
        void clear() {weights.clear();}
        void reset(int numItems);
        virtual qreal getWeight(int itemId1, int itemId2);
        void updateWeights(QVector<WeightIncrement> &increments);

        #ifndef CONSOLE
            QImage getImage(int numItems);
        #endif

	protected:
		GlsWeightSet() {}

    private:
        void addWeight(int itemId1, int itemId2, qreal weight);

        QHash<QPair<int,int>, qreal> weights;

    };

	class GlsNoWeightSet : public GlsWeightSet {
	public:
		GlsNoWeightSet() : GlsWeightSet() {}
		qreal getWeight(int itemId1, int itemId2) { return 1.0; }
	};
}

#endif // GLSWEIGHTSET_H
