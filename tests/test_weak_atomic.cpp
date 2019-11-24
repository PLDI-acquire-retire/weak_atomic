#include <thread>
#include <iostream>
#include <fstream>
#include <memory>
#include <atomic>
#include <assert.h>

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

/*
  Reference counted pointer to node
*/
struct node_ptr {
  node* n;

  node_ptr() : n(nullptr) {};

  // normal constructor
  node_ptr(node* _n) : n(_n) {
    n->count.fetch_add(1, std::memory_order_seq_cst);
  }
  node_ptr(int key, int val) : n(new node(key, val)) {
    n->count.fetch_add(1, std::memory_order_seq_cst);
  }

  // copy constructor
  node_ptr(const node_ptr& _ptr) {
    n = _ptr.n;
    if(n != nullptr) 
      n->count.fetch_add(1, std::memory_order_seq_cst);
  }

  // move constructor
  node_ptr(node_ptr&& _ptr) {
    n = _ptr.n;
    _ptr.n = nullptr;
  }

  // destructor
  ~node_ptr() {
    if(n == nullptr) return;
    int c = n->count.fetch_add(-1, std::memory_order_seq_cst);
    if(c == 1) {
      delete n;
      n = nullptr;
    }
  }

  // copy assignment
  node_ptr& operator=( const node_ptr& r ) {
    node* tmp = n;
    n = r.n;
    if(n != nullptr) 
      n->count.fetch_add(1, std::memory_order_seq_cst);
    if(tmp != nullptr) {
      int c = tmp->count.fetch_add(-1, std::memory_order_seq_cst);
      if(c == 1) delete tmp;
    }
    return *this;
  }

  // move assignment
  node_ptr& operator=( node_ptr&& r ) {
    node* tmp = n;
    n = r.n;
    r.n = nullptr;
    if(tmp != nullptr) {
      int c = tmp->count.fetch_add(-1, std::memory_order_seq_cst);
      if(c == 1) delete tmp;
    }
    return *this;
  }

  node* get() {
    return n;
  }
};

const int M = 500000;

template <template<typename> typename AcquireRetire>
void simple_test() {
  node_ptr p1(new node(1, 2));
  node_ptr p2(new node(3, 4));
  node_ptr p3(std::move(p1));
  node_ptr p4(p2);
  
  p2 = p3;

  assert(p1.get() == nullptr);
  assert(p2.get()->key == 1);
  assert(p3.get()->key == 1);
  assert(p4.get()->key == 3);

  weak_atomic<node_ptr, AcquireRetire> atomic_ptr(5, 6);
  thread other ([&] () {
      utils::my_id = 1;
      node_ptr a1(atomic_ptr.load());
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      node_ptr a2(atomic_ptr.load());
      cout << "a1 key = " << a1.get()->key << endl;
      if(a2.get() != nullptr)
        cout << "a2 key = " << a2.get()->key << endl;
  });
  node_ptr b1(atomic_ptr.load());
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
  weak_atomic<node_ptr, AcquireRetire> atomic_ptr(0, 0);
  atomic<long long int> sum(0);
  
  utils::my_id = 0;
  thread other1 ([&] () {
    utils::my_id = 1;
    thread other2 ([&] () {
        utils::my_id = 2;
        for(int i = 0; i < M; i++) {
          node_ptr n(atomic_ptr.load());
          sum += n.get()->key;
        }
    });

    for(int i = 0; i < M; i++) {
      node_ptr n(atomic_ptr.load());
      sum += n.get()->key;
    }
    other2.join();
  });

  for(int i = 0; i < M; i++) {
    node_ptr old(atomic_ptr.load());
    node_ptr new_node(i, i);
    atomic_ptr.cas(old, new_node); 
  }
  other1.join();

  cout << sum << endl;

  atomic_ptr.clear_all();
}

// one writer, one reader, and one CASer
template <template<typename> typename AcquireRetire>
void stress_test2() {
  weak_atomic<node_ptr, AcquireRetire> atomic_ptr(0, 0);
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
      atomic_ptr.cas(atomic_ptr.load(), node_ptr(i, i)); 
    }
    other2.join();
  });

  for(int i = 0; i < M; i++) {
    atomic_ptr.store(node_ptr(i, i)); 
  }
  other1.join();

  cout << sum << endl;

  atomic_ptr.clear_all();
}

template <template<typename> typename AcquireRetire>
void run_all_tests() {
  simple_test<AcquireRetire>();
  stress_test1<AcquireRetire>();
  stress_test2<AcquireRetire>();
}

int main () {
  run_all_tests<AcquireRetireWaitfree>();
  run_all_tests<AcquireRetireWaitfreeUnbounded>();
  run_all_tests<AcquireRetireLockfree>();
}

  
