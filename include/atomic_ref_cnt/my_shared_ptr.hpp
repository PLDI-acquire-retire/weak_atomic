#ifndef ATOMIC_REF_CNT_MY_SHARED_PTR_HPP_
#define ATOMIC_REF_CNT_MY_SHARED_PTR_HPP_

#include <atomic>

template<typename T>
struct reference_counted_t {
  std::atomic<int> count;
  T x;
  
  reference_counted_t() : count(0) { }
  
  template<typename F>
  reference_counted_t(F&& _x) : count(0), x(std::forward<F>(_x)) { }
  
  // Implicit conversion to underlying type
  operator T() const { return x; }
};

/*
  Reference counted pointer to node
*/

template<typename T>
struct my_shared_ptr {
  T* ptr;

  my_shared_ptr() : ptr(nullptr) {};

  // normal constructor
  my_shared_ptr(T* _ptr) : ptr(_ptr) {
    ptr->count.fetch_add(1, std::memory_order_seq_cst);
  }

  // copy constructor
  my_shared_ptr(const my_shared_ptr<T>& other) {
    ptr = other.ptr;
    if(ptr != nullptr) 
      ptr->count.fetch_add(1, std::memory_order_seq_cst);
  }

  // move constructor
  my_shared_ptr(my_shared_ptr&& other) {
    ptr = other.ptr;
    other.ptr = nullptr;
  }

  // destructor
  ~my_shared_ptr() {
    if(ptr != nullptr) {
      int c = ptr->count.fetch_add(-1, std::memory_order_seq_cst);
      if(c == 1) {
        delete ptr;
        ptr = nullptr;
      }
    }
  }

  // copy assignment
  my_shared_ptr& operator=( const my_shared_ptr& other ) {
    auto tmp = ptr;
    ptr = other.ptr;
    if(ptr != nullptr) 
      ptr->count.fetch_add(1, std::memory_order_seq_cst);
    if(tmp != nullptr) {
      int c = tmp->count.fetch_add(-1, std::memory_order_seq_cst);
      if(c == 1) delete tmp;
    }
    return *this;
  }

  // move assignment
  my_shared_ptr& operator=( my_shared_ptr&& other ) {
    auto tmp = ptr;
    ptr = other.ptr;
    other.ptr = nullptr;
    if(tmp != nullptr) {
      int c = tmp->count.fetch_add(-1, std::memory_order_seq_cst);
      if(c == 1) delete tmp;
    }
    return *this;
  }
  
  const T& operator*() const {
    return *(get());
  }
  
  T& operator*() {
    return *(get());
  }

  const T* get() const {
    return ptr;
  }

  T* get() {
    return ptr;
  }
};

#endif  // ATOMIC_REF_CNT_MY_SHARED_PTR_HPP_

