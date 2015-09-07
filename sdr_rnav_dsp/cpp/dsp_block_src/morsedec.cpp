// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// A morse decoding class. Takes a string of .'s and -'s 
// and ' ' to separate letters and 'S' to represent a long space.
// Puts the result in the buffer supplied and returns the number
// of letters decoded. Note that it also null-terminates the output
// string so you can just puts it or whatever.

#include <iostream>
#include "morsedec.h"

namespace djdsp {

// This is the lookup table for morse code. It's structured a bit oddly
// to make it very embedded friendly. It really a tree of 3-entry tables.
// The three entries in each group are for a separator or non symbol, a 
// dot and a dash, in that order. If the branch of the tree you are in
// is not sufficient to decide a letter, it will have a negative number
// at that location. Negating that number gives you a positive number
// that you can use as the base index to the next level of the table.
// If there is a positive number, then that's actually a letter.
//
// It's a bit of a hack and assumes all the letters are 7-bit ascii.
// Since this is Morse and we're only doing the alphabet, this is a
// safe bet.
//
// This table does NOT include every character in the morse code, just 
// the letters of the alphabet. Anything else is not of interest.
const int
morse_table[] = {

/* 0   */ '_',
/* 1 . */ -3,
/* 2 - */ -27,

/* .  */
/* 3   */ 'E',
/* 4 . */ -6,
/* 5 - */ -18,

/* ..  */
/* 6   */ 'I',
/* 7 . */ -9,
/* 8 - */ -15,

/* ... */
/* 9    */ 'S',
/* 10 . */ -12,
/* 11 - */ 'V',

/* .... */
/* 12   */ 'H',
/* 13 . */ '_',
/* 14 - */ '_',

/* ..-  */
/* 15   */ 'U',
/* 16 . */ 'F',
/* 17 - */ '_',

/* .-   */
/* 18   */ 'A',
/* 19 . */ -21,
/* 20 - */ -24,

/* .-.  */
/* 21   */ 'R',
/* 22 . */ 'L',
/* 23 - */ '_',

/* .--  */
/* 24   */ 'W',
/* 25 . */ 'P',
/* 26 - */ 'J',

/* -    */
/* 27   */ 'T',
/* 28 . */ -30,
/* 29 - */ -33,

/* -.   */
/* 30   */ 'N',
/* 31 . */ -36,
/* 32 - */ -39,

/* --   */
/* 33   */ 'M',
/* 34 . */ -42,
/* 35 - */ 'O',

/* -..  */
/* 36   */ 'D',
/* 37 . */ 'B',
/* 38 - */ 'X',

/* -.-  */
/* 39   */ 'K',
/* 40 . */ 'C',
/* 41 - */ 'Y',

/* --.  */
/* 42   */ 'G',
/* 43 . */ 'Z',
/* 44 - */ 'Q',

};


morsedec_c::morsedec_c() {
 tpos = 0;
 last_elems = 0;
 decoded_str.clear();
};

morsedec_c::~morsedec_c() {
};


#include <stdio.h>

std::string
morsedec_c::getDecoded() {
 std::string rv = decoded_str;
 decoded_str.clear();
 return rv;
}


void
morsedec_c::work() {

 uint32_t il = l;
 uint8_t opos = 0;
 int8_t ntpos;
 char och = 0;
 for (uint8_t i=0;i<il;i++) {
  char ich = (*in)[i];
  switch (ich) {
   case '.' :
    ntpos = morse_table[tpos+1];
    break;
   case '-' :
    ntpos = morse_table[tpos+2];
    break;
   default:
    ntpos = tpos ? morse_table[tpos] : 0;
    break;
  }
  if ((ntpos > 0) && (ntpos != '_')) {
   decoded_str += ntpos;
   och = ntpos;
   opos++;
   tpos = 0;
  } else {
   tpos = -ntpos;
  }
  if (1) {
   std::cout << "dd[" << ich << "] mo[";
   if (och) { std::cout << och; };
   std::cout << "]" << std::endl;
  }
  if (ich == 'S') { decoded_str += ' '; opos++; };
 }
 last_elems = opos;
};

} // namespace
