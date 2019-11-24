
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


// Wraps a potentially large type T in a pointer
template <class T>
struct wrapper {
  T* ptr;

  // normal constructors
  wrapper() : ptr(nullptr) {};
  wrapper(const T &t) : ptr(new T(t)) {};
  wrapper(T &&t) : ptr(new T(t)) {};
  wrapper(T* _ptr) : ptr(_ptr) {};

  // copy constructor
  wrapper(const wrapper& w) {
    ptr = new T(*(w.ptr));
  }

  // move constructor
  wrapper(wrapper&& w) {
    ptr = w.ptr;
    w.ptr = nullptr;
  }

  // destructor
  ~wrapper() { 
    if(ptr != nullptr) {
      delete ptr; 
      ptr = nullptr;
    }
  }

};

/*
  This class assumes that T() and ~T() behave reasonably.

  This class can support shared_ptrs, but only through 
  a level of indirection. It might be worth writing a separate
  atomic_shared_ptr class.

  This class would be good for a home-brew shared pointer

  I'm going to assume T is 8 btyes for ease of implementation
*/
template <class T, template<typename> typename AcquireRetire = AcquireRetireWaitfree>
struct weak_atomic_large {
  //using P = plain_old_data<sizeof(T)>;
  using P = void*;

  public:
    weak_atomic_large(const weak_atomic_large&) = delete;
    weak_atomic_large& operator=(weak_atomic_large const&) = delete;
    weak_atomic_large() {
      wrapper<T> t_wrap(nullptr);
      x = to_bits(t_wrap);
    };

    template<typename... Args>
    weak_atomic_large(Args... args) {
      wrapper<T> t_wrap(new T(args...));
      x = to_bits(t_wrap);
      new (&t_wrap) wrapper<T>; // prevent value in x from being destructed
    };

    ~weak_atomic_large() {
      from_bits(x); // calls T's destructor on x
    }

    T load() {
      P z_bits = ar.slots[utils::my_id].acquire(&this->x);
      wrapper<T> z_wrap(copy(z_bits));
      T z(std::move(*(z_wrap.ptr))); // probaby can get rid of some moves
      ar.slots[utils::my_id].release();
      //new (&z_wrap) wrapper<T>; // prevent value in z from being destructed
      return z;
    }

    // stores that race with CASes might fail
    void store(T desired) {
      // P old_bits = ar.slots[utils::my_id].acquire(&(this->x)); // why is this acquire needed?
      P old_bits = x;
      wrapper<T> desired_wrap(std::move(desired));
      P new_bits = to_bits(desired_wrap);
      if (utils::CAS(&x, old_bits, new_bits)) {
        new (&desired_wrap) wrapper<T>;   // prevent desired_wrap from being destructed
        if (old_bits != nullptr)  ar.slots[utils::my_id].retire(old_bits);
      } 
      // ar.slots[utils::my_id].release();
      destruct_all(utils::my_id);
    }

    void destruct_all(int tid) {
      for(P t : ar.slots[tid].eject_all())  
        from_bits(t);
    }

    // Calls eject_all() for all processes and destructs everything that gets returned
    // Only call this during a quiescent state 
    // Important for cleaning up lingering items in the retired list
    void clear_all() {
      for(int tid = 0; tid < MAX_THREADS; tid++)
        destruct_all(tid);   
    }

  private:
    P x;
    static AcquireRetire<P> ar;

    /*
      This function calls the copy constructor of T using a bit sequence.

      This function probably places restrictions on the copy constructor of T.
      It also calls the default constructor and destructor of T.
    */
    // I wonder if the return type of this function has to be T&&
    // For this to work we need to assume that T() always returns the same 
    // sequence of bits.
    // guy: made some changes.  Should be called copy.
    static wrapper<T> copy(P t) {
      wrapper<T> a = from_bits(t); 
      wrapper<T> b(a);               // call copy constructor for type T
      new (&a) wrapper<T>();         // clear A so not destructed
      return b;  // needs to be a move, otherwise will copy
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"

    static P to_bits(const wrapper<T> &t) {
      P a; 
      memcpy(&a, &t, sizeof(wrapper<T>)); // maybe this should just be a reinterpret cast
      return a;
    }

    static wrapper<T> from_bits(P t) {
      wrapper<T> a;
      memcpy(&a, &t, sizeof(wrapper<T>)); 
      return a;
    }

#pragma GCC diagnostic pop
};

 template<class T, template<typename> typename AcquireRetire>
 AcquireRetire<void*> weak_atomic_large<T, AcquireRetire>::ar{MAX_THREADS, nullptr};

#endif /* WEAK_ATOMIC_H */
