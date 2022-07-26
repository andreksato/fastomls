#ifndef _CSHAPEPARAMETERS_H_
#define _CSHAPEPARAMETERS_H_

// Forward declarations

class cShapeParameters;
class cShapeParameters_rotAccess;
class cShapeParameters_fAccess;
class cShapeParameters_tAccess;

#include <math.h>	// floor

inline double setInRange(double x) {
	if(x>=1 || x<0) return x - floor(x);
	return x;
}



class cShapeParameters{
	protected:
		unsigned long version;
		unsigned long rotVersion;
		unsigned long fVersion;
		unsigned long tVersion;
		bool d_rotFlag;
		unsigned int d_rot;
		double i_rot;		

		double i_f, i_t;

		cShapeParameters(cShapeParameters &p) {
			this->version = p.version;
			this->rotVersion = p.rotVersion;
			this->fVersion = p.fVersion;
			this->tVersion = p.tVersion;
			this->i_rot = p.i_rot;
			this->i_f = p.i_f;
			this->i_t = p.i_t;
			this->d_rot = p.d_rot;
			this->d_rotFlag = p.d_rotFlag;
		}
	
	public :
		cShapeParameters(bool d_rotFlag = false) {
			this->version = 0;
			this->rotVersion = 0;
			this->fVersion = 0;
			this->tVersion = 0;
			this->d_rotFlag = d_rotFlag;
			this->d_rot = 0;
		};

		double &rot() {
			return this->i_rot;
		}

		unsigned int &drot() {
			return this->d_rot;
		}

		unsigned int getRotVersion() const {
			return this->rotVersion;
		}

		double &f() {
			return this->i_f;
		}

		double &t() {
			return this->i_t;
		}

        bool isMoreRecentThan(const std::shared_ptr<cShapeParameters> &param) {
			return (this->version > param->version);
		}

        bool isRotationMoreRecentThan(const std::shared_ptr<cShapeParameters> &param) {
			return (this->rotVersion > param->rotVersion);
		}

		bool isRotationDiscrete() {
			return this->d_rotFlag;
		}

};

class cShapeWritableParameters : public cShapeParameters {
	
	public :
		
		cShapeWritableParameters(bool d_rotFlag = false) : cShapeParameters(d_rotFlag) {}

		void setrot(double rot) {
			this->i_rot = setInRange(rot);
			this->rotVersion++;
			this->version++;
		}

		void setdrot(unsigned int rot) {
			this->d_rot = rot;
			this->rotVersion++;
			this->version++;
		}
		
		void setf(double f) {
			this->i_f = setInRange(f);
			this->fVersion++;
		}

		void sett(double t) {
			this->i_t = setInRange(t);
			this->tVersion++;
			this->version++;
		}

		void setft(double f, double t) {
			this->i_t = setInRange(t);
			this->i_f = setInRange(f);
			this->fVersion++;
			this->tVersion++;
			this->version++;
		}

};

// Access Classes (just a syntax gimmick)


#endif	// _CSHAPEPARAMETERS_H_ 
