#ifndef RASTERSTRIPPACKINGPARAMETERS_H
#define RASTERSTRIPPACKINGPARAMETERS_H

#define DEFAULT_RDEC 0.04
#define DEFAULT_RINC 0.01

namespace RASTERVORONOIPACKING {
	enum ConstructivePlacement { KEEPSOLUTION, RANDOMFIXED, BOTTOMLEFT};
	enum Heuristic { NONE, GLS };
	enum EnclosedMethod { SQUARE, RANDOM_ENCLOSED, COST_EVALUATION, BAGPIPE };

	class RasterStripPackingParameters
	{
	public:
		RasterStripPackingParameters() :
			Nmo(200), maxSeconds(600), heuristicType(GLS), searchScale(-1),
			fixedLength(false), maxIterations(0), rectangularPacking(false), rdec(DEFAULT_RDEC), rinc(DEFAULT_RINC), rectangularPackingMethod(RANDOM_ENCLOSED)
		{} // Default parameters

		RasterStripPackingParameters(Heuristic _heuristicType, qreal _searchScale) :
			Nmo(200), maxSeconds(600), heuristicType(_heuristicType), searchScale(_searchScale),
			fixedLength(false), maxIterations(0), rectangularPacking(false), rdec(DEFAULT_RDEC), rinc(DEFAULT_RINC), rectangularPackingMethod(RANDOM_ENCLOSED)
		{} // Default parameters with specific solver parameters

		void setNmo(int _Nmo) { this->Nmo = _Nmo; }
		int getNmo() { return this->Nmo; }

		void setTimeLimit(int _maxSeconds) { this->maxSeconds = _maxSeconds; }
		int getTimeLimit() { return this->maxSeconds; }

		void setIterationsLimit(int _maxIterations) { this->maxIterations = _maxIterations; }
		int getIterationsLimit() { return this->maxIterations; }

		void setHeuristic(Heuristic _heuristicType) { this->heuristicType = _heuristicType; }
		Heuristic getHeuristic() { return this->heuristicType; }

		void setFixedLength(bool val) { this->fixedLength = val; }
		bool isFixedLength() { return this->fixedLength; }

		void setInitialSolMethod(ConstructivePlacement _initialSolMethod) { this->initialSolMethod = _initialSolMethod; };
		ConstructivePlacement getInitialSolMethod() { return this->initialSolMethod; }

		void setInitialLenght(qreal _initialLenght) { this->initialLenght = _initialLenght; setInitialSolMethod(RANDOMFIXED); } // FIXME: Should the initial solution method be set automatically?
		qreal getInitialLenght() { return this->initialLenght; }

		void setClusterFactor(qreal _clusterFactor) { this->clusterFactor = _clusterFactor; }
		qreal getClusterFactor() { return this->clusterFactor; }
		
		void setRectangularPacking(bool val) { this->rectangularPacking = val; }
		bool isRectangularPacking() { return this->rectangularPacking; }

		void setRectangularPackingMethod(EnclosedMethod method) { this->rectangularPackingMethod = method; }
		EnclosedMethod getRectangularPackingMethod() { return this->rectangularPackingMethod; }

		void setSearchScale(qreal _searchScale) { this->searchScale = _searchScale; }
		qreal getSearchScale() { return this->searchScale; }

		void setResizeChangeRatios(qreal _ratioDecrease, qreal _ratioIncrease) { this->rdec = _ratioDecrease; this->rinc = _ratioIncrease; }
		qreal getRdec() { return this->rdec; }
		qreal getRinc() { return this->rinc; }

		void Copy(RasterStripPackingParameters &source) {
			setNmo(source.getNmo());
			setTimeLimit(source.getTimeLimit());
			setIterationsLimit(source.getIterationsLimit());
			setHeuristic(source.getHeuristic());
			setFixedLength(source.isFixedLength());
			setInitialSolMethod(source.getInitialSolMethod());
			if (getInitialSolMethod() == RANDOMFIXED) setInitialLenght(source.getInitialLenght());
			setClusterFactor(source.getClusterFactor());
			setRectangularPacking(source.isRectangularPacking());
			setRectangularPackingMethod(source.getRectangularPackingMethod());
			setSearchScale(source.getSearchScale());
			setResizeChangeRatios(source.getRdec(), source.getRinc());
		}

	private:
		int Nmo, maxSeconds, maxIterations;
		Heuristic heuristicType;
		ConstructivePlacement initialSolMethod;
		qreal initialLenght; // Only used with RANDOMFIXED initial solution
		bool fixedLength, rectangularPacking;
		EnclosedMethod rectangularPackingMethod;
		qreal searchScale;
		qreal clusterFactor;
		qreal rdec, rinc;
	};
}

#endif //RASTERSTRIPPACKINGPARAMETERS_H