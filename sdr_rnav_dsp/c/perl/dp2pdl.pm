#!/usr/bin/perl -c

package dp2pdl;

use Exporter qw(import);

our @EXPORT = qw(pdl_from_vec_nocopy pdl_from_vec_copy);

use PDL::LiteF;
use PDL::Core::Dev;
use Data::Dumper;
use Cwd;

my $incdirs = '';
my @typemaps = ();
BEGIN {
 $incdirs  .= "-I" . &getcwd . "/../dp " . &PDL_INCLUDE;
 @typemaps =  (&getcwd . '/typemap' . &PDL_TYPEMAP);
}

use Inline(
 C   => Config =>
 INC           => $incdirs,
 TYPEMAPS      => &PDL_TYPEMAP,
 AUTO_INCLUDE  => &PDL_AUTO_INCLUDE,
 BOOT          => &PDL_BOOT,
);


use Inline C;
Inline->init;

1;

__DATA__
__C__

#include "dp_vector.h"

static pdl* new_pdl(int datatype, PDL_Indx dims[], int ndims)
{
  pdl *p = PDL->pdlnew();
  PDL->setdims (p, dims, ndims);  /* set dims */
  p->datatype = datatype;         /* and data type */
  PDL->allocdata (p);             /* allocate the data chunk */
  return p;
}

/* this creates a new PDL, a copy of the vector */
// pdl* pdl_from_vec_copy(dp_vec_t *v) { return pdl_from_vec_copy(v,0); }

pdl* pdl_from_vec_copy(dp_vec_t *v, int complex) {
 if (v->valid) {
  uint32_t i;
  PDL_Indx dims[] = {0,0};
  pdl *p = PDL->pdlnew();
  PDL_Short *pdata;
  p->datatype = PDL_S;

  if (complex == 2) {
   dims[0] = v->len / 2;
   dims[1] = 2;
   PDL->setdims(p,dims,2);
   PDL->allocdata(p);
   pdata =  (PDL_Short *)p->data;
   for (i=0;i<v->len/2;i++) {
    pdata[i]          = v->v[2*i];
    pdata[i+v->len/2] = v->v[2*i+1];
   }
  } else if (complex == 1) {
   dims[0] = 2;
   dims[1] = v->len / 2;
   PDL->setdims(p,dims,2);
   PDL->allocdata(p);
   pdata =  (PDL_Short *)p->data;
   for (i=0;i<v->len;i++) {
    pdata[i] = v->v[i];
   }
  } else {
   dims[0] = v->len;
   PDL->setdims(p,dims,1);
   PDL->allocdata(p);
   pdata =  (PDL_Short *)p->data;
   for (i=0;i<v->len;i++) {
    pdata[i] = v->v[i];
   }
  }
  return p;
 } else {
  return NULL;
 }
}


static void default_magic(pdl *p, int pa) { p->data = NULL; };

/* this creates a new PDL which references the vector. If 
 * the vector is destroyed, this pdl's data pointer is no longer
 * valid. Similarly, if this pdl is destroyed or goes out of 
 * scope, nothing happens to the data at this pointer.
 */
// pdl *pdl_from_vec_nocopy(dp_vec_t *v) { return pdl_from_vec_nocopy(v,0); }
pdl *pdl_from_vec_nocopy(dp_vec_t *v, int complex) {
 if (v->valid) {
  PDL_Indx dims[] = {0, 0};
  pdl *p = PDL->pdlnew();
  if (complex) {
   dims[0] = 2;
   dims[1] = v->len / 2;
   PDL->setdims(p,dims,2);
  } else {
   dims[0] = v->len;
   PDL->setdims(p,dims,1);
  }
  p->datatype = PDL_S;
  p->data = v->v;
  p->state |= PDL_DONTTOUCHDATA | PDL_ALLOCATED; 
  PDL->add_deletedata_magic(p,default_magic,0);
  return  p;
 } else {
  return NULL;
 }
}

