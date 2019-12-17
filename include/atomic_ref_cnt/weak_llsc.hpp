
#ifndef WEAK_LLSC_HPP
#define WEAK_LLSC_HPP

#include <vector>
#include <optional>
#include <unordered_set>
#include <algorithm>

#include "utils.hpp"

/*
  The type T should be a plain old data type
*/
template <class T>
struct weak_llsc {

  // Nested classes
  struct alignas(64) buffer {
    T val;
    volatile int tid;
    volatile bool seen;

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

    bool check_and_clear() {
      bool ret = (tid == utils::my_id && seen);
      tid = -1;
      return ret;
    }
  };

  struct alignas(128) slot_t {
    std::vector<buffer*> retired_list;
    std::vector<buffer*> free_list;
    buffer* announce;

    slot_t() : announce(nullptr) {
      free_list.reserve(2*weak_llsc::RECLAIM_DELAY*MAX_THREADS);
      retired_list.reserve(2*weak_llsc::RECLAIM_DELAY*MAX_THREADS);
      for(int i = 0; i < weak_llsc::RECLAIM_DELAY*MAX_THREADS; i++)
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
  static const int RECLAIM_DELAY = 10;

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
  
  void retire(buffer* buf) {
    slots[utils::my_id].retired_list.push_back(buf);
    if(slots[utils::my_id].retired_list.size() == weak_llsc::RECLAIM_DELAY*MAX_THREADS)
    {
      std::unordered_set<void*, utils::CustomHash<void*>> held;
      for(int i = 0; i < MAX_THREADS; i++) {
        buffer* reserved = slots[i].announce; // this read does not need to be atomic
        if (reserved != nullptr) 
          held.insert((void*) reserved);
      }
      auto f = [&] (buffer* x) {
        if (held.find((void*) x) == held.end()) {
          slots[utils::my_id].free_list.push_back(x);
          return true;
        } else return false;
      };
      slots[utils::my_id].retired_list.erase(remove_if(slots[utils::my_id].retired_list.begin(), slots[utils::my_id].retired_list.end(), f),
          slots[utils::my_id].retired_list.end());
    }
  }
};

template <class T>
typename weak_llsc<T>::slot_t weak_llsc<T>::slots[MAX_THREADS];

#endif /* WEAK_LLSC_HPP */
