#ifdef __WIN32

/* There is a bug or incompatibilityi in the header files
 * in Strawberry Perl on Windows. Some header is defining
 * PTHREAD_CREATE_JOINABLE = 0, but in pthread.h there is 
 * an enum that uses that as the name. I suspect this is 
 * clash between two schemes in two different pthread files.
 * One is a series of defines, the other is an enum with
 * the same names.
 */
# ifdef  PTHREAD_CREATE_JOINABLE
# define PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_JOINABLE
# endif

# ifdef close
# undef close
# endif

#endif

#define DEBUG

#define KISS_FFT_FIXED_POINT 32

