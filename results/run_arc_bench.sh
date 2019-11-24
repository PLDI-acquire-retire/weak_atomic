#!/bin/bash

mkdir arc_results

# backup old results
cp arc_results/results_bench_trival.txt arc_results/results_bench_trival.old
cp arc_results/results_bench_load_only.txt arc_results/results_bench_load_only.old

../build/release/benchmarks/bench_trivial > arc_results/results_bench_trivial.txt
../build/release/benchmarks/bench_load_only > arc_results/results_bench_load_only.txt
