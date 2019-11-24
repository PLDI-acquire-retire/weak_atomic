#include <thread>
#include <iostream>
#include <fstream>
#include <memory>
#include <atomic>

#include <atomic_ref_cnt/weak_atomic.hpp>
#include <atomic_ref_cnt/acquire_retire_waitfree.hpp>
#include <atomic_ref_cnt/acquire_retire_lockfree.hpp>

using namespace std;

struct node {
  int key, val;
  atomic<int> count;
  node *left, *right;

  node() : key(0), val(0), count(0), left(nullptr), right(nullptr) {};
  node(int _key, int _val) : key(_key), val(_val), count(0), left(nullptr), right(nullptr) {};
};


const int M = 500000;

template <template<typename> typename AcquireRetire>
void simple_test() {
  shared_ptr<node> p1(new node(1, 2));
  shared_ptr<node> p2(new node(3, 4));
  shared_ptr<node> p3(std::move(p1));
  shared_ptr<node> p4(p2);
  
  p2 = p3;

  cout << p1.get() << endl;
  cout << p2.get()->key << endl;
  cout << p3.get()->key << endl;
  cout << p4.get()->key << endl;

  weak_atomic<shared_ptr<node>, AcquireRetire> atomic_ptr(new node(5, 6));
  thread other ([&] () {
      utils::my_id = 1;
      shared_ptr<node> a1(atomic_ptr.load());
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      shared_ptr<node> a2(atomic_ptr.load());
      cout << "a1 key = " << a1.get()->key << endl;
      if(a2.get() != nullptr)
        cout << "a2 key = " << a2.get()->key << endl;
  });
  shared_ptr<node> b1(atomic_ptr.load());
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  atomic_ptr.cas(b1, p1);
  atomic_ptr.cas(p1, p4);
  other.join();
  atomic_ptr.clear_all();
  cout << atomic_ptr.load().get()->key << endl;
}

// two readers one updater
template <template<typename> typename AcquireRetire>
void stress_test1() {
  weak_atomic<shared_ptr<node>, AcquireRetire> atomic_ptr(new node(0,0));
  atomic<long long int> sum(0);
  
  utils::my_id = 0;
  thread other1 ([&] () {
    utils::my_id = 1;
    thread other2 ([&] () {
        utils::my_id = 2;
        for(int i = 0; i < M; i++) {
          shared_ptr<node> n(atomic_ptr.load());
          sum += n.get()->key;
        }
    });

    for(int i = 0; i < M; i++) {
      shared_ptr<node> n(atomic_ptr.load());
      sum += n.get()->key;
    }
    other2.join();
  });

  for(int i = 0; i < M; i++) {
    shared_ptr<node> old(atomic_ptr.load());
    shared_ptr<node> new_node(new node(i,i));
    atomic_ptr.cas(old, new_node); 
  }
  other1.join();

  atomic_ptr.clear_all();

  cout << sum << endl;
}

// one writer, one reader, and one CASer
template <template<typename> typename AcquireRetire>
void stress_test2() {
  weak_atomic<shared_ptr<node>, AcquireRetire> atomic_ptr(new node(0, 0));
  atomic<long long int> sum(0);

  utils::my_id = 0;
  thread other1 ([&] () {
    utils::my_id = 1;
    thread other2 ([&] () {
        utils::my_id = 2;
        for(int i = 0; i < M; i++) {
          sum += atomic_ptr.load().get()->key;
        }
    });

    for(int i = 0; i < M; i++) {
      atomic_ptr.cas(atomic_ptr.load(), shared_ptr<node>(new node(i, i))); 
    }
    other2.join();
  });

  for(int i = 0; i < M; i++) {
    atomic_ptr.store(shared_ptr<node>(new node(i, i))); 
  }
  other1.join();

  atomic_ptr.clear_all();

  cout << sum << endl;
}

template <template<typename> typename AcquireRetire>
void run_all_tests() {
  simple_test<AcquireRetire>();
  stress_test1<AcquireRetire>();
  stress_test2<AcquireRetire>();
}

int main () {
  run_all_tests<AcquireRetireWaitfree>();
  run_all_tests<AcquireRetireLockfree>();
}

  
