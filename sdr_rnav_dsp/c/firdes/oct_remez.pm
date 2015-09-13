# Author: David Jacobowitz
#         david.jacobowitz@gmail.com
#
# Date  : Fall 2013
#
# Copyright 2013, David Jacobowitz


# This file does nothing at all except put a wrapper around 
# the remez function found in the Octave Signal library.
#
# I don't understand that function at all, so it is reproduced
# below with no modifications except to turn it back into C from
# the Octave folks who turned it to CPP. I have also stripped the 
# Octave wrapper and created a simple perl wrapper.
#
# The only function to export is run_remez, the perl wrapper.

package oct_remez;
use strict;
use Inline 'C';
Inline->init;

use Exporter qw(import);

our @EXPORT = qw/run_remez/;

1;

#int remez(double *h[], int numtaps,
#	  int numband, const double bands[], 
#	  const double des[], const double weight[],
#	  int type, int griddensity)

__DATA__
__C__

/* Note: This code is from the GNU Octave Project, signals package, remez.cc */

/**************************************************************************
 * Parks-McClellan algorithm for FIR filter design (C version)
 *-------------------------------------------------
 *  Copyright (c) 1995,1998  Jake Janovetz (janovetz@uiuc.edu)
 *  Copyright (c) 2004  Free Software Foundation, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 51 Franklin Street, Boston, MA  02110-1301  USA
 *
 *
 *  Sep 1999 - Paul Kienzle (pkienzle@cs.indiana.edu)
 *      Modified for use in octave as a replacement for the matlab function
 *      remez.mex.  In particular, magnitude responses are required for all
 *      band edges rather than one per band, griddensity is a parameter,
 *      and errors are returned rather than printed directly.
 *  Mar 2000 - Kai Habel (kahacjde@linux.zrz.tu-berlin.de)
 *      Change: ColumnVector x=arg(i).vector_value();
 *      to: ColumnVector x(arg(i).vector_value());
 *  There appear to be some problems with the routine search. See comments
 *  therein [search for PAK:].  I haven't looked closely at the rest
 *  of the code---it may also have some problems.
 *************************************************************************/

#include <math.h>

#define CONST const
#define BANDPASS       1
#define DIFFERENTIATOR 2
#define HILBERT        3

#define NEGATIVE       0
#define POSITIVE       1

#define Pi             3.1415926535897932
#define Pi2            6.2831853071795865

#define GRIDDENSITY    16
#define MAXITERATIONS  80

/*******************
 * CreateDenseGrid
 *=================
 * Creates the dense grid of frequencies from the specified bands.
 * Also creates the Desired Frequency Response function (D[]) and
 * the Weight function (W[]) on that dense grid
 *
 *
 * INPUT:
 * ------
 * int      r        - 1/2 the number of filter coefficients
 * int      numtaps  - Number of taps in the resulting filter
 * int      numband  - Number of bands in user specification
 * double   bands[]  - User-specified band edges [2*numband]
 * double   des[]    - Desired response per band [2*numband]
 * double   weight[] - Weight per band [numband]
 * int      symmetry - Symmetry of filter - used for grid check
 * int      griddensity
 *
 * OUTPUT:
 * -------
 * int    gridsize   - Number of elements in the dense frequency grid
 * double Grid[]     - Frequencies (0 to 0.5) on the dense grid [gridsize]
 * double D[]        - Desired response on the dense grid [gridsize]
 * double W[]        - Weight function on the dense grid [gridsize]
 *******************/

