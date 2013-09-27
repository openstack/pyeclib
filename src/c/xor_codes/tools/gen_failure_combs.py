import optparse
import mpmath
import itertools
import sys
import random

def get_combinations(list, k):
  return itertools.combinations(list, k)

if len(sys.argv) != 3:
  print "Usage: %s <num_fragments> <num_combs>"
  sys.exit(1)

n = int(sys.argv[1])
k = int(sys.argv[2])

if n is None or k is None:
  print "Usage: %s <num_fragments> <num_combs>"
  sys.exit(1)

fragments = [i for i in range(n)]
fragment_combinations = []

for i in range(1, k+1):
  fragment_combinations.extend([list(comb) for comb in get_combinations(fragments, i)])

for comb in fragment_combinations:
  while len(comb) < 4:
    comb.append(-1)

failure_comb_format_str = "int failure_combs_%d_%d[NUM_%d_%d_COMBS][%d] =  %s ;"

fragment_combination_str = ("%s" % fragment_combinations).replace("[", "{").replace("]", "}")

print "#define NUM_%d_%d_COMBS %d" % (n, k+1, len(fragment_combinations))
print failure_comb_format_str % (n, k+1, n, k+1, 4, fragment_combination_str)
