
#ifndef WEAK_ATOMIC_H
#define WEAK_ATOMIC_H

#include <thread>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <experimental/optional>
#include <experimental/algorithm>
#include <atomic>
#include <stdio.h>
#include <string.h>

#include "acquire_retire_waitfree.hpp"

/*

  ASSUMPTIONS:
    - T is 8 bytes or 16 bytes
    - T must be trivially move constructable
    - There exists an "empty" instantiation of T that
      is trivial to construct and destruct
*/

namespace weak_atomic_utils {
  //P turns T into a plain old data type
  template <class T>
  using POD =
    typename std::conditional<(sizeof(T) == 8), uint64_t,
        uint128_t
    >::type;

  template<typename T>
  struct DefaultWeakAtomicTraits {
    static T empty_t() { return T{}; }
  };
}


template <typename T, template<typename> typename AquireRetire = AcquireRetireWaitfree, typename Traits = weak_atomic_utils::DefaultWeakAtomicTraits<T>>
struct weak_atomic {

  using P = weak_atomic_utils::POD<T>;
  
  static T empty_t;
  static P empty_p;

  public:
    weak_atomic(const weak_atomic&) = delete;
    weak_atomic& operator=(weak_atomic const&) = delete;
    weak_atomic() {
      new (&x) T();
    };

    template<typename... Args>
    weak_atomic(Args&&... args) {
      new (&x) T(std::forward<Args>(args)...);
    };

    ~weak_atomic() {
      destroy(&x);
    }

    T load() {
      P z_bits = ar.slots[utils::my_id].acquire(&this->x);
      T z(copy(z_bits));
      ar.slots[utils::my_id].release();
      return z;
    }

    void store(T desired) {
      P old_bits = x;                   // this read does not have to be atomic
      P new_bits = to_bits(desired);
      if (__sync_bool_compare_and_swap(&x, old_bits, new_bits)) {
        set_to_empty(&desired);
        if (old_bits != empty_p)  ar.slots[utils::my_id].retire(old_bits);
      } 
      destruct_all(utils::my_id);
    }

    // we might need to assume that there are no races on old_val and new_val
    // while this is going on
    // I believe this would work for std::move(new_val) as well
    // We have to create a new copy of new_val just in case the CAS succeeds
    bool cas(const T &old_val, T new_val) {
      P old_bits = to_bits(old_val);
      P new_bits = to_bits(new_val);
      bool succ = utils::CAS(&x, old_bits, new_bits);
      if (succ) {
        set_to_empty(&new_val);                 // prevent new_val from being destructed
        if(old_bits != empty_p)                 // never retire nullptr because it is used
          ar.slots[utils::my_id].retire(old_bits);     // to represent 'empty' in the annoucement array
      }
      destruct_all(utils::my_id);
      return succ;
    }
    
    // calls the destructor of an object of type T at
    // the memory location p
    inline static void destroy(P* p) {
      reinterpret_cast<T*>(p)->~T();
    }

    // Set the value pointed to by ptr to the bits
    // that represent the empty object.
    inline static void set_to_empty(T* ptr) {
      new (ptr) P(empty_p);
    }

    void destruct_all(int tid) {
      for(P t : ar.slots[tid].eject())  
        destroy(&t);
    }

    // Calls eject_all() for all processes and destructs everything that gets returned
    // Only call this during a quiescent state 
    // Important for cleaning up lingering items in the retired list
    void clear_all() {
      for(int tid = 0; tid < MAX_THREADS; tid++) {
        for(P t : ar.slots[tid].eject_all())  
          destroy(&t);      
      }
    }
    
  private:
    alignas(16) P x;
    static AquireRetire<P> ar;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

    /*
      This function calls the copy constructor of T using a bit sequence.

      This function probably places restrictions on the copy constructor of T.
    */
    static T copy(P t) {
      return T(*reinterpret_cast<T*>(&t));
    }

    static P to_bits(const T &t) {
      return *reinterpret_cast<const P*>(&t);
    }

#pragma GCC diagnostic pop

};

template<typename T, template<typename> typename AcquireRetire, typename Traits>
T weak_atomic<T, AcquireRetire, Traits>::empty_t = Traits::empty_t();

template<typename T, template<typename> typename AcquireRetire, typename Traits>
typename weak_atomic<T, AcquireRetire, Traits>::P weak_atomic<T, AcquireRetire, Traits>::empty_p = weak_atomic<T, AcquireRetire, Traits>::to_bits(Traits::empty_t());

template<typename T, template<typename> typename AcquireRetire, typename Traits>
AcquireRetire<typename weak_atomic<T, AcquireRetire, Traits>::P> weak_atomic<T, AcquireRetire, Traits>::ar{MAX_THREADS, weak_atomic<T, AcquireRetire, Traits>::to_bits(Traits::empty_t())};

#endif /* WEAK_ATOMIC_H */
