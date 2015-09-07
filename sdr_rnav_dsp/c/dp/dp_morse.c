/* Author: David Jacobowitz
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
*/

#include "dp_morse.h"
#include <stdio.h>

/* This is the lookup table for morse code. It's structured a bit oddly
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
*/

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

void dp_morse_set_autolen(dp_base_t *b, uint32_t *al) {
 DP_IFSUBVALID(b,morse) {
  s->autolen = al;
 }
}

DP_SUB_CREATOR_IMPL(morse)

DP_FN_PREAMBLE(morse,init) {
 s->tpos = 0;
 s->last_elems = 0;
 s->decoded_str = malloc(DP_MORSE_MAX_STR+1);
 memset(s->decoded_str,0,DP_MORSE_MAX_STR+1);
 s->autolen = 0;
 b->sub_work = &dp_morse_work;
 b->sub_deinit = &dp_morse_deinit;

};
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(morse,deinit) {
 if (s->decoded_str) {
  free(s->decoded_str);
 }
}
DP_FN_POSTAMBLE


void
dp_morse_get_decoded(dp_base_t *b, char *o) {
 DP_IFSUBVALID(b,morse) {
  strcpy(o,s->decoded_str);
  memset(s->decoded_str,0,DP_MORSE_MAX_STR+1);
  s->strpos=0;
 }
}


DP_FN_PREAMBLE(morse,work) {
 uint32_t il;
 uint8_t opos = 0;
 int8_t ntpos;
 char och = 0;
 uint32_t i = 0;
 if (s->autolen) {
  il = *(s->autolen);
 } else {
  il = b->runlength;
 };
 for (i=0;i<il;i++) {
  char ich = b->in_v->v[i];
  switch (ich) {
   case '.' :
    ntpos = morse_table[s->tpos+1];
    break;
   case '-' :
    ntpos = morse_table[s->tpos+2];
    break;
   default:
    ntpos = s->tpos ? morse_table[s->tpos] : 0;
    break;
  }
  if (1) {
   if ((ntpos > 0) && (ntpos != '_')) {
    s->decoded_str[s->strpos++] = ntpos;
    och = ntpos;
    opos++;
    s->tpos = 0;
   } else {
    s->tpos = -ntpos;
   }
   if (1) {
    fprintf(stderr,"-cd- morse (%s) dd[%c] mo[%c]\n",b->name,ich,och);
   }
   if (ich == 'S') { s->decoded_str[s->strpos++] = ' '; opos++; };
   i++;
  }
 }
 s->last_elems = opos;
}
DP_FN_POSTAMBLE

