
#ifndef SW_COPY_HPP
#define SW_COPY_HPP

#include <optional>
#include <iostream>
#include <assert.h>

#include "utils.hpp"
#include "weak_llsc.hpp"

template <class T>
struct destination {
  struct val_or_ptr {
    val_or_ptr() : val(T()), is_val(true) {};
    val_or_ptr(T _val) : val(_val), is_val(true) {};
    val_or_ptr(T* _ptr) : ptr(_ptr), is_val(false) {};

    union {
      T val;
      T* ptr;
    };
    bool is_val;
  };

  weak_llsc<val_or_ptr> data;
  T old;

  destination() : data(val_or_ptr(T())), old(T()) {};
  destination(T _val) : data(val_or_ptr(_val)), old(T()) {};

  T read() {
    std::optional<val_or_ptr> c = data.weak_LL();
    if(!c) {
      c = data.weak_LL();
      if(!c) {
        //if(sizeof(T) <= 8) std::cout << "1: " << (uint64_t) old << std::endl;
        return old;
      }
    }
    // c is guaranteed to not be empty
    val_or_ptr v = c.value();
    if(v.is_val) {
      //if(sizeof(T) <= 8) std::cout << "2: " << (uint64_t) v.val << std::endl;
      return v.val;
    }
    else {
      T val = *(v.ptr);
      if(data.SC(val_or_ptr(val))) {
        //if(sizeof(T) <= 8) std::cout << "3: " << (uint64_t) val << std::endl;
        return val;
      }
      else {
        //if(sizeof(T) <= 8) std::cout << "4: " << (uint64_t) old << std::endl;
        return old;
      }
    }
  }

  void write(T new_val) {
    data.quiescent_write(val_or_ptr(new_val));
    std::atomic_thread_fence(std::memory_order_seq_cst);
    // data.weak_LL(); //can't fail
    // data.SC(val_or_ptr(new_val));
  }

  T sw_copy(T* src) {
    old = data.weak_LL().value().val;
    //data.SC_and_LL(val_or_ptr(src));
    data.quiescent_write_and_LL(val_or_ptr(src));
    T val = *src;
    data.SC(val_or_ptr(val)); 
    assert(data.weak_LL().value().is_val);
    return data.weak_LL().value().val;

    // if(data.SC(val_or_ptr(val))) return val;
    // else return data.weak_LL().value().val;
  }
};

#endif // SW_COPY_HPP