/* doom's compiled sources touch libm in exactly one spot (v_video.c: fabs on a
 * scaling factor). Provide the handful that could plausibly be referenced. */

double fabs(double x)  { return x < 0.0 ? -x : x; }
double floor(double x) { double t = (double)(long long)x; return t > x ? t - 1.0 : t; }
double ceil(double x)  { double t = (double)(long long)x; return t < x ? t + 1.0 : t; }
