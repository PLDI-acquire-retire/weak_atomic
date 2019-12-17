#ifndef RESOURCE_H
#define RESOURCE_H

#include <thread>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <experimental/algorithm>
#include <atomic>

#include <atomic_ref_cnt/acquire_retire_waitfree.hpp>
#include <atomic_ref_cnt/utils.hpp>

template <class T, template<typename> typename AcquireRetire = AcquireRetireWaitfree>
struct protect {
  AcquireRetire<T*> ar{MAX_THREADS, nullptr}; 
  T *a;
  protect(const protect&) = delete;
  protect& operator=(protect const&) = delete;
  protect() : a(nullptr) {};

  template<typename... Args>
  protect(Args... args) : a(new T(args...)) {};
  ~protect() {
    delete(a);
    for(int tid = 0; tid < MAX_THREADS; tid++)
    {
      for(T *t : ar.slots[tid].eject_all()) 
        delete t;
      while(eject_and_destruct(tid)) { }
    }
  }

  template <class R, class F>
  R use(F f) {
    T *b = ar.slots[utils::my_id].acquire(&(this->a));
    if constexpr (std::is_same<R, void>::value) {
      f(b);
      ar.slots[utils::my_id].release();     
    } else {
      R result = f(b);
      ar.slots[utils::my_id].release();
      return result;
    }
  }

  template <class F>
  void use(F f) {
    T *b = ar.slots[utils::my_id].acquire(&(this->a));
    f(b);
    ar.slots[utils::my_id].release();
  }

  void redirect(T* newT) {
    T* oldT = utils::FAS(&a, newT);
    if(oldT != nullptr)
      ar.slots[utils::my_id].retire(oldT);
    eject_and_destruct(utils::my_id);
  }

  bool eject_and_destruct(int tid) {
    std::optional<T*> t = ar.slots[tid].eject();
    if(t) {
      delete t.value();
      return true;
    } 
    return false;
  }
};

#endif /* RESOURCE_H */
