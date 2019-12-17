#ifndef UTILS_H
#define UTILS_H

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <atomic>
#include <iostream>
#include <memory>
#include <type_traits>

#define MAX_THREADS 65

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;
const uint128_t ZERO = uint128_t(0);
const uint128_t MASK = (~uint128_t(0)>>64);

namespace utils {
  // easy to forget to set this upon thread creation
  thread_local int my_id = 0;

  // a slightly cheaper, but possibly not as good version
  // based on splitmix64
  inline uint64_t hash64_2(uint64_t x) {
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
  } 

  template <typename T>
  struct CustomHash;

  template<>
  struct CustomHash<uint64_t> {
    size_t operator()(uint64_t a) const {
      return hash64_2(a);
    }    
  };

  template<class T>
  struct CustomHash<T*> {
    size_t operator()(T* a) const {
      return hash64_2((uint64_t) a);
    }    
  };

  template<>
  struct CustomHash<uint128_t> {
    size_t operator()(const uint128_t &a) const {
      return hash64_2(a>>64) + hash64_2(a & MASK);
    }    
  };

  template <class T>
  using POD =
    typename std::conditional<(sizeof(T) == 1), uint8_t,
      typename std::conditional<(sizeof(T) == 4), uint32_t,
        typename std::conditional<(sizeof(T) == 8), uint64_t,
          typename std::conditional<(sizeof(T) == 16), uint128_t,
              std::nullptr_t
          >::type
        >::type
      >::type
    >::type;

  template<typename T, typename Sfinae = void>
  struct Padded;

  // Use user-defined conversions to pad primitive types
  template<typename T>
  struct alignas(128) Padded<T, typename std::enable_if<std::is_fundamental<T>::value>::type> {
    Padded() = default;
    Padded(T _x) : x(_x) { }
    operator T() { return x; }
    T x;
  };

  // Use inheritance to pad class types
  template<typename T>
  struct alignas(128) Padded<T, typename std::enable_if<std::is_class<T>::value>::type> : public T {

  };

  inline void FENCE() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

  template <typename ET>
  inline ET load(ET* a) {    
  #if defined(MCX16)
    static_assert(sizeof(ET) <= 16, "Bad load length");
  #else
    static_assert(sizeof(ET) <= 8, "Bad load length");
  #endif

    if constexpr (sizeof(ET) <= 8) return *a;
  #if defined(MCX16)
    uint128_t ret = __sync_val_compare_and_swap(reinterpret_cast<uint128_t*>(a), ZERO, ZERO);
    return *reinterpret_cast<ET*>(&ret);
  #endif
  }

  template <typename ET>
  inline void non_racy_store(ET* a, ET b) {    
  #if defined(MCX16)
    static_assert(sizeof(ET) <= 16, "Bad store length");
  #else
    static_assert(sizeof(ET) <= 8, "Bad store length");
  #endif

    if (sizeof(ET) <= 8) {
      *a = b;
      return;
    }
  #if defined(MCX16)
    ET old = load(a);
    __sync_bool_compare_and_swap(reinterpret_cast<uint128_t*>(a), *reinterpret_cast<uint128_t*>(&old), *reinterpret_cast<uint128_t*>(&b));
  #endif
  }  

  template <typename ET>
  inline bool CAS(ET* a, const ET &oldval, const ET &newval) {
    static_assert(!std::is_same<POD<ET>, std::nullptr_t>::value, "Bad CAS length");
    return __sync_bool_compare_and_swap(reinterpret_cast<POD<ET>*>(a), *reinterpret_cast<const POD<ET>*>(&oldval), *reinterpret_cast<const POD<ET>*>(&newval));
  }

  template <typename ET>
  inline ET FAS(ET* a, const ET &newval) {
    static_assert(!std::is_same<POD<ET>, std::nullptr_t>::value, "Bad FAS length");
    ET ret;
    __atomic_exchange(reinterpret_cast<POD<ET>*>(a), reinterpret_cast<const POD<ET>*>(&newval), reinterpret_cast<const POD<ET>*>(&ret), __ATOMIC_RELAXED);
    return ret;
  }

#pragma GCC diagnostic pop

  namespace rand {
    thread_local static unsigned long x=123456789, y=362436069, z=521288629;

    unsigned long get_rand(void) {          //period 2^96-1
      unsigned long t;
      //x += utils::my_id;
      x ^= x << 16;
      x ^= x >> 5;
      x ^= x << 1;

      t = x;
      x = y;
      y = z;
      z = t ^ x ^ y;

      return z;
    }
  }
}
#endif /* UTILS_H */
