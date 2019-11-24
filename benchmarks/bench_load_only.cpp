
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
#include <stdlib.h>

#include "common.hpp"

using namespace std;

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType,
int N=1, int store_percent=0, int cas_percent=0>
struct LoadOnlyBenchmark : Benchmark {

  LoadOnlyBenchmark(int _P)
    : Benchmark(_P), asp_vec(N) {
    for(int i = 0; i < N; i++)
      asp_vec[i].store(SPType<DataType<int>>(new DataType<int>(3)));
  }
  
  void bench() override {
    std::vector<long long int> cnt(P);
    std::vector<std::thread> threads;

    std::atomic<bool> start = false, done = false;
    
    //cout << N << endl;

    for (int p = 0; p < P; p++) {
      threads.emplace_back([&start, &done, this, &cnt, p]() {
        utils::my_id = p + 1;
        long long r = 0;

        while (!start) { }
        
        long long int ops = 0;
        volatile long long int sum = 0;
        
        for (; !done; ops++) {
          //r = utils::rand::get_rand();
          r = r+1;
          int op = r%100;
          int asp_index = r%N;
          //cout << asp_index << endl;
          if(op < store_percent){ // store
            SPType<DataType<int>> x = asp_vec[asp_index].load();
            int new_val = *x + 1;
            SPType<DataType<int>> y(new DataType<int>(new_val));
            asp_vec[asp_index].store(std::move(y));
          } else if(op < store_percent + cas_percent) {  // CAS
            cerr << "not implemented" << endl;
            exit(1);
            // TODO: implement this
            // SPType<DataType<int>> x = asp_vec[asp_index].load();
            // int new_val = *x + 1;
            // SPType<DataType<int>> y(new DataType<int>(new_val));
            // asp_vec[asp_index].compare_exchange_strong(x, std::move(y));
          } else {  // load
            SPType<DataType<int>> x = asp_vec[asp_index].load();
            sum += *x;            
          }
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



  //vector of atomic shared pointers

  vector<Padded<AtomicSPType<DataType<int>>>> asp_vec;
};

static void print_bench_name(int N, int store_percent, int cas_percent) {
  std::cout << "----------------------------------------------------------------" << std::endl;
  std::cout << "\t\tMicro-benchmark: N = " << N << ", stores = " << 
                      store_percent << ", CASes = " << cas_percent << std::endl;
  std::cout << "--------------------------------------------------------------" << std::endl;
}

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
using LoadOnlyBenchmark1_0 = LoadOnlyBenchmark<AtomicSPType, SPType, DataType, 1, 0, 0>;

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
using LoadOnlyBenchmark100_0 = LoadOnlyBenchmark<AtomicSPType, SPType, DataType, 100, 0, 0>;

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
using LoadOnlyBenchmark1000000_0 = LoadOnlyBenchmark<AtomicSPType, SPType, DataType, 1000000, 0, 0>;

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
using LoadOnlyBenchmark1_10 = LoadOnlyBenchmark<AtomicSPType, SPType, DataType, 1, 10, 0>;

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
using LoadOnlyBenchmark100_10 = LoadOnlyBenchmark<AtomicSPType, SPType, DataType, 100, 10, 0>;

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
using LoadOnlyBenchmark1000000_10 = LoadOnlyBenchmark<AtomicSPType, SPType, DataType, 1000000, 10, 0>;

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
using LoadOnlyBenchmark1_50 = LoadOnlyBenchmark<AtomicSPType, SPType, DataType, 1, 50, 0>;

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
using LoadOnlyBenchmark100_50 = LoadOnlyBenchmark<AtomicSPType, SPType, DataType, 100, 50, 0>;

template<template<typename> typename AtomicSPType, template<typename> typename SPType, template<typename> typename DataType>
using LoadOnlyBenchmark1000000_50 = LoadOnlyBenchmark<AtomicSPType, SPType, DataType, 1000000, 50, 0>;


int main() {
  print_bench_name(1000000, 0, 0);
  run_all_benchmarks<LoadOnlyBenchmark1000000_0>();
  print_bench_name(1000000, 10, 0);
  run_all_benchmarks<LoadOnlyBenchmark1000000_10>();
  print_bench_name(1000000, 50, 0);
  run_all_benchmarks<LoadOnlyBenchmark1000000_50>();
}


