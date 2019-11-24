# Atomic reference-counted pointers for C++

## How to build

./configure
cd build/release
make

## Notes

Make sure you have a very recent version of cmake. Something like 3.16.0
You have to compile with g++-7 to g++-9 to use just threads. It does not support anything newer than clang 5.
