
#include <thread>
#include <iostream>
#include <fstream>
#include <memory>
#include <atomic>
#include <assert.h>

#include <atomic_ref_cnt/sw_copy.hpp>
#include <atomic_ref_cnt/sw_copy_unbounded.hpp>

using namespace std;

template <template<typename> typename Dest>
void test_sequential() {
  int a = 1;
  int b = 2;
  Dest<int> x(6);
  assert(x.read() == 6);

  x.write(5);
  assert(x.read() == 5);

  x.sw_copy(&a);
  assert(x.read() == 1);

  x.sw_copy(&b);
  assert(x.read() == 2);

  Dest<int> y;
  y.sw_copy(&b);
  assert(y.read() == 2);
}

const int M = 500000;

template <template<typename> typename Dest>
void test_concurrent() {
  Dest<int> x(100);
  int a = 100;

  thread other ([&] () {
    utils::my_id = 1;
    for(int i = 0; i < M; i++)
    {
      //cout << x.read() << endl;
      assert(x.read() >= 100);
      assert(x.read() <= 100 + M);
      a++;
    }
  });

  for(int i = 0; i < M; i++)
    x.sw_copy(&a);

  other.join();
}

template <template<typename> typename Dest>
void run_all_tests(){
  test_sequential<Dest>();
  test_concurrent<Dest>();
}

int main () {
  run_all_tests<destination_unbounded>();
  run_all_tests<destination>();
  return 0; 
}

  
