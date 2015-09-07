#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_helpers.h"
#include "pm_remez.h"


// I have no idea what this parameter does.
#define DEFAULT_GRID_DENSITY (16)

void init_f(filter_t *f) {
 f->bands   = new double[MAX_BANDS*2];
 f->des     = new double[MAX_BANDS*2];
 f->weights = new double[MAX_BANDS];
 for (int i=0;i<MAX_BANDS;i++) { 
  f->weights[i]   = 1.0;
  f->bands[2*i]   = 0;
  f->bands[2*i+1] = 0;
  f->des[i]       = 1.0;

 };
 f->type = 1;
 f->num_taps = 1;
 f->num_bands = 1;
 f->h_allocd = 0;
};

void cleanup_f(filter_t *f) {
 delete f->bands;
 delete f->des;
 delete f->weights;
 if (f->h_allocd) {
  delete f->h;
 }
};

void dump_f(filter_t *f) {

 printf("---\n");
 printf("number_taps: %d\n",f->num_taps);
 printf("type: %d\n",f->type);
 printf("number_bands: %d\n",f->num_bands);
 printf("bands:\n");
 for (int i=0;i<f->num_bands;i++) {
  printf(" - start: %lf\n",f->bands[2*i]);
  printf("   end: %lf\n",f->bands[2*i+1]);
 };
 printf("gains:\n");
 for (int i=0;i<f->num_bands;i++) {
  printf(" - start: %lf\n",f->des[2*i]);
  printf("   end: %lf\n",f->des[2*i+1]);
 };
 printf("weights:\n");
 for (int i=0;i<f->num_bands;i++) {
  printf(" - %lf\n",f->weights[i]);
 };
 printf("taps:\n");
 for (int i=0;i<f->num_taps;i++) {
  printf(" - %lf\n",f->h[i]);
 };
 printf("\n\n");
};

void
remez_wrap_f(filter_t *f) {
 int failed = 
 gr::filter::remez(f->h, 
       f->num_taps,
       f->num_bands,
       f->bands,
       f->des,
       f->weights,
       f->type,
       DEFAULT_GRID_DENSITY);
 if (failed) {
  printf("-ERROR- remez failed\n");
 };
};


void
read_args_f(int argc, char *argv[], filter_t *f) {

 char *arg = 0;
 int  i = 1;
 while (i < argc) {
  arg = argv[i];
  if (!strcmp(arg,"taps")) {
   i++;
   if (i < argc) {
    arg = argv[i];
    f->num_taps = strtol(arg,0,10);
    f->h = new double[f->num_taps];
    if (f->h) {
     f->h_allocd = f->num_taps;
    } else {
     printf("-error- could not alloc for %d taps\n",f->num_taps);
    }
   }
  } else if (!strcmp(arg,"type")) {
   i++;
   if (i < argc) {
    arg = argv[i];
    switch (arg[0]) {
     case 'b': f->type = 1; break;
     case 'B': f->type = 1; break;
     case 'd': f->type = 2; break;
     case 'D': f->type = 2; break;
     case 'h': f->type = 3; break;
     case 'H': f->type = 3; break;
    }
   }
  } else if (!strcmp(arg,"num_bands")) {
   i++;
   if (i < argc) {
    arg = argv[i];
    f->num_bands = strtol(arg,0,10);
    if (f->num_bands > MAX_BANDS) {
     printf("-error- too many bands requested. Max is %d\n",MAX_BANDS);
     exit(-1);
    }
   }
  } else if (!strcmp(arg,"bands")) {
   for (int bc=0;bc<f->num_bands*2;bc++) {
    i++;
    if (i < argc) {
     arg = argv[i];
     f->bands[bc] = strtof(arg,0);
    }
   }
  } else if (!strcmp(arg,"weights")) {
   for (int bc=0;bc<f->num_bands;bc++) {
    i++;
    if (i < argc) {
     arg = argv[i];
     f->weights[bc] = strtof(arg,0);
    }
   }
  } else if (!strcmp(arg,"gains")) {
   for (int bc=0;bc<f->num_bands*2;bc++) {
    i++;
    if (i < argc) {
     arg = argv[i];
     f->des[bc] = strtof(arg,0);
    }
   }
  } else {
   printf("-error- unknown command line option %s\n",arg);
   exit(-1);
  }
  i++;
 }
};


