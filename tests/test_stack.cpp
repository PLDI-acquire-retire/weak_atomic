
#include <thread>
#include <iostream>
#include <fstream>
#include <memory>
#include <atomic>
#include <assert.h>
#include <thread>
#include <iostream>
#include <fstream>

#include <other/stack.hpp>

using namespace std;

const int M = 100000;

void test_seq() {
  Stack<int> stack;
  assert(!stack.pop());
  stack.push(5);
  assert(stack.pop().value() == 5);
  assert(!stack.peek());
  assert(!stack.pop());
  stack.push(5);
  stack.push(6);
  stack.push(7);
  assert(stack.peek().value() == 7);
  assert(stack.pop().value() == 7);
  assert(stack.pop().value() == 6);
  assert(stack.peek().value() == 5);
}

void test_par() {
  volatile long long sum = 0;
  Stack<int> stack;
  thread other ([&] () {
    utils::my_id = 1;
    for(int i = 0; i < M; i++)
      stack.push(i);
  });

  thread other2 ([&] () {
    utils::my_id = 2;
    for(int i = 0; i < M; i++)
      stack.pop();
  });

  for(int i = 0; i < M; i++) {
    auto v = stack.peek();
    if(v)
      sum += v.value();
  }

  other.join();
  other2.join();

  cout << sum << endl;
}

int main () {
  test_seq();
  test_par();
}

  
