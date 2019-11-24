
#ifndef WEAK_LLSC_HPP
#define WEAK_LLSC_HPP

#include <vector>
#include <optional>

#include "utils.hpp"

/*
  The type T should be a plain old data type
*/
template <class T>
struct weak_llsc {

  // Nested classes
  struct alignas(128) buffer {
    T val;
    volatile int tid;
    bool seen;

    void init(const T& new_val) {
      val = new_val;
      tid = -1;
      seen = false;
    }

    void prepare() {
      tid = utils::my_id;
      seen = false;
    }

    void mark() { 
      if(tid == utils::my_id) seen = true;
    }

    bool check() {
      return tid == utils::my_id && seen;
    }
  };

  struct alignas(128) slot_t {
    std::vector<buffer*> retired_list;
    std::vector<buffer*> free_list;
    buffer* announce;

    slot_t() : announce(nullptr) {
      for(int i = 0; i < 2*MAX_THREADS; i++)
        free_list.push_back(new buffer);
    }

    ~slot_t() {
      for(auto buf : retired_list) delete buf;
      for(auto buf : free_list) delete buf;
    }
  };

  // variables
  alignas(16) buffer* x;
  static slot_t slots[MAX_THREADS];

  // constructors
  weak_llsc() { x = new buffer; }
  weak_llsc(const T &val) { 
    x = new buffer; 
    x->init(val);
  }
  weak_llsc(weak_llsc& other) = delete;
  weak_llsc& operator=(const weak_llsc&) = delete;
  weak_llsc(weak_llsc&& other) {
    x = other.x;
    other.x = nullptr;
  }
  weak_llsc& operator=(const weak_llsc&& other) = delete;
  ~weak_llsc() { 
    if(x) delete x; 
  }

  std::optional<T> weak_LL() { 
    auto tmp = x;
    slots[utils::my_id].announce = tmp;
    utils::FENCE();
    if(x == tmp) 
      return tmp->val;
    else 
      return {};
  }

  bool SC(const T& new_val) {
    buffer* new_buf = slots[utils::my_id].free_list.back();
    new_buf->init(new_val);
    auto tmp = slots[utils::my_id].announce;
    bool succ = utils::CAS(&x, tmp, new_buf);
    slots[utils::my_id].announce = nullptr;
    if(succ) {
      slots[utils::my_id].free_list.pop_back();
      retire(tmp);
    }
    return succ;
  }

  // There cannot be any successful SCs concurrent with this
  void quiescent_write(const T& new_val) {
    buffer* new_buf = slots[utils::my_id].free_list.back();
    new_buf->init(new_val);
    buffer* tmp = x;
    x = new_buf;
    slots[utils::my_id].free_list.pop_back();
    retire(tmp);
  }

  // There cannot be any successful SCs concurrent with this
  // Also performs LL on the new value
  void quiescent_write_and_LL(const T& new_val) {
    buffer* new_buf = slots[utils::my_id].free_list.back();
    new_buf->init(new_val);
    slots[utils::my_id].announce = new_buf;
    buffer* tmp = x;
    utils::FENCE();
    x = new_buf;
    slots[utils::my_id].free_list.pop_back();
    retire(tmp);
  }

  /* 
    performs an SC and if it is successful, behaves as if you
    performed LL on the new value 
  */
  bool SC_and_LL(const T& new_val) {
    buffer* new_buf = slots[utils::my_id].free_list.back();
    new_buf->init(new_val);
    auto tmp = slots[utils::my_id].announce;
    slots[utils::my_id].announce = new_buf; 
    bool succ = utils::CAS(&x, tmp, new_buf);
    if(succ) {
      slots[utils::my_id].free_list.pop_back();
      retire(tmp);
    }
    return succ;
  }

  void retire(buffer* buf) {
    slots[utils::my_id].retired_list.push_back(buf);
    if(slots[utils::my_id].retired_list.size() == 2*MAX_THREADS)
    {
      for(auto buf : slots[utils::my_id].retired_list)
        buf->prepare();
      for(int i = 0; i < MAX_THREADS; i++)
      {
        buffer* buf = slots[i].announce;
        if(buf) buf->mark();
      }
      std::vector<buffer*> new_retired_list;
      for(auto buf : slots[utils::my_id].retired_list)
      {
        if(buf->check())
          new_retired_list.push_back(buf);
        else
          slots[utils::my_id].free_list.push_back(buf);
      }
      slots[utils::my_id].retired_list = std::move(new_retired_list);
    }
  }
};

template <class T>
typename weak_llsc<T>::slot_t weak_llsc<T>::slots[MAX_THREADS];

#endif /* WEAK_LLSC_HPP */