
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdio.h>
#include <stdlib.h>

#include <inttypes.h>
#include <vector>

namespace djdsp {

// definitions for the vector type used by most blocks.
// This is a pretty naked wrapper on arrays, but with
// someone else managing the memory. :-)
typedef int16_t myint_t;
typedef std::vector<myint_t>  dvec_t;
typedef std::vector<int8_t>   i8vec_t;

// routines to print out vectors above
void show_dv(dvec_t d);
void show_dv(dvec_t d, uint32_t l);

// routine to make sure any math that's supposed to 
// fit into a 30b number does
#ifndef SATURATE30
#define SATURATE30(name) \
	((name >  0x3fffffff) ?  0x3fffffff : \
	 (name < -0x40000000) ? -0x40000000 : name)
#endif


// routine to make sure any math that's supposed to
// fit into a 16b number does.
//
#ifndef SATURATE16
#define SATURATE16(name) \
        ((name > 32767) ? 32767 : \
	 (name < -32767) ? -32767 : name)
#endif

// routines to print out variables. I'm a lazy typer and don't like
// elaborate printfs for tracing a variable. These routines let me
// just say
//
// d2(var1, var2)
//
// and I'll get nice printouts with variable name and line number of 
// those variables and their values.
// If DEBUG is turned off these macros are replaced with nothingness
//
#ifdef DEBUG
#define d1(a)     printf("%s:%d -- " #a ": %d\n",__FILE__,__LINE__,a)
#define d2(a,b)   printf("%s:%d -- " #a ": %d, " #b ": %d\n", \
		  __FILE__,__LINE__,a,b)
#define d3(a,b,c) printf("%s:%d -- " #a ": %d, " #b ": %d, " #c ": %d\n", \
		  __FILE__,__LINE__,a,b,c)
#define d4(a,b,c,d) printf("%s:%d -- " #a ": %d, " #b ": %d, " #c ": %d, " #d ": %d,\n", \
		  __FILE__,__LINE__,a,b,c,d)

#define d2c(a,b) printf("%s:%d, %d, %d\n",__FILE__,__LINE__,a,b)
#define d3c(a,b,c) printf("%s:%d, %d, %d, %d\n",__FILE__,__LINE__,a,b,c)
#define d4c(a,b,c,d) printf("%s:%d, %d, %d, %d, %d\n",__FILE__,__LINE__,a,b,c,d)
#define d7c(a,b,c,d,e,f,g) printf("%s:%d, %d, %d, %d, %d, %d, %d, %d\n",__FILE__,__LINE__,a,b,c,d,e,f,g)

#define f1(a)     printf("%s:%d -- " #a ": %f\n",__FILE__,__LINE__,a)
#define f2(a,b)   printf("%s:%d -- " #a ": %f, " #b ": %f\n", \
		  __FILE__,__LINE__,a,b)
#define f3(a,b,c) printf("%s:%d -- " #a ": %f, " #b ": %f, " #c ": %f\n", \
		  __FILE__,__LINE__,a,b,c)

#else

#define d1(a) 
#define d2(a,b)
#define d3(a,b,c)
#define f1(a) 
#define f2(a,b)
#define f3(a,b,c)
#define d2c(a,b)
#define d3c(a,b,c)
#define d4c(a,b,c,d)
#define d7c(a,b,c,d,e,f,g)

#endif

#ifndef PI
#define PI (3.14159265358)
#endif

} // namespace

#endif

