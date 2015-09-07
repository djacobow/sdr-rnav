
#ifdef __WIN32

// This is to get over a bug in strawberry perl's windows
// header files where this enum in the pthread.h is also 
// a defined macro
# ifdef  PTHREAD_CREATE_JOINABLE 
# define PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_JOINABLE
# endif

# ifdef setbuf
# undef setbuf
# endif

# ifdef close
# undef close
# endif

#endif

#define INLINE_PERL
#define USE_RADIO
// #define FFT_FIR
#define DEBUG

#ifdef __linux
// #define AUDIO_OUTPUT
#endif


