

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include "chebdes.h"
#include <inttypes.h>
#include "helpers.h"
#include <math.h>

namespace djdsp {

// from http://www.dspguide.com/ch20/4.htm
// not really tested, and so far not really used (7/15/2013) since
// current version of program is 100% FIR.
bool chebdes_i(double fc, bool  lh, double pr, uint16_t np,
	       int16_t *a, int16_t *b) {

 double da[23];
 double db[23];
 chebdes_d(fc,lh,pr,np,da,db);

 double max_abs_tap = 1;
 for (uint16_t i=0;i<=np;i++) {
  double aabs = abs(a[i]);
  double babs = abs(b[i]);
  if (aabs > max_abs_tap) { max_abs_tap = aabs; };
  if (babs > max_abs_tap) { max_abs_tap = babs; };
 } 
 for (uint16_t i=0;i<=np;i++) {
	 f1(max_abs_tap);
   da[i] /= max_abs_tap;
   db[i] /= max_abs_tap;
   a[i] = da[i] * 32767.0;
   b[i] = db[i] * 32767.0;
 };
 return true;
};




bool chebdes_d(double fc, bool  lh, double pr, uint16_t np,
	       double *a, double *b) {

 double ta[23];
 double tb[23];

 double a0, a1, a2;
 double b1, b2;

 for (uint16_t i=0;i<=22;i++) {
  a[i] = 0;
  b[i] = 0;
 }
 a[2] = 1;
 b[2] = 1;

 for (uint16_t p=1;p<=(np/2);p++) {
  cd_helper(fc, lh, pr, np, p,
	    &a0, &a1, &a2, &b1, &b2);

  for (uint16_t i=0;i<=22;i++) {
   ta[i] = a[i];
   tb[i] = b[i]; 
  }
  for (uint16_t i=2;i<=22;i++) {
   a[i] = a0*ta[i] + a1*ta[i-1] + a2*ta[i-2];
   b[i] =    tb[i] - b1*tb[i-1] - b2*tb[i-2];
  }
 }

 b[2] = 0;
 for (uint16_t i=0;i<=20;i++) {
  a[i] = a[i+2];
  b[i] = -b[i+2];
 }

 double sa = 0;
 double sb = 0;
 for (uint16_t i=0;i<=20;i++) {
  if (lh) {
   sa = sa + a[i] * (pow(-1.0,i));
   sb = sb + b[i] * (pow(-1.0,i));
  } else {
   sa = sa + a[i];
   sb = sb + b[i];
  }
 };

 double gain = sa / (1 - sb);

 for (uint16_t i=0;i<=20;i++) {
  a[i] /= gain;
 };
 return true;
}






void
cd_helper(double fc, bool lh, double pr, uint16_t np, double p,
	  double *a0, double *a1, double *a2, double *b1, double *b2) {
	
 double rp = -cos(PI/(np*2.0) + (p-1.0)*PI/np);
 double ip =  sin(PI/(np*2.0) + (p-1.0)*PI/np);

 double es = 0;
 double vx = 0;
 double kx = 0;

 if (pr) {
  es = sqrt(pow((100.0/(100.0-pr)),2)-1.0);
  vx = (1.0/np)*log((1.0/es)+sqrt((1.0/(es*es))+1.0));
  kx = (1.0/np)*log((1.0/es)+sqrt((1.0/(es*es))-1.0));
  kx = (exp(kx) + exp(-kx))/2.0;
  rp = rp * ((exp(vx)-exp(-vx))/2.0)/kx;
  ip = ip * ((exp(vx)+exp(-vx))/2.0)/kx;
 }

 f1(rp);
 f1(ip);
 f1(es);
 f1(vx);
 f1(kx);

 double t = 2.0 * tan(1.0/2.0);
 double w = 2.0 * PI * fc;
 double m = rp*rp + ip*ip;
 double d = 4.0 - 4.0*rp*t + m*t*t;
 double x0 = t*t/d;
 double x1 = 2.0*t*t/d;
 double x2 = t*t/d;
 double y1 = (8.0-2.0*m*t*t)/d;
 double y2 = (-4.0 - 4.0*rp*t - m*t*t)/d;

 f1(t);
 f1(w);
 f1(m);
 f1(d);
 f1(x0);
 f1(x1);
 f1(x2);
 f1(y1);
 f1(y2);

 double k;
 if (lh) {
  k = -cos(w/2 + 1.0/2.0) / cos(w/2 - 1.0/2.0);
 } else {
  k =  sin(1.0/2.0 - w/2.0) / sin(1.0/2.0 + w/2.0);
 }
 d = 1 + y1*k - y2*k*k;
 *a0 = (x0-x1*k+x2*k*k)/d;
 *a1 = (-2.0*x0*k + x1 + x1*k*k - 2*x2*k)/d;
 *a2 = (x0*k*k - x1*k + x2)/d;
 *b1 = (2.0*k + y1 + y1*k*k - 2*y2*k)/d;
 *b2 = (-(k*k) - y1*k + y2)/d;
 if (lh) {
  *a1 = - *a1;
  *b1 = - *b1;
 };
};

  

} // namespace
