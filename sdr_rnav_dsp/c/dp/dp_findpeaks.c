#include "dp_findpeaks.h"
#include <math.h>
#include <stdio.h>

DP_SUB_CREATOR_IMPL(findpeaks)

void peak_pts_t_init(peak_pts_t *p) {
 int i;
 memset(p,0,sizeof(peak_pts_t));
 p->points = malloc(sizeof(peak_pt_t *) * DP_MAX_PTS);
 memset(p->points,0,sizeof(peak_pt_t *) * DP_MAX_PTS);
 for (i=0;i<DP_MAX_PTS;i++) {
  p->points[i] = malloc(sizeof(peak_pt_t));
  memset(p->points[i],0,sizeof(peak_pt_t));
 } 
 p->maxpts = DP_MAX_PTS;
}

void peak_pts_t_deinit(peak_pts_t *p) {
 int i;
 if (p->points) {
  for (i=0;i<DP_MAX_PTS;i++) {
   if (p->points[i]) { free(p->points[i]); };
  }
  free(p->points);
 }
 p->maxpts = 0;
}

DP_FN_PREAMBLE(findpeaks,init) {
 s->ln10 = log(10);
 s->iter = 0;
 s->threshold = 20;
 b->sub_work   = &dp_findpeaks_work;
 b->sub_prerun = &dp_findpeaks_prerun;
}
DP_FN_POSTAMBLE

DP_SUB_GENERIC_SETTER_IMPL(findpeaks,threshold,int)

DP_SUB_GENERIC_SETTER_IMPL(findpeaks,out,peak_pts_t *)

DP_FN_PREAMBLE(findpeaks,prerun) {
 s->out->length = b->runlength;
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(findpeaks,work) {
 int32_t avg = 0;
 uint32_t i;
 int32_t db20 = s->threshold;
 uint32_t ccount = 0;
 float v;

 for (i=0;i<b->runlength;i++) {
  avg += b->in_v->v[i];
 };
 avg /= b->runlength;

 s->out->average = avg;

 for (i=0;i<b->runlength;i++) {
  dp_int_t vv = b->in_v->v[i];
  
  /* careful not to take log of zero */
  if (avg == 0) {
   v = 100;
  } else if (vv == 0) {
   v = -90;
  } else {
   v= 20.0 * log((vv / avg) / s->ln10);
  }
  if ((v > db20 ) && (ccount < s->out->maxpts)) {
   /* printf("%d,%f,*\n",i,v); */
   s->out->points[ccount]->bin = i;
   s->out->points[ccount]->db  = v;
   s->out->points[ccount]->abs = b->in_v->v[i];
   ccount++;
  }
 }
 s->out->actpts = ccount;
 s->out->iteration = s->iter;
 s->iter++;
}
DP_FN_POSTAMBLE

