
#ifndef MEMORY_RECLAMATION_H
#define MEMORY_RECLAMATION_H

#include <atomic>

#include <atomic_ref_cnt/acquire_retire_waitfree.hpp>
#include <atomic_ref_cnt/utils.hpp>

template<typename T, template<typename> typename AcquireRetire = AcquireRetireWaitfree>
struct MemoryManager {
  AcquireRetire<T*> ar;

  MemoryManager() : ar(MAX_THREADS, nullptr) {}
  ~MemoryManager() {
    for(int tid = 0; tid < MAX_THREADS; tid++)
    {
      for(T *t : ar.slots[tid].eject_all()) 
        delete t;
      while(eject_and_destruct(tid)) { }
    }    
  }

  template<class R, class F>
  R protected_read(T** location, F f) {
    T* ptr = ar.slots[utils::my_id].acquire(location);
    if constexpr (std::is_same<R, void>::value) {
      f(ptr);
      ar.slots[utils::my_id].release();     
    } else {
      R result = f(ptr);
      ar.slots[utils::my_id].release();
      return result;
    }
  }

  bool eject_and_destruct(int tid) {
    std::optional<T*> t = ar.slots[tid].eject();
    if(t) {
      delete t.value();
      return true;
    } 
    return false;
  }

  void safe_free(T* ptr) {
    if(ptr != nullptr)
      ar.slots[utils::my_id].retire(ptr);
    eject_and_destruct(utils::my_id);
  }
};



#endif // MEMORY_RECLAMATION_H