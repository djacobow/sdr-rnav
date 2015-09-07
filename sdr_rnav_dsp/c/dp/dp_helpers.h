#ifndef _DP_HELPERS_H
#define _DP_HELPERS_H

#ifdef DEBUG

#define d1(a)     fprintf(stderr,"%s:%d -- " #a ": %d\n",__FILE__,__LINE__,a)
#define x1(a)     fprintf(stderr,"%s:%d -- " #a ": 0x%x\n",__FILE__,__LINE__,a)
#define d2(a,b)   fprintf(stderr,"%s:%d -- " #a ": %d, " #b ": %d\n",__FILE__,__LINE__,a,b)
#define d3(a,b,c) fprintf(stderr,"%s:%d -- " #a ": %d, " #b ": %d, " #c ": %d\n",__FILE__,__LINE__,a,b,c)

#else

#define d1(a)

#endif

#ifndef SATURATE30
#define SATURATE30(name) \
	((name >  0x3fffffff) ?  0x3fffffff : \
	 (name < -0x40000000) ? -0x40000000 : name)
#endif

#ifndef SATURATE16
#define SATURATE16(name) \
        ((name > 32767) ? 32767 : \
	 (name < -32767) ? -32767 : name)
#endif


#endif

