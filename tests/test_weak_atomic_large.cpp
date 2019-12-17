#include <thread>
#include <iostream>
#include <fstream>
#include <memory>
#include <atomic>
#include <vector>

#include <atomic_ref_cnt/weak_atomic.hpp>
#include <atomic_ref_cnt/acquire_retire_waitfree.hpp>
#include <atomic_ref_cnt/acquire_retire_lockfree.hpp>

using namespace std;

const int M = 50000;

template <template<typename> typename AcquireRetire>
void simple_test() {
  weak_atomic<vector<int>, AcquireRetire> vec(5, 6);
  thread other ([&] () {
      utils::my_id = 1;
      vector<int> a1(vec.load());
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      vector<int> a2(vec.load());
      cout << "a1 key = " << a1[0] << endl;
      cout << "a2 key = " << a2[0] << endl;
  });
  vector<int> b1(vec.load());
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  vector<int> n1(4, 1);
  vector<int> n2(3, 2);
  vec.store(n1);
  vec.store(n2);
  other.join();
  vec.clear_all();
  cout << vec.load()[0] << endl;
}

// two readers one updater
template <template<typename> typename AcquireRetire>
void stress_test1() {
  weak_atomic<vector<int>, AcquireRetire> vec(10, 0);
  atomic<long long int> sum(0);

  utils::my_id = 0;
  thread other1 ([&] () {
    utils::my_id = 1;
    thread other2 ([&] () {
        utils::my_id = 2;
        for(int i = 0; i < M; i++) {
          vector<int> n(vec.load());
          sum += n[0];
        }
    });

    for(int i = 0; i < M; i++) {
      vector<int> n(vec.load());
      sum += n[0];
    }
    other2.join();
  });

  for(int i = 0; i < M; i++) {
    vector<int> old(vec.load());
    vector<int> new_vec(5, i);
    vec.store(new_vec); 
  }
  other1.join();

  vec.clear_all();

  cout << sum << endl;
}

template <template<typename> typename AcquireRetire>
// one writer, one reader, and one CASer
void stress_test2() {
  weak_atomic<vector<int>, AcquireRetire> vec(10, 0);
  atomic<long long int> sum(0);

  utils::my_id = 0;
  thread other1 ([&] () {
    utils::my_id = 1;
    thread other2 ([&] () {
        utils::my_id = 2;
        for(int i = 0; i < M; i++) {
          sum += vec.load()[0];
        }
    });

    for(int i = 0; i < M; i++) {
      vec.store(vector<int>(5, i)); 
    }
    other2.join();
  });

  for(int i = 0; i < M; i++) {
    vec.store(vector<int>(5, i)); 
  }
  other1.join();

  vec.clear_all();

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
