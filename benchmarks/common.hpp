
#ifndef BENCHMARKS_COMMON_HPP_
#define BENCHMARKS_COMMON_HPP_

#include <atomic>
#include <chrono>
#include <iostream>

// Just::threads
#include <experimental/atomic>

#include <atomic_ref_cnt/weak_atomic.hpp>
#include <atomic_ref_cnt/my_shared_ptr.hpp>
#include <atomic_ref_cnt/acquire_retire_waitfree.hpp>
#include <atomic_ref_cnt/acquire_retire_lockfree.hpp>

// ==================================================================
//                  Shared pointer implementations
// ==================================================================

// C++ standard library atomic support for shared ptrs
template<typename T>
struct StlAtomicSharedPtr {

  StlAtomicSharedPtr() = default;
  StlAtomicSharedPtr(const std::shared_ptr<T>& other) : sp(other) { }
  std::shared_ptr<T> load() { return std::atomic_load(&sp); }
  void store(std::shared_ptr<T> r) { std::atomic_store(&sp, r); }
  bool compare_exchange_strong(std::shared_ptr<T>& expected, std::shared_ptr<T> desired) {
    return atomic_compare_exchange_strong(&sp, expected, desired);
  }
  
  std::shared_ptr<T> sp;
};

// Just::threads atomic support for shared ptrs
template<typename T>
using JssAtomicSharedPtr = std::experimental::atomic_shared_ptr<T>;

// Our weak atomic wrapper around a standard library shared pointer
template<typename T>
using ArcAtomicSharedPtrWaitfree = weak_atomic<std::shared_ptr<T>, AcquireRetireWaitfree>;

// Our weak atomic wrapper around a home-brewed shared pointer
template<typename T>
using ArcAtomicMySharedPtrWaitfree = weak_atomic<my_shared_ptr<T>, AcquireRetireWaitfree>;

// Our weak atomic wrapper around a standard library shared pointer
template<typename T>
using ArcAtomicSharedPtrLockfree = weak_atomic<std::shared_ptr<T>, AcquireRetireLockfree>;

// Our weak atomic wrapper around a home-brewed shared pointer
template<typename T>
using ArcAtomicMySharedPtrLockfree = weak_atomic<my_shared_ptr<T>, AcquireRetireLockfree>;

// Wrappers for redundant types
template<typename T>
using SameType = T;

template<typename T>
using RawPtr = T*;


// ==================================================================
//                     Benchmarking framework
// ==================================================================

// Returns the sequence of core numbers to be used for benchmarking.
// Uses powers of 2 until the maximum number of cores, plus double
// the number of cores for hyperthreading
std::vector<int> get_core_series() {
  const auto num_cores = static_cast<int>(std::thread::hardware_concurrency() / 2);
  std::vector<int> ps; 
  for (int p = 1; p < num_cores; p = 2*p) ps.push_back(p);
  ps.push_back(num_cores);
  ps.push_back(num_cores * 2);
  return ps;
}

// Generic base class for benchmarks
// Benchmarks should implement the bench() function, which
// runs the benchmark with P threads.
struct Benchmark {
  
  Benchmark(int _P) : P(_P) { }

  void start_timer() {
    start = std::chrono::high_resolution_clock::now();
  }

  double read_timer() {
    auto now = std::chrono::high_resolution_clock::now();
    double elapsed_seconds = std::chrono::duration<double>(now - start).count();
    return elapsed_seconds;
  }

  virtual void bench() = 0;
  virtual ~Benchmark() = default;
  
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  int P;    // Number of threads
};

// Run a specific benchmark with a specific shared ptr implementation on varying thread counts
template<template<template<typename> typename, template<typename> typename, template<typename> typename> typename BenchmarkType, template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
void run_benchmarks(const std::string& name) {
  std::cout << "----------------------------------------------------------------" << std::endl;
  std::cout << "\t\t" << name << std::endl;
  std::cout << "--------------------------------------------------------------" << std::endl;
  for (int p : get_core_series()) {
    std::cout << "****** p = " << p << " ******" << std::endl;
    BenchmarkType<AtomicSPType, SPType, DataType> benchmark(p);
    benchmark.bench();
  }
}

// Run the given benchmark with all shared ptr implementations
template<template<template<typename> typename, template<typename> typename, template<typename> typename> typename BenchmarkType>
void run_all_benchmarks() {
  run_benchmarks<BenchmarkType, JssAtomicSharedPtr, std::experimental::shared_ptr, SameType>("Just::threads atomic shared ptr");
  run_benchmarks<BenchmarkType, StlAtomicSharedPtr, std::shared_ptr, SameType>("libstdc++ atomic shared ptr");
  run_benchmarks<BenchmarkType, ArcAtomicSharedPtrWaitfree, std::shared_ptr, SameType>("ARC Weak Atomic std::shared_ptr (waitfree)");
  run_benchmarks<BenchmarkType, ArcAtomicMySharedPtrWaitfree, my_shared_ptr, reference_counted_t>("ARC Weak Atomic my_shared_ptr (waitfree)");
  run_benchmarks<BenchmarkType, ArcAtomicSharedPtrLockfree, std::shared_ptr, SameType>("ARC Weak Atomic std::shared_ptr (lockfree)");
  run_benchmarks<BenchmarkType, ArcAtomicMySharedPtrLockfree, my_shared_ptr, reference_counted_t>("ARC Weak Atomic my_shared_ptr (lockfree)");
}

#endif  // BENCHMARKS_COMMON_HPP_

