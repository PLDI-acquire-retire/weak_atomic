
#include <thread>
#include <iostream>
#include <fstream>
#include <memory>
#include <atomic>
#include <assert.h>
#include <thread>
#include <iostream>
#include <fstream>

#include <other/ref_counting.hpp>

using namespace std;

const int M = 100000;

struct node {
  atomic<int> cnt;
  int val;
  ref_ptr<node> Left, Right;

  node(int v, ref_ptr<node> L, ref_ptr<node> R) :
    cnt(0), val(v), Left(L), Right(R) {}

  int add_counter(int i) {
    return cnt.fetch_add(i, std::memory_order_seq_cst);
  }
};

void example_use() {
  using np = ref_ptr<node>;

  // create tree Tr with three nodes
  np Tr(new node(5, 
          np(new node(3, nullptr, nullptr)),
          np(new node(7, nullptr, nullptr))));

  // update Tr in another thread 
  thread other([&] () {
    Tr.update(np(new node(11,nullptr,nullptr)));});

  // copy Tr into Tr2, races with update
  np Tr2(Tr);

  // join thread
  other.join();

  Tr.clear_all();
}

void test_par() {
  volatile long long sum = 0;
  using np = ref_ptr<node>;

  np Tr(new node(5, 
          np(new node(3, nullptr, nullptr)),
          np(new node(7, nullptr, nullptr))));

  thread other ([&] () {
    utils::my_id = 1;
    for(int i = 0; i < M; i++)
      Tr.update(np(new node(i,nullptr,nullptr)));
  });

  for(int i = 0; i < M; i++) {
    Tr.template with_ptr<void>([&sum] (node* p) {
      sum += p->val;
    });
  }

  other.join();

  Tr.clear_all();

  cout << sum << endl;
}

int main () {
  example_use();
  test_par();
}

  
