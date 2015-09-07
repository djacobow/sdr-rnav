#ifndef FIRDES_MAIN_H
#define FIRDES_MAIN_H

#define MAX_BANDS (10)

typedef struct filter_t {
 int num_taps;
 double *bands;
 double *des;
 double *weights;
 int     type;
 int     num_bands;
 int     h_allocd;
 double  *h;
} filter_t;


void init_f(filter_t *f);
void cleanup_f(filter_t *f);
void dump_f(filter_t *f);
void read_args_f(int argc, char *argv[], filter_t *f);
void remez_wrap_f(filter_t *f);

#endif

