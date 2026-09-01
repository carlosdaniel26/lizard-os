#pragma once
/* doom's compiled sources include <math.h> but do not actually call libm;
 * these declarations exist only so the includes resolve. */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double fabs(double x);
double floor(double x);
double ceil(double x);
double sqrt(double x);
double pow(double x, double y);
double sin(double x);
double cos(double x);
double atan2(double y, double x);
