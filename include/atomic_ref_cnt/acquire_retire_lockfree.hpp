
#ifndef AR_LOCKFREE_H
#define AR_LOCKFREE_H

#include <iostream>
#include <vector>
#include <unordered_set>
#include <atomic>

#include "utils.hpp"

/* acquire-retire should only be used on plain old data types
   that fit into 8 btyes (while there is 16 btye CAS, there's no 16 byte read).
   Currently assumes that T = uintptr_t and that nullptr will never be retired.
   If nullptr is ever retired, it will possibly never be ejected.
*/
template <typename T>
struct AcquireRetireLockfree {
  struct alignas(128) slot_t {
    slot_t(T _empty) : empty(_empty), announce(empty), parent(nullptr) {}
    slot_t(AcquireRetireLockfree* parent, T _empty) : empty(_empty), announce(empty), parent(parent) {}
    
    T empty;
    T announce;
    std::vector<T> retired;
    std::vector<T> free;
    AcquireRetireLockfree* parent;
    

    T acquire(T* x) {
      T a;
      do {
        a = *x; // this read does not have to be atomic
        announce = a; // this write does not have to be atomic
        std::atomic_thread_fence(std::memory_order_seq_cst);
      } while (utils::load(x) != a);
      return a;
    }

    void release() {
      announce = empty; // this write does not have to be atomic
    }

    void retire(T t) {
      retired.push_back(t);
    }

    std::optional<T> eject() {
      if(retired.size() == AcquireRetireLockfree::RECLAIM_DELAY*MAX_THREADS)
      {
        std::vector<T> tmp = eject_all();
        free.insert(free.end(), tmp.begin(), tmp.end());
      }
      
      if(free.empty()) return std::nullopt;
      else {
        T ret = free.back();
        free.pop_back();
        return ret;
      }
    }

    std::vector<T> eject_all() {
      std::unordered_set<T, utils::CustomHash<T>> held;
      std::vector<T> ejected;
      for (auto &x : parent->slots) {
        T reserved = x.announce; // this read does not need to be atomic
        if (reserved != empty) 
          held.insert(reserved);
      }
      auto f = [&] (T x) {
        if (held.find(x) == held.end()) {
          ejected.push_back(x);
          return true;
        } else {
          held.erase(x);
          return false;
        }
      };
      retired.erase(remove_if(retired.begin(), retired.end(), f),
          retired.end());
      return ejected;
    }
    
  };

  T empty;  
  std::atomic<int> num_slots;
  std::vector<slot_t> slots;
  static const int RECLAIM_DELAY = 10;
  
  AcquireRetireLockfree(const AcquireRetireLockfree&) = delete;
  AcquireRetireLockfree& operator=(AcquireRetireLockfree const&) = delete;
  
  AcquireRetireLockfree(int max_slots, T _empty) : empty(_empty), num_slots(0), slots{}
  {
    slots.reserve(max_slots*4);
    for(int i = 0; i < max_slots; i++)
      slots.push_back(slot_t(this, empty));
  }

  // slot_t* register_ar() {
  //   int i = num_slots.fetch_add(1);
  //   if (i > slots.size()) abort();
  //   return &slots[i];
  // }
    
};

#endif /* AR_LOCKFREE_H */
