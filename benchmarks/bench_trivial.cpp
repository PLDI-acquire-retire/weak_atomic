
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>

#include "common.hpp"

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
struct TrivialBenchmark : Benchmark {

  TrivialBenchmark(int _P) : Benchmark(_P) {
    SPType<DataType<int>> sp(new DataType<int>(0));
    asp.store(sp);
  }
  
  void bench() override {
    std::vector<long long int> cnt(P);
    std::vector<std::thread> threads;

    std::atomic<bool> start = false, done = false;
    
    for (int p = 0; p < P; p++) {
      threads.emplace_back([&start, &done, this, &cnt, p]() {
        utils::my_id = p + 1;
        while (!start) { }
        
        long long int ops = 0;
        
        for (; !done; ops++) {
          SPType<DataType<int>> x = asp.load();
          int new_val = *x + 1;
          SPType<DataType<int>> y(new DataType<int>(new_val));
          asp.store(std::move(y));
        }
        
        cnt[p] = ops;
      });
    }
    
    // Run benchmark for one second
    start = true;
    start_timer();
    while (read_timer() < 1) { }
    done = true;
    
    for (auto& t : threads) t.join();

    
    // Read results
    long long int total = std::accumulate(std::begin(cnt), std::end(cnt), 0LL);
    std::cout << "Total operations = " << total/1000000.0 << "M in 1 second" << std::endl;
  }

  AtomicSPType<DataType<int>> asp;
};

int main() {
  run_all_benchmarks<TrivialBenchmark>();
}


