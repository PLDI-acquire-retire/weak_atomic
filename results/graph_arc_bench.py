import sqlite3
import os.path
from os.path import isdir
from os import mkdir
import numpy as np
import matplotlib as mpl
mpl.use('Agg')

import pandas as pd
import matplotlib.pyplot as plt
#plt.style.use('ggplot')
import sys
import math
import re

font = {'weight' : 'normal',
      'size'   : 14}
mpl.rc('font', **font)

indir = "arc_results/"
outdir = "arc_graphs/"
if not os.path.exists(outdir):
  os.mkdir(outdir)

def to_string2(alg, th):
  return alg + ", " + str(th)

def to_string(d):
  return to_string2(d['alg'], str(d['th']))

def to_string_list(int_list):
  return [str(x) for x in int_list]

def firstInt(s):
  for t in s.split():
    try:
      return int(t)
    except ValueError:
      pass
  print("no number in string")

def firstFloat(s):
  for t in s.split():
    try:
      return float(t)
    except ValueError:
      pass
  print("no number in string")

def add_result(results, key_dict, num):
  key = to_string(key_dict)
  results[key] = num

def create_graph(results, algs, threads, graph_file, graph_title):
  series = {}
  for alg in algs:
    series[alg] = []
    for th in threads:
      series[alg].append(results[to_string2(alg, th)])
  # create plot
  fig, ax = plt.subplots()
  opacity = 0.8
  rects = {}
   
  offset = 0
  for alg in algs:
    rects[alg] = plt.plot(to_string_list(threads), series[alg],
    alpha=opacity,
    #color=color[get_type(ds)],
    #hatch=hatch[ds],
    marker="o",
    label=alg)
   
  plt.xlabel('Number of threads')
  plt.ylabel('Throughput (Mop/s)')
  plt.title(graph_title)
  plt.semilogy()

  legend_x = 1
  legend_y = 0.5
  plt.legend(loc='center left', bbox_to_anchor=(legend_x, legend_y))
   
  #plt.tight_layout()
  #plt.show()

  plt.savefig(outdir+graph_file, bbox_inches='tight')
  plt.close('all')
  # print "done"

def graph_results_from_file(results_file, graph_file, graph_title):
  results = {}
  key = {}
  algs = []
  threads = []

  alg_name = 0

  with open(indir + results_file) as f:
    for line in f:
      if alg_name == 2:
        alg_name = 0
        continue
      if alg_name == 1:
        alg_name = 2
        key['alg'] = line.strip().replace(') with unbounded sequence numbers', ' unbounded)').replace('ARC ', '')
        if key['alg'] not in algs:
          algs.append(key['alg'])        

      if line.find('---------------') >= 0:
        alg_name = 1
      elif line.find(" p = ") >= 0:
        key['th'] = firstInt(line)
        if key['th'] not in threads:
          threads.append(key['th'])
      elif line.find("Total operations = ") >= 0:
        add_result(results, key, firstFloat(line.replace('M', '')))
  # print(algs)
  # print(threads)
  # print(results)
  create_graph(results, algs, threads, graph_file, graph_title)

graph_results_from_file("results_bench_trivial.txt", "bench_trivial.png", "ARC Load-Store Benchmark")
graph_results_from_file("results_bench_load_only.txt", "bench_load_only.png", "ARC Load-Only Benchmark")


# for th in threads:
#   s = str(th) + "\t"
#   d = {}
#   d['th'] = th
#   for alg in algs:
#     d['alg'] = alg
#     s += str(results[to_string(d)][0]) + "\t"
#   print(s)


