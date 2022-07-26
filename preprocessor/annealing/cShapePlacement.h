#ifndef	_CSHAPEPLACEMENT_H_
#define	_CSHAPEPLACEMENT_H_

// forward declarations
class cShapePlacement;

#include "cVector.h"

class cShapePlacement{
	protected:
		unsigned long version;
		unsigned long rotVersion;
		unsigned long transVersion;
		double rot;
		cVector translation;
	
	public:

		cShapePlacement() {
			this->version = 0;
			this->transVersion = 0;
			this->rotVersion = 0;
		}

		//double getRotation() const {
		//	return this->rot;
		//}

		const cVector &getTranslation() const {
			return this->translation;
		}

        bool updatePlacement(const cVector &translation) {
            bool ret = false;
            if(this->translation!=translation) {	// Translation update
                this->translation = translation;
                this->transVersion++;
                this->version++;
                ret = true;
            }
            return ret;
        }

		bool updatePlacement(const cVector &translation, const std::shared_ptr<cShapeParameters> &param) {
			bool ret = false;
			if(this->translation!=translation) {	// Translation update
				this->translation = translation;
				this->transVersion++;
				this->version++;
				ret = true;
			}
			unsigned int newRotVersion = param->getRotVersion();
			if(this->rotVersion<newRotVersion) {	// Rotation update
				this->rot = param->rot();
				this->rotVersion = newRotVersion;
				this->version++;
				ret = true;
			}
			return ret;
		}

		bool isMoreRecentThan(const std::shared_ptr<cShapePlacement> &placement)	const {
			return this->version > placement->version;
		}

		bool isRotationMoreRecentThan(const std::shared_ptr<cShapePlacement> &placement) const {
			return this->rotVersion > placement->rotVersion;
		}

		bool isTranslationMoreRecentThan(const std::shared_ptr<cShapePlacement> &placement) const {
			return this->transVersion > placement->transVersion;
		}

		unsigned int getTranslationVersion() const {
			return this->transVersion;
		}

		unsigned int getRotationVersion() const {
			return this->rotVersion;
		}

		friend class cShape;
};

#endif	// _CSHAPEPLACEMENT_H_
