#include <test.h>

/*
 * information for firdes routines
 *
__firdes__start__yaml__
---
 - name: demo_filt
   type: fir
   subtype: bandpass
   sample_rate: 1
   bands:
    - start: 0
      end: 0.1
    - start: 0.2
      end: 1.0 
   mags:
    - 1
    - 0
   ripple:
    - 0.01dB
    - -60dB
 - name: filt_one
   type: fir
   subtype: bandpass
   sample_rate: 31.25k
   bands:
    - start: 0
      end: 40
    - start: 100
      end: 15625
   mags:
    - 1
    - 0
   ripple:
    - 0.5dB
    - -30dB
 - name: filt_two
   type: fir
   subtype: bandpass
   sample_rate: 8k
   bands:
    - start: 0
      end: 1000
    - start: 2k
      end: 4k 
   mags:
    - 1
    - 0
   ripple:
    - 0.5dB
    - -30dB
 - name: filt_three
   type: fir
   subtype: bandpass
   sample_rate: 32k 
   bands:
    - start: 0
      end: 700
    - start: 900
      end: 1200
    - start: 1400
      end: 4000
   mags:
    - 0
    - 1
    - 0
   ripple:
    - -30dB
    - 0.5dB
    - -30dB
 - name: filt_like_tfilter_default 
   type: fir
   subtype: bandpass
   sample_rate: 8k 
   num_taps: 100
   bands:
    - start: 0
      end: 250 
    - start: 650
      end: 1000
   mags:
    - 1
    - 0
   ripple:
    - 0.1dB
    - -40dB
__firdes__end__yaml__
 
 *
 */


void foo() {

 poo f1, f2;

 f1.setup(_filt_type_FIR_taps_200_bands_0_40_100_15625_mags_0_0_n40_n40_sr_31250_coeffs);


 f2.setup(_filt_type_FIR_taps_10_bands_0_100_2000_4k_mags_0_0_n30_n30_sr_8k_coeffs);


 _filt_type_FIR_taps_60_bands_0_800_1000_1040_1240_4k_mags_n30_n30_0_0_n30_n30_sr_8k_coeffs

