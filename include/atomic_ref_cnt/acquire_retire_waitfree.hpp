
#ifndef AR_WAITFREE_H
#define AR_WAITFREE_H

#include <iostream>
#include <vector>
#include <unordered_set>
#include <optional>
#include <atomic>

#include "utils.hpp"
#include "sw_copy.hpp"

/* acquire-retire should only be used on plain old data types
   that fit into 8 or 16 bytes.
*/
template <typename T, template<typename> typename Dest = destination>
struct AcquireRetireWaitfree {
  struct alignas(128) slot_t {
    slot_t(T _empty) : empty(_empty), announce(empty), announce_waitfree(empty), announce_waitfree_flag(false), parent(nullptr) {}
    slot_t(AcquireRetireWaitfree* parent, T _empty) : empty(_empty), announce(empty), announce_waitfree(empty), announce_waitfree_flag(false), parent(parent) {}

    T empty;
    T announce;
    Dest<T> announce_waitfree;
    bool announce_waitfree_flag;
    std::vector<T> retired;
    std::vector<T> free;
    AcquireRetireWaitfree* parent;
    
    std::optional<T> try_acquire(T* x) {
      T a = *x; // this read does not have to be atomic
      announce = a; // this write does not have to be atomic
      std::atomic_thread_fence(std::memory_order_seq_cst);
      if(utils::load(x) == a) return a;
      else return {};
    }

    T acquire(T* x) {
      announce_waitfree_flag = false;
      for(int i = 0; i < 3; i++) {
        auto a = try_acquire(x);
        if(a) return a.value();
      }
      announce = empty;
      announce_waitfree_flag = true;
      announce_waitfree.sw_copy(x);
      return announce_waitfree.read();
    }

    void release() {
      if(announce_waitfree_flag) {
        announce_waitfree.write(empty);
        announce_waitfree_flag = false;
      }
      else announce = empty; // this write does not have to be atomic
    }

    void retire(T t) {
      retired.push_back(t);
    }

    std::optional<T> eject() {
      if(retired.size() == AcquireRetireWaitfree::RECLAIM_DELAY*MAX_THREADS)
      {
        std::vector<T> tmp = eject_all();
        free.insert(free.end(), tmp.begin(), tmp.end());
      }
      
      if(free.size() == 0) return std::nullopt;
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
        if(x.announce_waitfree_flag) {
          //utils::FENCE(); not needed because read() has a fence
          T reserved2 = x.announce_waitfree.read();
          if (reserved2 != empty) 
            held.insert(reserved2);         
        }
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

#endif /* AR_LOCKFREE_H */
