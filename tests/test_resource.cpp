#include <thread>
#include <iostream>
#include <fstream>
#include <memory>
#include <atomic>
#include <assert.h>
#include <thread>
#include <iostream>
#include <fstream>

#include <other/resource.hpp>

using namespace std;

void example_use() {
  protect<std::ofstream> outstr("monday_log.txt", std::ofstream::out);
  thread other ([&] () {
    utils::my_id = 1;
    outstr.use<void>([] (ofstream* str) {
      *str << "hello world" << endl;
    });
  });
  std::ofstream* new_outstr = new std::ofstream("tuesday_log.txt");
  outstr.redirect(new_outstr);
  other.join();
}

const int M = 100000;

void stress_test() {
  volatile long long sum = 0;
  protect<vector<int> > vec(3, 100);
  thread other ([&] () {
    utils::my_id = 1;
    for(int i = 0; i < M; i++)
    {
      vector<int> *new_vec = new vector<int>(4, i);
      vec.redirect(new_vec);
    }
  });

  for(int i = 0; i < M; i++) {
    sum += vec.use<int>([&sum] (vector<int>* v) {
      return (*v)[0];
    });
  }

  other.join();

  cout << sum << endl;
}

int main () {
  example_use();
  stress_test();
}

  
