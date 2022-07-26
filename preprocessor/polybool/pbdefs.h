//	pbdefs.h - common definitions for PolyBoolean
//
//	This file is a part of PolyBoolean software library
//	(C) 1998-1999 Michael Leonov
//	Consult your license regarding permissions and restrictions
//

#ifndef _PBDEFS_H_
#define _PBDEFS_H_

namespace POLYBOOLEAN
{

////////////// Begin of the platform specific section //////////////////
#ifdef _USE64BITSCOORDINATES

#ifdef __GNUC__ // GCC's 64 bits

// TODO: Add more plataform independent code!!!

typedef long  long				        pbINT;
typedef long				        	pbDINT __attribute__((mode(TI))); // GCC's 128 bits	
typedef unsigned long long				UINT;

#else // __GNUC__
#error "128 bits not defined on this platform"
#endif	// __GNUC__

#else // _USE64BITSCOORDINATES
#ifdef _MSC_VER

typedef __int32				pbINT;
typedef __int64				pbDINT;
typedef unsigned __int32	UINT;

#else

// insert platform specific sized integer types here

typedef int					pbINT;
typedef long long			pbDINT;
typedef unsigned int		UINT;

#endif // _MSC_VER
#endif // _USE64BITSCOORDINATES
////////////// End of the platform specific section //////////////////

// if you would like to use your own VECT2, simply put it here
// instead of the default struct VECT2 definition
struct VECT2
{
	double	x, y;
};

} // namespace POLYBOOLEAN

// ranges for the integer coordinates

#ifdef _USE64BITSCOORDINATES

#define INT20_MAX			+274877906943LL
#define INT20_MIN			-274877906944LL


#else

#define INT20_MAX			+524287
#define INT20_MIN			-524288

#endif 


// error codes thrown by the library 
enum {
	err_ok = 0,		// never thrown
	err_no_memory,	// not enough memory
	err_io,			// file I/O error
	err_bad_parm	// bad input data
};

#endif // _PBDEFS_H_