void CreateDenseGrid(int r, int numtaps, int numband, const double bands[],
                     const double des[], const double weight[], int gridsize,
                     double Grid[], double D[], double W[],
                     int symmetry, int griddensity)
{
   int i, j, k, band;
   double delf, lowf, highf, grid0;

   delf = 0.5/(griddensity*r);

/*
 * For differentiator, hilbert,
 *   symmetry is odd and Grid[0] = max(delf, bands[0])
 */
   grid0 = (symmetry == NEGATIVE) && (delf > bands[0]) ? delf : bands[0];

   j=0;
   for (band=0; band < numband; band++)
   {
      lowf = (band==0 ? grid0 : bands[2*band]);
      highf = bands[2*band + 1];
      k = (int)((highf - lowf)/delf + 0.5);   /* .5 for rounding */
      for (i=0; i<k; i++)
      {
         D[j] = des[2*band] + i*(des[2*band+1]-des[2*band])/(k-1);
         W[j] = weight[band];
         Grid[j] = lowf;
         lowf += delf;
         j++;
      }
      Grid[j-1] = highf;
   }

/*
 * Similar to above, if odd symmetry, last grid point can't be .5
 *  - but, if there are even taps, leave the last grid point at .5
 */
   if ((symmetry == NEGATIVE) &&
       (Grid[gridsize-1] > (0.5 - delf)) &&
       (numtaps % 2))
   {
      Grid[gridsize-1] = 0.5-delf;
   }
}


/********************
 * InitialGuess
 *==============
 * Places Extremal Frequencies evenly throughout the dense grid.
 *
 *
 * INPUT: 
 * ------
 * int r        - 1/2 the number of filter coefficients
 * int gridsize - Number of elements in the dense frequency grid
 *
 * OUTPUT:
 * -------
 * int Ext[]    - Extremal indexes to dense frequency grid [r+1]
 ********************/

void InitialGuess(int r, int Ext[], int gridsize)
{
   int i;

   for (i=0; i<=r;i++)
      Ext[i] = i * (gridsize-1) / r;
}


/***********************
 * CalcParms
 *===========
 *
 *
 * INPUT:
 * ------
 * int    r      - 1/2 the number of filter coefficients
 * int    Ext[]  - Extremal indexes to dense frequency grid [r+1]
 * double Grid[] - Frequencies (0 to 0.5) on the dense grid [gridsize]
 * double D[]    - Desired response on the dense grid [gridsize]
 * double W[]    - Weight function on the dense grid [gridsize]
 *
 * OUTPUT:
 * -------
 * double ad[]   - 'b' in Oppenheim & Schafer [r+1]
 * double x[]    - [r+1]
 * double y[]    - 'C' in Oppenheim & Schafer [r+1]
 ***********************/

void CalcParms(int r, int Ext[], double Grid[], double D[], double W[],
                double ad[], double x[], double y[])
{
   int i, j, k, ld;
   double sign, xi, delta, denom, numer;

/*
 * Find x[]
 */
   for (i=0; i<=r; i++)
      x[i] = cos(Pi2 * Grid[Ext[i]]);

/*
 * Calculate ad[]  - Oppenheim & Schafer eq 7.132
 */
   ld = (r-1)/15 + 1;         /* Skips around to avoid round errors */
   for (i=0; i<=r; i++)
   {
       denom = 1.0;
       xi = x[i];
       for (j=0; j<ld; j++)
       {
          for (k=j; k<=r; k+=ld)
             if (k != i)
                denom *= 2.0*(xi - x[k]);
       }
       if (fabs(denom)<0.00001)
          denom = 0.00001;
       ad[i] = 1.0/denom;
   }

/*
 * Calculate delta  - Oppenheim & Schafer eq 7.131
 */
   numer = denom = 0;
   sign = 1;
   for (i=0; i<=r; i++)
   {
      numer += ad[i] * D[Ext[i]];
      denom += sign * ad[i]/W[Ext[i]];
      sign = -sign;
   }
   delta = numer/denom;
   sign = 1;

/*
 * Calculate y[]  - Oppenheim & Schafer eq 7.133b
 */
   for (i=0; i<=r; i++)
   {
      y[i] = D[Ext[i]] - sign * delta/W[Ext[i]];
      sign = -sign;
   }
}


/*********************
 * ComputeA
 *==========
 * Using values calculated in CalcParms, ComputeA calculates the
 * actual filter response at a given frequency (freq).  Uses
 * eq 7.133a from Oppenheim & Schafer.
 *
 *
 * INPUT:
 * ------
 * double freq - Frequency (0 to 0.5) at which to calculate A
 * int    r    - 1/2 the number of filter coefficients
 * double ad[] - 'b' in Oppenheim & Schafer [r+1]
 * double x[]  - [r+1]
 * double y[]  - 'C' in Oppenheim & Schafer [r+1]
 *
 * OUTPUT:
 * -------
 * Returns double value of A[freq]
 *********************/

