/* original version found at http://www.ews.uiuc.edu/%7Ejanovetz/DSP/remez/ */
/* added static modifiers before most functions */

/**************************************************************************
 * Parks-McClellan algorithm for FIR filter design (C version)
 *-------------------------------------------------
 *  Copyright (c) 1995,1998  Jake Janovetz (janovetz@uiuc.edu)
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

 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *************************************************************************/
#ifndef __REMEZ_H__
#define __REMEZ_H__

#define BANDPASS       1
#define DIFFERENTIATOR 2
#define HILBERT        3

#define NEGATIVE       0
#define POSITIVE       1

#define GRIDDENSITY    16
#define MAXITERATIONS  90

#ifndef M_2PI
#define M_2PI (3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148L)
#endif

/* Function prototype for remez() - the only function that should need be
 * called from external code
 */
void remez(double h[], int numtaps,
           int numband, double bands[], double des[], double weight[],
           int type);

#endif /* __REMEZ_H__ */

