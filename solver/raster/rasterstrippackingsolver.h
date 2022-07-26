#ifndef RASTERSTRIPPACKINGSOLVER_H
#define RASTERSTRIPPACKINGSOLVER_H
#include <memory>
#include "rasterpackingproblem.h"
#include "rasteroverlapevaluator.h"
#include "rasterpackingsolution.h"
#include "rasterstrippackingparameters.h"
#include "totaloverlapmap.h"
#include "glsweightset.h"

//class MainWindow;

namespace RASTERVORONOIPACKING {
	enum BottomLeftMode {BL_STRIPPACKING, BL_RECTANGULAR, BL_SQUARE};

	class RasterStripPackingSolver
	{
                friend class ::MainWindow;

	public:
		static std::shared_ptr<RasterStripPackingSolver> createRasterPackingSolver(std::shared_ptr<RasterPackingProblem> problem, RasterStripPackingParameters &parameters, int initialWidth = -1, int initialHeight = -1);

		RasterStripPackingSolver(std::shared_ptr<RasterPackingProblem> _problem, std::shared_ptr<RasterTotalOverlapMapEvaluator> _overlapEvaluator);

		// --> Getter Functions
		int getNumItems() { return originalProblem->count(); }
		int getCurrentWidth() { return currentWidth; }
		int getCurrentHeight() { return currentHeight; }
		// Basic Functions
		void generateRandomSolution(RasterPackingSolution &solution);
		void generateBottomLeftSolution(RasterPackingSolution &solution, BottomLeftMode mode = BL_STRIPPACKING);
		// --> Get layout overlap (sum of individual overlap values)
		qreal getGlobalOverlap(RasterPackingSolution &solution);
		// --> Local search
		void performLocalSearch(RasterPackingSolution &solution);
		// --> Change container size
		bool setContainerWidth(int &pixelWidth, RasterPackingSolution &solution);
		bool setContainerWidth(int &pixelWidth);
		bool setContainerDimensions(int &pixelWidthX, int &pixelWidthY, RasterPackingSolution &solution);
		bool setContainerDimensions(int &pixelWidthX, int &pixelWidthY);
		// --> Guided Local Search functions
		virtual void updateWeights(RasterPackingSolution &solution) { overlapEvaluator->updateWeights(solution); };
		virtual void resetWeights() { overlapEvaluator->resetWeights(); };
		// --> Size information function
		int getMinimumContainerWidth() { return originalProblem->getMaxWidth(); }
		int getMinimumContainerHeight() { return originalProblem->getMaxHeight(); }

	protected:
		// --> Get minimum overlap position for item
		QPoint getMinimumOverlapPosition(int itemId, int orientation, RasterPackingSolution &solution, qreal &value);

		// --> Overlap determination functions
		bool detectItemPartialOverlap(QVector<int> sequence, int itemSequencePos, QPoint itemPos, int itemAngle, RasterPackingSolution &solution);
		qreal getItemTotalOverlap(int itemId, RasterPackingSolution &solution);

		// --> Pointer to problem, size variables and total map evaluator
		std::shared_ptr<RasterPackingProblem> originalProblem;
		int currentWidth, currentHeight, initialWidth, initialHeight;
		std::shared_ptr<RasterTotalOverlapMapEvaluator> overlapEvaluator;

	private:
		void generateBottomLeftStripSolution(RasterPackingSolution &solution);
		void generateBottomLeftRectangleSolution(RasterPackingSolution &solution);
		void generateBottomLeftSquareSolution(RasterPackingSolution &solution);
	};

	class RasterStripPackingClusterSolver : public RasterStripPackingSolver {
	public:
		RasterStripPackingClusterSolver(std::shared_ptr<RasterPackingClusterProblem> _problem, std::shared_ptr<RasterTotalOverlapMapEvaluator> _overlapEvaluator, std::shared_ptr<RasterStripPackingSolver> _originalSolver) : RasterStripPackingSolver(_problem, _overlapEvaluator)  {
			this->originalClusterProblem = _problem; this->originalSolver = _originalSolver;
		}
		// --> Convert solution to unclustered version
		void declusterSolution(RASTERVORONOIPACKING::RasterPackingSolution &solution) {
			if (solution.getNumItems() == originalClusterProblem->getOriginalProblem()->count()) return;
			originalClusterProblem->convertSolution(solution);
		};
		std::shared_ptr<RasterStripPackingSolver> getOriginalSolver() { return this->originalSolver; }
	private:
		std::shared_ptr<RasterPackingClusterProblem> originalClusterProblem;
		std::shared_ptr<RasterStripPackingSolver> originalSolver;
	};
}

#endif // RASTERSTRIPPACKINGSOLVER_H