double ComputeA(double freq, int r, double ad[], double x[], double y[])
{
   int i;
   double xc, c, denom, numer;

   denom = numer = 0;
   xc = cos(Pi2 * freq);
   for (i=0; i<=r;i++)
   {
      c = xc - x[i];
      if (fabs(c) < 1.0e-7)
      {
         numer = y[i];
         denom = 1;
         break;
      }
      c = ad[i]/c;
      denom += c;
      numer += c*y[i];
   }
   return numer/denom;
}


/************************
 * CalcError
 *===========
 * Calculates the Error function from the desired frequency response
 * on the dense grid (D[]), the weight function on the dense grid (W[]),
 * and the present response calculation (A[])
 *
 *
 * INPUT:
 * ------
 * int    r      - 1/2 the number of filter coefficients
 * double ad[]   - [r+1]
 * double x[]    - [r+1]
 * double y[]    - [r+1]
 * int gridsize  - Number of elements in the dense frequency grid
 * double Grid[] - Frequencies on the dense grid [gridsize]
 * double D[]    - Desired response on the dense grid [gridsize]
 * double W[]    - Weight function on the desnse grid [gridsize]
 *
 * OUTPUT:
 * -------
 * double E[]    - Error function on dense grid [gridsize]
 ************************/

void CalcError(int r, double ad[], double x[], double y[],
               int gridsize, double Grid[],
               double D[], double W[], double E[])
{
   int i;
   double A;

   for (i=0; i<gridsize; i++)
   {
      A = ComputeA(Grid[i], r, ad, x, y);
      E[i] = W[i] * (D[i] - A);
   }
}

/************************
 * Search
 *========
 * Searches for the maxima/minima of the error curve.  If more than
 * r+1 extrema are found, it uses the following heuristic (thanks
 * Chris Hanson):
 * 1) Adjacent non-alternating extrema deleted first.
 * 2) If there are more than one excess extrema, delete the
 *    one with the smallest error.  This will create a non-alternation
 *    condition that is fixed by 1).
 * 3) If there is exactly one excess extremum, delete the smaller
 *    of the first/last extremum
 *
 *
 * INPUT:
 * ------
 * int    r        - 1/2 the number of filter coefficients
 * int    Ext[]    - Indexes to Grid[] of extremal frequencies [r+1]
 * int    gridsize - Number of elements in the dense frequency grid
 * double E[]      - Array of error values.  [gridsize]
 * OUTPUT:
 * -------
 * int    Ext[]    - New indexes to extremal frequencies [r+1]
 ************************/
