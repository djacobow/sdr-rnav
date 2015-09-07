#ifndef __DP_BASE_INTERNALS_H
#define __DP_BASE_INTERNALS_H

#include "dp_base.h"

#define DP_SUB_GENERIC_GETTER_IMPL(name,vname,vtype) \
vtype dp_ ## name ## _get_ ##vname(dp_base_t *b) { \
 dp_ ## name ## _sub_t *s;  \
 if (DP_SUBVALID(b,name)) { \
  s = b->sub; \
  return s->vname; \
 } \
 return 0; \
}

#define DP_SUB_GENERIC_SETTER_IMPL(name,vname,vtype) \
void dp_ ## name ## _set_ ## vname(dp_base_t *b, vtype x) { \
 dp_ ## name ## _sub_t *s;  \
 if (DP_SUBVALID(b,name)) { \
  s = b->sub; \
  s->vname = x; \
 }  \
}

#define DP_SUB_STRING_SETTER_IMPL(name,vname) \
void dp_ ## name ## _set_ ## vname(dp_base_t *b, const char *x) { \
 dp_ ## name ## _sub_t *s;  \
 if (DP_SUBVALID(b,name)) { \
  s = b->sub; \
  if (s->vname) { \
   free(s->vname); \
  } \
  s->vname = malloc(strlen(x)+1); \
  strcpy(s->vname,x);  \
 }  \
}

#define DP_SUB_GENERIC_SETTER_DECL(name,vname,vtype) \
void dp_ ## name ## _set_ ## vname(dp_base_t *b, vtype)

#define DP_SUB_STRING_SETTER_DECL(name,vname) \
void dp_ ## name ## _set_ ## vname(dp_base_t *b, const char *)

#define DP_SUB_GENERIC_GETTER_DECL(name,vname,vtype) \
vtype dp_ ## name ## _get_ ##vname(dp_base_t *b)

#define DP_SUB_CREATOR_DECL(t) \
dp_base_t *dp_ ## t ## _create_from(dp_base_t *b) 

#define DP_SUB_CREATOR_IMPL(t) \
dp_base_t *dp_ ## t ## _create_from(dp_base_t *b) { \
 /* if (DP_VALID(b)) { */ \
 if (b) { \
  dp_base_create_from(b); \
  b->sub = malloc(sizeof(dp_ ## t ## _sub_t)); \
  if (b->sub) { \
   memset(b->sub,0,sizeof(dp_ ## t ## _sub_t));  \
   b->complex_ok = 0; \
   b->sub_valid = 1; \
   b->subt = dp_subt_ ## t; \
   dp_ ## t ## _init(b); \
  } \
 }  \
 return b; \
}

#ifdef DEBUG
#define DP_IFSUBVALID(b,t) \
  dp_ ## t ## _sub_t *s = b->sub; \
  assert (b && b->base_valid && \
          b->sub_valid && b->sub && \
	  (b->subt == dp_subt_ ## t)); \
  if (1) 

#else
#define DP_IFSUBVALID(b,t) \
  dp_ ## t ## _sub_t *s = b->sub; \
  if (b && b->base_valid && b->sub_valid && \
      b->sub && (b->subt == dp_subt_ ## t)) 

#endif

#define DP_SUBVALID(b,t) \
  (b && b->base_valid && b->sub_valid && b->sub && (b->subt == dp_subt_ ## t))

#define DP_VALID(b) \
  (b && b->base_valid)

#define DP_FN_DECL(t,n) \
void dp_ ## t ## _ ## n(dp_base_t *)


#define DP_FN_PREAMBLE(t,n) \
void dp_ ## t ## _ ## n(dp_base_t *b) { \
 dp_ ## t ## _sub_t *s = b->sub; \
 if (DP_SUBVALID(b,t)) \

#define DP_FN_POSTAMBLE \
}


#endif

