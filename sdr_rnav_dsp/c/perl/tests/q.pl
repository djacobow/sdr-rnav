#!/usr/bin/perl -w

use Inline C;

$| = 1;
print "starting\n";

create_threads();

sub perl_sub_1 {
    print map "$_\n", @_;
}

__DATA__
__C__

#include <pthread.h>

pthread_t t1, t2;


void *t1_fn(void *p) {
 while (1) { 
  printf("hi from t1_fn\n");
  sleep(100);
 }
}

void *t2_fn(void *p) {
 while (1) {
  printf("greetings from t2_fn\n");
  sleep(200);
 }
}

void create_threads() {
 printf("starting threads\n");
 int v = 5;
 int rv1 = pthread_create(t1,NULL,&t1_fn,&v);
 // int rv2 = pthread_create(t2,NULL,&t2_fn,NULL);
 // printf("rvs %d %d\n",rv1,rv2);
}


void c_func_2(SV* text) {
     dSP;

     ENTER;
     SAVETMPS;

     XPUSHs(sv_2mortal(newSVpvf("Plus an extra line")));
     PUTBACK;

     call_pv("perl_sub_1", G_DISCARD);

     FREETMPS;
     LEAVE;
}

void c_func_1(SV* text) {
     c_func_2(text);
}
