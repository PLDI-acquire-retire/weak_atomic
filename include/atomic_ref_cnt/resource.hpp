#ifndef RESOURCE_H
#define RESOURCE_H

#include <thread>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <experimental/algorithm>
#include <atomic>

#include "acquire_retire_waitfree.hpp"
#include "utils.hpp"

template <class T, class AcquireRetire = AcquireRetireWaitfree>
struct resource {
  AcquireRetire<T*, nullptr> ar{MAX_THREADS};
  T *a;
  resource(const resource&) = delete;
  resource& operator=(resource const&) = delete;
  resource() : a(nullptr) {};

  template<typename... Args>
  resource(Args... args) : a(new T(args...)) {};
  ~resource() {
    delete(a);

    // this loop in important for cleaning up
    // lingering items in the retired lists
    for(int tid = 0; tid < MAX_THREADS; tid++)
      for(T *t : ar.slots[tid].eject_all()) 
        delete t;
  }

  /*
    computed by f.
  */
  template <class F>
  void with_resource(F f) {
    T *b = ar.slots[utils::my_id].acquire(&(this->a));
    f(b);
    ar.slots[utils::my_id].release();
  }

  /*
    The function F has to return a pointer to a new object.
    This might be inefficient if the function decides to return
    the same object that was passed in. It would have to first
    increment the reference count and them decrement.

    The function F also has to be functional like in PAM.
  */
  template <class F>
  bool update_resource(F f) {
    T *b = ar.slots[utils::my_id].acquire(&(this->a));
    T *n = f(b);
    bool succ = utils::CAS(&a, b, n);
    if (succ) {
      if (b != nullptr) ar.slots[utils::my_id].retire(b);}
    else delete(n);
    ar.slots[utils::my_id].release();
    for(T *t : ar.slots[utils::my_id].eject_all()) delete t;
    return succ;
  }

  template<typename... Args>
  bool reset(Args... args) {
    T *o = ar.slots[utils::my_id].acquire(&(this->a));
    T *n = new T(args...);
    bool succ = utils::CAS(&a, o, n);
    if (succ) {
      if (o != nullptr) ar.slots[utils::my_id].retire(o);}
    else delete(n);
    ar.slots[utils::my_id].release();
    for(T *t : ar.slots[utils::my_id].eject_all()) delete t;
    return succ;
  }
};

#endif /* RESOURCE_H */
