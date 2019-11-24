#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <ctype.h>
#include <memory>
#include <stdlib.h>
#include <type_traits>
#include <math.h>
#include <atomic>
#include <cstring>

#define MAX_THREADS 150

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;
const uint128_t ZERO = uint128_t(0);

namespace utils {
  // easy to forget to set this upon thread creation
  thread_local int my_id = 0;

  template <class T>
  using POD =
    typename std::conditional<(sizeof(T) == 1), uint8_t,
      typename std::conditional<(sizeof(T) == 4), uint32_t,
        typename std::conditional<(sizeof(T) == 8), uint64_t,
          typename std::conditional<(sizeof(T) == 16), uint128_t,
              nullptr_t
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

  template <typename T>
  struct alignas(128) cache_aligned {
    T x;
    // Implicit conversion to underlying type
    operator T() const { return x; }
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

    if (sizeof(ET) <= 8) return *a;
  #if defined(MCX16)
    uint128_t ret = __sync_val_compare_and_swap(reinterpret_cast<uint128_t*>(a), ZERO, ZERO);
    return *reinterpret_cast<ET*>(&ret);
  #endif
  }

#pragma GCC diagnostic pop

  template <typename ET>
  inline bool CAS(ET* a, const ET &oldval, const ET &newval) {
    static_assert(!std::is_same<POD<ET>, nullptr_t>::value, "Bad CAS length");
    return __sync_bool_compare_and_swap(reinterpret_cast<POD<ET>*>(a), *reinterpret_cast<const POD<ET>*>(&oldval), *reinterpret_cast<const POD<ET>*>(&newval));
  }

  // template <typename ET>
  // ET bool FAS(ET* a, const ET &newval) {
  //   static_assert(!std::is_same<POD<ET>, nullptr_t>::value, "Bad FAS length");
  //   return __sync_bool_compare_and_swap(reinterpret_cast<POD<ET>*>(a), *reinterpret_cast<const POD<ET>*>(&oldval), *reinterpret_cast<const POD<ET>*>(&newval));
  // }

  namespace rand {
    thread_local static unsigned long x=123456789, y=362436069, z=521288629;

    unsigned long get_rand(void) {          //period 2^96-1
      unsigned long t;
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
