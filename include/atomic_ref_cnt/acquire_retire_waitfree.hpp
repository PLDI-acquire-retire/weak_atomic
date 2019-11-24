
#ifndef AR_WAITFREE_H
#define AR_WAITFREE_H

#include <iostream>
#include <vector>
#include <unordered_set>
#include <optional>
#include <atomic>

#include "utils.hpp"
#include "sw_copy.hpp"
#include "sw_copy_unbounded.hpp"

/* acquire-retire should only be used on plain old data types
   that fit into 8 or 16 bytes.
    
    The waitfree version with destination_unbounded only works for 8 byte types.
*/
template <typename T, template<typename> typename Dest = destination>
struct AcquireRetireWaitfree {
  struct alignas(128) slot_t {
    slot_t(T _empty) : empty(_empty), announce(empty), parent(nullptr) {}
    slot_t(AcquireRetireWaitfree* parent, T _empty) : empty(_empty), announce(empty), parent(parent) {}

    T empty;
    Dest<T> announce;
    std::vector<T> retired;
    AcquireRetireWaitfree* parent;
    
    std::optional<T> try_acquire(T* x) {
      T a = *x; // this read does not have to be atomic
      announce.write(a); // this write does not have to be atomic
      if(utils::load(x) == a) return a;
      else return {};
    }

    T acquire(T* x) {
      for(int i = 0; i < 3; i++) {
        auto a = try_acquire(x);
        if(a) return a.value();
      }
      return announce.sw_copy(x);
    }

    void release() {
      announce.write(empty); // this write does not have to be atomic
    }

    void retire(T t) {
      retired.push_back(t);
    }

    std::vector<T> eject() {
      if(retired.size() == 3*MAX_THREADS)
        return eject_all();
      else
        return {};
    }

    std::vector<T> eject_all() {
      std::unordered_set<T> held;
      std::vector<T> ejected;
      for (auto &x : parent->slots) {
        T reserved = x.announce.read(); // this read does not need to be atomic
        if (reserved != empty) 
          held.insert(reserved);
      }
      auto f = [&] (T x) {
        if (held.find(x) == held.end()) {
          ejected.push_back(x);
          return true;
        } else return false;
      };
      retired.erase(remove_if(retired.begin(), retired.end(), f),
          retired.end());
      return ejected;
    }

  };

  T empty;  
  std::atomic<int> num_slots;
  std::vector<slot_t> slots;
  
  AcquireRetireWaitfree(const AcquireRetireWaitfree&) = delete;
  AcquireRetireWaitfree& operator=(AcquireRetireWaitfree const&) = delete;
  
  AcquireRetireWaitfree(int max_slots, T _empty) : empty(_empty), num_slots(0), slots{}
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

template <typename T>
using AcquireRetireWaitfreeUnbounded = AcquireRetireWaitfree<T, destination_unbounded>;

#endif /* AR_LOCKFREE_H */
