#ifndef _CSHAPE_H_
#define _CSHAPE_H_

// Forward declarations

class cShape;
class cContainer;

#include <memory>
#include "cShapeParameters.h"
#include "cVector.h"
#include "cShapePlacement.h"
#include "cPolygon.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

//TEST 
#include <vector>



class cShape{
	private:

		static unsigned long newId;
		unsigned long id;
        
        std::shared_ptr<cShapeWritableParameters> parameters;
        std::shared_ptr<cShapePlacement>	placement;

		double originalValue;

        std::shared_ptr<cPolygon> originalPolygon;

        std::shared_ptr<cPolygon> scaled;
        std::shared_ptr<cPolygon> rotated;
        std::shared_ptr<cPolygon> translated;

        std::shared_ptr<cShape> larger;
        std::shared_ptr<cShape> smaller;

		int anglesCount;
		//int *angles;
		// FIXME: Needs better solution to avoid Memory Leak
		std::vector<int> angles;
        int multiplicity;

        void recursiveUpdateRotations() {
            this->rotated.reset();
            this->translated.reset();
        }

		cShape(cShape &s);

	public:

        inline cShape(const cPolygon &p, double value, int multiplicity);
        inline cShape(const cPolygon &p, double value,  int multiplicity, int *angles, int anglesCount, bool d_rotFlag);
        inline cShape(const std::shared_ptr<cPolygon> &p, double value,  int multiplicity, int *angles, int anglesCount, bool d_rotFlag);

		// Copy constructor from reference
        //inline cShape(const std::shared_ptr<cShape> &shape);
        cShape(const std::shared_ptr<cShape> &shape);
		
        int getMultiplicity() {
            return this->multiplicity;
        }

		unsigned long getId() const {
			return this->id;
		}

		unsigned int getAnglesCount() const {
			return this->anglesCount;
		}

		unsigned int getAngle(int index) const {
			return this->angles[index];
		}

        const std::shared_ptr<cShapeParameters> &getParameters() {
            return (std::shared_ptr<cShapeParameters> &)this->parameters;
		}

        const std::shared_ptr<cShapePlacement> &getPlacement() {
			return this->placement;
		}

        inline const std::shared_ptr<cPolygon> &getRotatedPolygon() {
            if(this->rotated == nullptr) {
                double rotationvalue;
                if(this->parameters->isRotationDiscrete()) {
                    rotationvalue = (double)(this->parameters->drot())*2*M_PI/360.0;
                    this->rotated = this->scaled->getRotated(rotationvalue);
                }
                else {
                    rotationvalue = this->parameters->rot()*2*M_PI;
                    this->rotated = this->scaled->getRotated(rotationvalue);
                }
            }
			return this->rotated;
		}
        const std::shared_ptr<cPolygon> &getTranslatedPolygon();
        
        inline const std::shared_ptr<cShape> &getSmaller();
        inline const std::shared_ptr<cShape> &getLarger();
        inline const std::shared_ptr<cShape> &getCoarser();

		void setRot(double rot) {
			this->parameters->setrot(rot);
            this->recursiveUpdateRotations();
		}

		void setdRot(unsigned int drot) {
            this->parameters->setdrot(drot);
            this->recursiveUpdateRotations();
		}

		void setFandT(double f, double t) {
			this->parameters->setft(f, t);
		}

//        double getScale() const {
//        }

//		double getValue() const {
//			double scale = this->getScale();
//			return this->originalValue*scale*scale;
//		}

		int getMaxX() {
			return this->getTranslatedPolygon()->getMaxX();
		}

		int getMinX() {
			return this->getTranslatedPolygon()->getMinX();
		}

		// TEST
		double getRot() {
			return this->parameters->rot();
		}

		unsigned int getdRot() {
			return this->parameters->drot();
		}

		double getF() {
			return this->parameters->f();
		}

		double getT() {
			return this->parameters->t();
		}
};



#endif	/* _CSHAPE_H_ */
