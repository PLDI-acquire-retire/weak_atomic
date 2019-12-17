
#ifndef SW_COPY_HPP
#define SW_COPY_HPP

#include <optional>
#include <iostream>
#include <assert.h>

#include "utils.hpp"
#include "weak_llsc.hpp"

// T must be a plain old data type
template <class T>
struct destination {
  struct Data {
    Data() : val(T()), ptr(nullptr) {}
    Data(T _val) : val(_val), ptr(nullptr) {};
    Data(T* _ptr) : ptr(_ptr) {};

    T val;
    T* ptr;
  };

  weak_llsc<Data> data;
  alignas(16) T old;

  destination() : data(Data(T())), old(T()) {};
  destination(T _val) : data(Data(_val)), old(T()) {};

  T read() {
    std::optional<Data> d = data.weak_LL();
    if(!d) {
      d = data.weak_LL();
      if(!d) {
        //if(sizeof(T) <= 8) std::cout << "1: " << (uint64_t) old << std::endl;
        return utils::load(&old);
      }
    }
    // d is guaranteed to not be empty
    Data v = d.value();
    if(v.ptr == nullptr) {
      //if(sizeof(T) <= 8) std::cout << "2: " << (uint64_t) v.val << std::endl;
      return v.val;
    }
    else {
      T val = *(v.ptr);
      if(data.SC(Data(val))) {
        //if(sizeof(T) <= 8) std::cout << "3: " << (uint64_t) val << std::endl;
        return val;
      }
      else {
        d = data.weak_LL();
        if(d && d.value().ptr == nullptr)
          return d.value().val;
        //if(sizeof(T) <= 8) std::cout << "4: " << (uint64_t) old << std::endl;
        return utils::load(&old);
      }
    }
  }

  void write(T new_val) {
    // The following store is non-racy because only one process
    // can call sw_copy and write. This is only need to support
    // 16 btye types T.
    // Also the following weak_LL() cannot fail.
    utils::non_racy_store(&old, data.weak_LL().value().val);
    data.SC(Data(new_val)); // cannot fail
  }

  void sw_copy(T* src) {
    // The following store is non-racy because only one process
    // can call sw_copy and write.
    // Also the following weak_LL() cannot fail.
    utils::non_racy_store(&old, data.weak_LL().value().val);
    data.SC(Data(src)); // cannot fail
    T val = *src;
    std::optional<Data> d = data.weak_LL();
    if(d && d.value().ptr != nullptr)
      data.SC(Data(val));
  }
};

#endif // SW_COPY_HPP
