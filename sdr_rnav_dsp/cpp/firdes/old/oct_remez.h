#ifndef OCT_REMEZ_H
#define OCT_REMEZ_H

#include <vector>

#ifndef OCTAVE_LOCAL_BUFFER
#define OCTAVE_LOCAL_BUFFER(T, buf, size) \
  std::vector<T> buf ## _vector (size); \
  T *buf = &(buf ## _vector[0])
#endif


#define CONST const
#define BANDPASS       1
#define DIFFERENTIATOR 2
#define HILBERT        3

#define NEGATIVE       0
#define POSITIVE       1

#ifndef Pi
#define Pi             3.1415926535897932
#define Pi2            6.2831853071795865
#endif

#define GRIDDENSITY    16
#define MAXITERATIONS  40

int remez(double h[], int numtaps,
	  int numband, const double bands[], 
	  const double des[], const double weight[],
	  int type, int griddensity);

#endif

