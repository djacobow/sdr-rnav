
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef BLOCK_BASE_C
#define BLOCK_BASE_C

#include "helpers.h"
#include <string>

namespace djdsp {

/*
 * base class for all the dsp "block" classes.
 * Has setup routines to set the address of input and output buffers,
 * as a pre_run() routine that can be used to do any initial setup work,
 * a post_run() routine that can be used to close files or do any shutdown
 * work, and a work() routine that process the input to the otuput
 *
 * A note about set_runlen():
 *
 *  This is the number of iterations for the internal loop; it corresponds
 *  ideally to the number of samples to be handled. It is *NOT* the length
 *  of the input or output buffer, not in bytes, not in elements, not in any 
 *  way.
 *
 *  It is presumed that the work() functions will understand what they are 
 *  working on, be it int8_t's, int16_t's, etc, or be it real data (one int 
 *  per sample) or IQ data (two ints per sample). The creator of the buffer
 *  has to be aware of these things, and the work() routine will also be 
 *  aware of them as it accesses the buffers.
 *
 *  ... but in general, in this library lengths are to be interpreted as
 *  samples in time, regardless of the size of the type or number of
 *  "channels."
 *
 */
typedef enum sig_cpx_t {
 s_real,
 s_complex,
 s_invalid,
} sig_cpx_t;


class block_base_c {
 public:
  block_base_c();
  ~block_base_c();
  // set the input buffer
  void set_in(const dvec_t *i);               
  void set_in(const dvec_t *i, uint32_t il);
  void set_in(const dvec_t *i, uint32_t il, sig_cpx_t s);

  void set_complex(sig_cpx_t s);              // set if input is complex
  void set_out(dvec_t *o);                    // set the output buffer
  void set_runlen(uint32_t l);                // set the number of iterations to run
  void set_name(std::string n);               // optionally set a name, for debugging
  std::string get_name();
  virtual void pre_run();                     // each dsp block redefines these
  void run(uint32_t rmask);
  virtual void post_run();
  void set_group(uint32_t d) { group_mask = d; };
 protected:
  
  virtual void work();
  const dvec_t *in;
  dvec_t *out;
  uint32_t l;
  std::string name;
  uint32_t group_mask;
  bool l_set;
  bool is_complex;
  bool complex_ok;
};

} // namespace

#endif

