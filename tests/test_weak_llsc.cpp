#include <thread>
#include <iostream>
#include <fstream>
#include <memory>
#include <atomic>
#include <assert.h>

#include <atomic_ref_cnt/weak_llsc.hpp>

using namespace std;

void test_sequential() {
  weak_llsc<int> X(6);
  optional<int> x;
  
  x = X.weak_LL();
  assert(x.value_or(0) == 6);

  X.SC(10);
  x = X.weak_LL();
  assert(x.value_or(0) == 10);

  weak_llsc<int> Y;
  Y.weak_LL();
  Y.SC(3);
  x = Y.weak_LL();
  assert(x.value_or(0) == 3);
}

const int M = 500000;

void test_concurrent() {
  weak_llsc<int> X(100);
  optional<int> x;

  thread other ([&] () {
    utils::my_id = 1;
    for(int i = 0; i < M; i++)
    {
      x = X.weak_LL();
      if(x)
      {
        assert(x.value_or(0) >= 100);
        assert(x.value_or(0) <= 100 + M);
      }
    }
  });

  for(int i = 0; i < M; i++)
  {
    int a = X.weak_LL().value();
    assert(a == 100+i);
    X.SC(a + 1);
  }

  other.join();
}

int main () {
  test_sequential();
  test_concurrent();
  return 0; 
}

  
