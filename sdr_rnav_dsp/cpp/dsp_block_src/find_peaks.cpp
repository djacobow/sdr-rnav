#include "find_peaks.h"
#include <cmath>

namespace djdsp {

findpeaks_c::findpeaks_c() {
 ln10 = log(10);
 iter = 0;
}

findpeaks_c::~findpeaks_c() {
 // nothing
}

void
findpeaks_c::set_out(peak_pts_t *ipts) {
 pts = ipts; 
}
void
findpeaks_c::pre_run() {
 pts->points.clear();
 pts->length = l;
};

void
findpeaks_c::work() {

 int32_t avg = 0;
 for (uint32_t i=0;i<l;i++) {
  avg += (*in)[i];
 };
 avg /= l;
 pts->average = avg;

 pts->points.clear();
 int32_t db20 = 100;
 for (uint32_t i=0;i<l;i++) {
  if ((*in)[i] > (db20 * avg )) {
   // d3(avg,i,(*in)[i]);
   float v = 20.0 * log((*in)[i] / avg) / ln10; 
   peak_pt_t pt;
   pt.bin = i;
   pt.db  = v;
   pts->points.push_back(pt);
  }
 }
 pts->iteration = iter;
 iter++;
};

} // namespace
