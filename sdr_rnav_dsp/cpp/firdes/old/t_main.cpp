#include "oct_remez.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
 const double bands[4] = { 0, 0.1, 0.2, 1 };
 const double gains[4] = { 1, 1, 0, 0 };
 const double weights[2] = { 1.7, 1 };
 const int ntaps = 15;
 double h[ntaps];

 for (int i=0;i<ntaps;i++) {
  h[i] = -1;
 }
 int result = remez(h, ntaps, 2, bands, gains, weights, 1, 16);

 if (result) {
  printf("remez failed\n");
 }
 for (int i=0;i<ntaps;i++) {
  printf("h[%d]  = %f\n",i,h[i]);
 }
 return result;
};

