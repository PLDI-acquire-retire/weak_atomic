
#ifndef SW_COPY_UNBOUNDED_HPP
#define SW_COPY_UNBOUNDED_HPP

#include <optional>
#include <iostream>
#include <assert.h>

#include "utils.hpp"

// This class only works for 8 byte types
template <class T>
struct destination_unbounded {
  struct val_or_ptr {
    val_or_ptr() : val(T()), cnt_and_flag(1) {};
    val_or_ptr(T _val) : val(_val), cnt_and_flag(1) {};
    val_or_ptr(T* _ptr) : ptr(_ptr), cnt_and_flag(0) {};
    val_or_ptr(T _val, uint64_t _cnt_and_flag) : val(_val), cnt_and_flag(_cnt_and_flag) {};
    val_or_ptr(T* _ptr, uint64_t _cnt_and_flag) : ptr(_ptr), cnt_and_flag(_cnt_and_flag) {};

    union {
      T val;
      T* ptr;
    };
    uint64_t cnt_and_flag; // flag = 1 means val

    bool is_val() {
      return cnt_and_flag % 2;
    }
  };

  alignas(16) val_or_ptr data;
  T old;

  destination_unbounded() : data(val_or_ptr(T())), old(T()) {};
  destination_unbounded(T _val) : data(val_or_ptr(_val)), old(T()) {};

  T read() {
    val_or_ptr v = utils::load(&data);
    if(v.is_val()) {
      //if(sizeof(T) <= 8) std::cout << "2: " << (uint64_t) v.val << std::endl;
      return v.val;
    }
    else {
      T val = *(v.ptr);
      if(utils::CAS(&data, v, val_or_ptr(val, v.cnt_and_flag+1))) {
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
    utils::CAS(&data, data, val_or_ptr(new_val, data.cnt_and_flag+2));
  }

  T sw_copy(T* src) {
    val_or_ptr v = val_or_ptr(src, data.cnt_and_flag+1);
    val_or_ptr old_data = data;
    old = old_data.val;
    utils::CAS(&data, old_data, v);
    T val = *src;
    utils::CAS(&data, v, val_or_ptr(val, v.cnt_and_flag+1));
    return data.val;
  }
};

#endif // SW_COPY_UNBOUNDED_HPP