#!/bin/bash

mkdir build
mkdir build/debug
mkdir build/release

cd build/debug
cmake ../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=gold -DCMAKE_CXX_COMPILER=g++-9
cd ../..

cd build/release
cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=gold -DCMAKE_CXX_COMPILER=g++-9
cd ../..
