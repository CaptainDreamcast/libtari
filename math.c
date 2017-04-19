#include "include/math.h"

#include <stdlib.h>
#include <math.h>

#define fabs(x) ((x < 0) ? -x : x)
// TODO: fix these math include problems

double randfrom(double tMin, double tMax) {
	double range = (tMax - tMin); 
	if(range == 0) return tMin;

	double div = RAND_MAX / range;
	return tMin + (rand() / div);
}

#define PI_FLOAT     3.14159265f
#define PIBY2_FLOAT  1.5707963f

double fatan2(double y, double x){
	if ( x == 0.0f )
	{
		if ( y > 0.0f ) return PIBY2_FLOAT;
		if ( y == 0.0f ) return 0.0f;
		return -PIBY2_FLOAT;
	}
	float atan;
	float z = (float)(y/x);
	if ( fabs( z ) < 1.0f )
	{
		atan = z/(1.0f + 0.28f*z*z);
		if ( x < 0.0f )
		{
			if ( y < 0.0f ) return atan - PI_FLOAT;
			return atan + PI_FLOAT;
		}
	}
	else
	{
		atan = PIBY2_FLOAT - z/(z*z + 0.28f);
		if ( y < 0.0f ) return atan - PI_FLOAT;
	}
	return atan;
}

double getLinearInterpolationFactor(double a, double b, double p) {
	return (p - a) / (b - a);
}

double interpolateLinear(double a, double b, double t) {
	return a + t * (b-a);
}