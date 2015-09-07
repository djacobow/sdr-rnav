#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_helpers.h"
#include <inttypes.h>

#if defined(__WIN32__)
#include <windows.h>
#endif


int main(int argc, char *argv[]) {

#if defined(__WIN32__)
// This is to disable the annoying crash box that pops up,
// particularly problematic for this program which pretty
// much announces it could not find a solution by crashing.

SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);

//or if you only care about Vista or newer:
//SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);

#endif

 filter_t f;
 init_f(&f);
 read_args_f(argc, argv, &f);
 remez_wrap_f(&f);
 dump_f(&f);
 cleanup_f(&f);
};