int Search(int r, int Ext[],
            int gridsize, double E[])
{
   int i, j, k, l, extra;     /* Counters */
   int up, alt;
   int *foundExt;             /* Array of found extremals */

/*
 * Allocate enough space for found extremals.
 */
   foundExt = (int *)malloc((2*r) * sizeof(int));
   k = 0;

/*
 * Check for extremum at 0.
 */
   if (((E[0]>0.0) && (E[0]>E[1])) ||
       ((E[0]<0.0) && (E[0]<E[1])))
      foundExt[k++] = 0;

/*
 * Check for extrema inside dense grid
 */
   for (i=1; i<gridsize-1; i++)
   {
      if (((E[i]>=E[i-1]) && (E[i]>E[i+1]) && (E[i]>0.0)) ||
          ((E[i]<=E[i-1]) && (E[i]<E[i+1]) && (E[i]<0.0))) {
	// PAK: we sometimes get too many extremal frequencies
	if (k >= 2*r) return -3;
	foundExt[k++] = i;
      }
   }

/*
 * Check for extremum at 0.5
 */
   j = gridsize-1;
   if (((E[j]>0.0) && (E[j]>E[j-1])) ||
       ((E[j]<0.0) && (E[j]<E[j-1]))) {
     if (k >= 2*r) return -3;
     foundExt[k++] = j;
   }

   // PAK: we sometimes get not enough extremal frequencies
   /* printf("k %d <? r+1 %d\n",k,r+1); */
   if (k < r+1) return -2;


/*
 * Remove extra extremals
 */
   extra = k - (r+1);
   assert(extra >= 0);

   while (extra > 0)
   {
      if (E[foundExt[0]] > 0.0)
         up = 1;                /* first one is a maxima */
      else
         up = 0;                /* first one is a minima */

      l=0;
      alt = 1;
      for (j=1; j<k; j++)
      {
         if (fabs(E[foundExt[j]]) < fabs(E[foundExt[l]]))
            l = j;               /* new smallest error. */
         if ((up) && (E[foundExt[j]] < 0.0))
            up = 0;             /* switch to a minima */
         else if ((!up) && (E[foundExt[j]] > 0.0))
            up = 1;             /* switch to a maxima */
         else
	 { 
            alt = 0;
	    // PAK: break now and you will delete the smallest overall
	    // extremal.  If you want to delete the smallest of the
	    // pair of non-alternating extremals, then you must do:
            //
	    // if (fabs(E[foundExt[j]]) < fabs(E[foundExt[j-1]])) l=j;
	    // else l=j-1;
            break;              /* Ooops, found two non-alternating */
         }                      /* extrema.  Delete smallest of them */
      }  /* if the loop finishes, all extrema are alternating */

/*
 * If there's only one extremal and all are alternating,
 * delete the smallest of the first/last extremals.
 */
      if ((alt) && (extra == 1))
      {
         if (fabs(E[foundExt[k-1]]) < fabs(E[foundExt[0]]))
	   /* Delete last extremal */
	   l = k-1;
	   // PAK: changed from l = foundExt[k-1]; 
         else
	   /* Delete first extremal */
	   l = 0;
	   // PAK: changed from l = foundExt[0];     
      }

      for (j=l; j<k-1; j++)        /* Loop that does the deletion */
      {
         foundExt[j] = foundExt[j+1];
	 assert(foundExt[j]<gridsize);
      }
      k--;
      extra--;
   }

   for (i=0; i<=r; i++)
   {
      assert(foundExt[i]<gridsize);
      Ext[i] = foundExt[i];       /* Copy found extremals to Ext[] */
   }

   free(foundExt);
   return 0;
}


/*********************
 * FreqSample
 *============
 * Simple frequency sampling algorithm to determine the impulse
 * response h[] from A's found in ComputeA
 *
 *
 * INPUT:
 * ------
 * int      N        - Number of filter coefficients
 * double   A[]      - Sample points of desired response [N/2]
 * int      symmetry - Symmetry of desired filter
 *
 * OUTPUT:
 * -------
 * double h[] - Impulse Response of final filter [N]
 *********************/
void FreqSample(int N, double A[], double h[], int symm)
{
   int n, k;
   double x, val, M;

   M = (N-1.0)/2.0;
   if (symm == POSITIVE)
   {
      if (N%2)
      {
         for (n=0; n<N; n++)
         {
            val = A[0];
            x = Pi2 * (n - M)/N;
            for (k=1; k<=M; k++)
               val += 2.0 * A[k] * cos(x*k);
            h[n] = val/N;
         }
      }
      else
      {
         for (n=0; n<N; n++)
         {
            val = A[0];
            x = Pi2 * (n - M)/N;
            for (k=1; k<=(N/2-1); k++)
               val += 2.0 * A[k] * cos(x*k);
            h[n] = val/N;
         }
      }
   }
   else
   {
      if (N%2)
      {
         for (n=0; n<N; n++)
         {
            val = 0;
            x = Pi2 * (n - M)/N;
            for (k=1; k<=M; k++)
               val += 2.0 * A[k] * sin(x*k);
            h[n] = val/N;
         }
      }
      else
      {
          for (n=0; n<N; n++)
          {
             val = A[N/2] * sin(Pi * (n - M));
             x = Pi2 * (n - M)/N;
             for (k=1; k<=(N/2-1); k++)
                val += 2.0 * A[k] * sin(x*k);
             h[n] = val/N;
          }
      }
   }
}

