# Acquire-Retire: Protecting Read-Destruct Races in Constant-Time and Bounded-Space

## Table of Contents

Acquire Retire (Figure 3 in paper):
  - Implementation in [include/atomic_ref_cnt/acquire_retire_waitfree.hpp](include/atomic_ref_cnt/acquire_retire_waitfree.hpp)

Resource (Figure 2 in paper):
  - Implementation in [include/other/resource.hpp](include/other/resource.hpp)
  - Example use in [tests/test_resource.cpp](ests/test_resource.cpp)

Protected Block Memory Reclamation (Figure 4 in paper):
  - Implementation in [include/other/memory_reclamation.hpp](include/other/memory_reclamation.hpp)
  - Example use in [include/other/stack.hpp](include/other/stack.hpp)

Stack with wait-free peek (Figure 5 in paper):
  - Implementation in [include/other/stack.hpp](include/other/stack.hpp)
  - Example use in [tests/test_stack.cpp](tests/test_stack.cpp)

Reference Counting (Figure 6 in paper):
  - Implementation in [include/other/ref_counting.hpp](include/other/ref_counting.hpp)
  - Example use in [tests/test_ref_counting.cpp](tests/test_ref_counting.cpp)

Weak Atomic (Figure 7 in paper):
  - Implementation in [include/atomic_ref_cnt/weak_atomic_small.hpp](include/atomic_ref_cnt/weak_atomic_small.hpp)
  - Example use in [tests/test_weak_atomic.cpp](tests/test_weak_atomic.cpp)
  - Example use in [tests/test_weak_atomic_16_byte.cpp](tests/test_weak_atomic_16_byte.cpp)

## Running Benchmarks from Paper

### How to build

You'll need CMake. There are several kinds of builds that you can use.

### Configure Release (optimized) build using CMake

Optimized builds should be used for running benchmarks.

```
mkdir build && cd build
mkdir Release && cd Release
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make
```

Depending on your compiler, you may have to add `-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=gold` to the cmake command. Clang works fine since it uses a different linker, but GCC might fail to link without this.

### Configure Debug build using CMake

Debug builds should be used for running unit tests and for debugging issues with the benchmarks.

```
mkdir build && cd build
mkdir Release && cd Release
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
make
```

See the notes about changing the linker above.

### Configuring RelWithDebInfo (profiling) build using CMake

For profiling, you should use the RelWithDebInfo configuration. Do the same as above but with
RelWithDebInfo in place of Debug/Release. RelWithDebInfo compiles optimized code to ensure
that realistic performance is measured, but also includes debugging information in the binary
so that profiling tools can correctly analyze the programs behaviour.

### Configure sensible defaults automatically

A script is provided to make release and debug builds configured as is presently used for benchmarking.

```
./configure
cd build/release
make
```

### Notes

Make sure you have a very recent version of cmake. At least 3.5 is required. This may be higher
than what comes in your package manager so you could have to install it manually from [Here](https://cmake.org).

