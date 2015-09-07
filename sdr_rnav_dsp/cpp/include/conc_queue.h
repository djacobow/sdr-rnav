
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

// This implements a thread-safe queue class on a generic object, but with 
// a few wrinkles of semantics. In particular, there is a try_pop_all, which
// sucks everything out of the queue and returns only the last item. There
// is also push_max which will push an object onto the queue, but if there 
// are more than max_size objects after the push, a few will be popped off
// to keep the total queue length only max_size;

#ifndef __CONC_QUEUE_H
#define __CONC_QUEUE_H

#include <queue>
#include <pthread.h>
#include <iostream>

template <class T>
class concurrent_queue_c {
 private:
  std::queue<T> the_queue;
  mutable pthread_mutex_t the_mutex;
  pthread_cond_t the_condition_variable;
 public:
  concurrent_queue_c() {
   pthread_mutex_init(&the_mutex,0);
   pthread_cond_init(&the_condition_variable,0);
  };
  ~concurrent_queue_c() {
   pthread_mutex_destroy(&the_mutex);
   pthread_cond_destroy(&the_condition_variable);
  };
  void 
  push(T const& data) {
   // std::cout << "-q- getting lock" << std::endl;
   pthread_mutex_lock(&the_mutex);
   the_queue.push(data);
   // std::cout << "-q- releasing lock and signaling" << std::endl;
   pthread_mutex_unlock(&the_mutex);
   pthread_cond_signal(&the_condition_variable);
  };
  void
  push_max(T const &data, unsigned int max_size) {
   // std::cout << "-q- getting lock" << std::endl;
   pthread_mutex_lock(&the_mutex);
   if (max_size < 1) { max_size = 1; };
   while (the_queue.size() >= (max_size-1)) {
    the_queue.pop();
   }
   the_queue.push(data);
   // std::cout << "-q- releasing lock and signaling" << std::endl;
   pthread_mutex_unlock(&the_mutex);
   pthread_cond_signal(&the_condition_variable);
  };

  bool 
  empty() {
   pthread_mutex_lock(&the_mutex);
   bool rv = the_queue.empty();
   pthread_mutex_unlock(&the_mutex);
   return rv;
  };

  bool try_pop_all(T& popped_value) {
   pthread_mutex_lock(&the_mutex);
   bool rv = false;
   if (the_queue.empty()) {
    rv = false;
   } else {
    rv = true;
    while (the_queue.size() > 1) {
     the_queue.pop();
    }
    popped_value = the_queue.front();
    the_queue.pop();
   }
   pthread_mutex_unlock(&the_mutex);
   return rv;
  }

  bool 
  try_pop(T& popped_value) {
   pthread_mutex_lock(&the_mutex);
   bool rv = false;
   if(the_queue.empty()) {
    rv = false;
   } else {
    popped_value=the_queue.front();
    the_queue.pop();
    rv = true;
   }
   pthread_mutex_unlock(&the_mutex);
   return rv;
  };

  void 
  wait_and_pop(T& popped_value) {
   pthread_mutex_lock(&the_mutex);
   while(the_queue.empty()) {
    pthread_cond_wait(&the_condition_variable, &the_mutex);
   }
   popped_value=the_queue.front();
   the_queue.pop();
   pthread_mutex_unlock(&the_mutex);
  };
};

#endif