/*******************
 * isDone
 *========
 * Checks to see if the error function is small enough to consider
 * the result to have converged.
 *
 * INPUT:
 * ------
 * int    r     - 1/2 the number of filter coeffiecients
 * int    Ext[] - Indexes to extremal frequencies [r+1]
 * double E[]   - Error function on the dense grid [gridsize]
 *
 * OUTPUT:
 * -------
 * Returns 1 if the result converged
 * Returns 0 if the result has not converged
 ********************/

int isDone(int r, int Ext[], double E[])
{
   int i;
   double min, max, current;

   min = max = fabs(E[Ext[0]]);
   for (i=1; i<=r; i++)
   {
      current = fabs(E[Ext[i]]);
      if (current < min)
         min = current;
      if (current > max)
         max = current;

   }
   return (((max-min)/max) < 0.0001);
}








/********************
 u remez
 *=======
 * Calculates the optimal (in the Chebyshev/minimax sense)
 * FIR filter impulse response given a set of band edges,
 * the desired reponse on those bands, and the weight given to
 * the error in those bands.
 *
 * INPUT:
 * ------
 * int     numtaps     - Number of filter coefficients
 * int     numband     - Number of bands in filter specification
 * double  bands[]     - User-specified band edges [2 * numband]
 * double  des[]       - User-specified band responses [numband]
 * double  weight[]    - User-specified error weights [numband]
 * int     type        - Type of filter
 *
 * OUTPUT:
 * -------
 * double h[]      - Impulse response of final filter [numtaps]
 * returns         - true on success, false on failure to converge
 ********************/

int remez(double h[], int numtaps,
	  int numband, const double bands[], 
	  const double des[], const double weight[],
	  int type, int griddensity)
{
   double *Grid, *W, *D, *E;
   int    i, iter, gridsize, r, *Ext;
   double *taps, c;
   double *x, *y, *ad;
   int    symmetry;

   if (type == BANDPASS)
      symmetry = POSITIVE;
   else
      symmetry = NEGATIVE;

   r = numtaps/2;                  /* number of extrema */
   if ((numtaps%2) && (symmetry == POSITIVE))
      r++;

/*
 * Predict dense grid size in advance for memory allocation
 *   .5 is so we round up, not truncate
 */
   gridsize = 0;
   for (i=0; i<numband; i++)
   {
      gridsize += (int)(2*r*griddensity*(bands[2*i+1] - bands[2*i]) + .5);
   }
   if (symmetry == NEGATIVE)
   {
      gridsize--;
   }

   /*
   printf("remez-gridsize %d\n",gridsize);
   */
/*
 * Dynamically allocate memory for arrays with proper sizes
 */
   Grid = (double *)malloc(gridsize * sizeof(double));
   D = (double *)malloc(gridsize * sizeof(double));
   W = (double *)malloc(gridsize * sizeof(double));
   E = (double *)malloc(gridsize * sizeof(double));
   Ext = (int *)malloc((r+1) * sizeof(int));
   taps = (double *)malloc((r+1) * sizeof(double));
   x = (double *)malloc((r+1) * sizeof(double));
   y = (double *)malloc((r+1) * sizeof(double));
   ad = (double *)malloc((r+1) * sizeof(double));

/*
 * Create dense frequency grid
 */
   CreateDenseGrid(r, numtaps, numband, bands, des, weight,
                   gridsize, Grid, D, W, symmetry, griddensity);
   InitialGuess(r, Ext, gridsize);


/*
 * For Differentiator: (fix grid)
 */
   if (type == DIFFERENTIATOR)
   {
      for (i=0; i<gridsize; i++)
      {
/* D[i] = D[i]*Grid[i]; */
         if (D[i] > 0.0001)
            W[i] = W[i]/Grid[i];
      }
   }

/*
 * For odd or Negative symmetry filters, alter the
 * D[] and W[] according to Parks McClellan
 */
   if (symmetry == POSITIVE)
   {
      if (numtaps % 2 == 0)
      {
         for (i=0; i<gridsize; i++)
         {
            c = cos(Pi * Grid[i]);
            D[i] /= c;
            W[i] *= c; 
         }
      }
   }
   else
   {
      if (numtaps % 2)
      {
         for (i=0; i<gridsize; i++)
         {
            c = sin(Pi2 * Grid[i]);
            D[i] /= c;
            W[i] *= c;
         }
      }
      else
      {
         for (i=0; i<gridsize; i++)
         {
            c = sin(Pi * Grid[i]);
            D[i] /= c;
            W[i] *= c;
         }
      }
   }

/*
 * Perform the Remez Exchange algorithm
 */
   for (iter=0; iter<MAXITERATIONS; iter++)
   {
      CalcParms(r, Ext, Grid, D, W, ad, x, y);
      CalcError(r, ad, x, y, gridsize, Grid, D, W, E);
      int err = Search(r, Ext, gridsize, E);
      if (err) return err;
      int i;
      for(i=0; i <= r; i++) assert(Ext[i]<gridsize);
      if (isDone(r, Ext, E))
         break;
   }

   CalcParms(r, Ext, Grid, D, W, ad, x, y);

/*
 * Find the 'taps' of the filter for use with Frequency
 * Sampling.  If odd or Negative symmetry, fix the taps
 * according to Parks McClellan
 */
   for (i=0; i<=numtaps/2; i++)
   {
      if (symmetry == POSITIVE)
      {
         if (numtaps%2)
            c = 1;
         else
            c = cos(Pi * (double)i/numtaps);
      }
      else
      {
         if (numtaps%2)
            c = sin(Pi2 * (double)i/numtaps);
         else
            c = sin(Pi * (double)i/numtaps);
      }
      taps[i] = ComputeA((double)i/numtaps, r, ad, x, y)*c;
   }

/*
 * Frequency sampling design with calculated taps
 */
   FreqSample(numtaps, taps, h, symmetry);

/*
 * Delete allocated memory
 */
   free(Grid);
   free(W);
   free(D);
   free(E);
   free(Ext);
   free(x);
   free(y);
   free(ad);
   return iter<MAXITERATIONS?0:-1;
}


/* ****************************************************** */
/* ****************************************************** */
/* ****************************************************** */
/* ****************************************************** */

/* Code below here is Copyright 2013, David Jacobowitz */


double *alloc_doubles(int i) {
 return malloc(sizeof(double) * i);
}

void copyToC(double *dst, SV *src,int c, double scale) {
 AV *a_src = (AV *)SvRV(src);
 int i;
 for (i=0;i<c;i++) {
  SV *s = *av_fetch(a_src,i,0);
  dst[i] = SvNV(s) * scale;
 }
}

void showC(double *x, int c) {
 int i;
 for (i=0;i<c;i++) {
  printf("x[%d] = %f\n",i,x[i]);
 }
 printf("\n\n");
};

void showCi(double *x, int c) {
 int i;
 for (i=0;i<c;i++) {
  printf("x[%d] = %f\n",i,(32768.0 * x[i]));
 }
 printf("\n\n");
};

SV *run_remez(int numtaps, int numbands, SV *bands,
              SV *des, SV *weights, int type, int griddensity) {

 double *c_results = alloc_doubles(numtaps);
 double *c_bands   = alloc_doubles(numbands * 2);
 double *c_des     = alloc_doubles(numbands * 2);
 double *c_weights = alloc_doubles(numbands);
 copyToC(c_bands,bands,numbands * 2,0.5);
 copyToC(c_des,des,numbands * 2,1.0);
 copyToC(c_weights,weights,numbands,1.0);
 /*
 showC(c_bands,numbands * 2);
 showC(c_des,numbands * 2);
 showC(c_weights,numbands);
 */
 memset(c_results,0,sizeof(double) * numtaps);

 int fail = remez(c_results,numtaps,numbands,c_bands,c_des,c_weights,type,griddensity);
 free(c_bands);
 free(c_des);
 free(c_weights);

 AV *o =  newAV();

 if (fail) { 
  printf("-remez failed-\n");
 } else {
  int i;
  for (i=0;i<numtaps;i++) {
   av_store(o, i, newSVnv(c_results[i]));
  }
  free(c_results);
 }
 return  newRV_inc(o);;
}
