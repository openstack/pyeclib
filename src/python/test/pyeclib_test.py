import pyeclib
import time
import random
import string
import sys
import os

class Timer:
  def __init__(self):
    self.start_time = 0
    self.end_time = 0

  def start(self):
    self.start_time = time.time()

  def stop(self):
    self.end_time = time.time()

  def curr_delta(self):
    return self.end_time - self.start_time

  def stop_and_return(self):
    self.end_time = time.time()
    return self.curr_delta()

def setup(file_sizes):

  for size_str in file_sizes:
    filename = "test_file.%s" % size_str
    fp = open("test_files/%s" % filename, "w")

    size_desc = size_str.split("-")

    size = int(size_desc[0])

    if (size_desc[1] == 'M'):
      size *= 1000000
    elif (size_desc[1] == 'K'):
      size *= 1000

    buffer = ''.join(random.choice(string.letters) for i in range(size))

    fp.write(buffer)
    fp.close()

def cleanup(file_sizes):
  for size_str in file_sizes:
    filename = "test_files/test_file.%s" % size_str
    os.unlink(filename)

def time_encode(num_data, num_parity, w, type, file_size, iterations):
  filename = "test_file.%s" % file_size
  fp = open("test_files/%s" % filename, "r")
  timer = Timer()
  sum = 0
  handle = pyeclib.init(num_data, num_parity, w, type)

  whole_file_str = fp.read()

  timer.start()
  for i in range(iterations):
    fragments=pyeclib.encode(handle, whole_file_str)
  sum = timer.stop_and_return()

  return sum / iterations

def time_decode(num_data, num_parity, w, type, file_size, iterations):
  filename = "test_file.%s" % file_size
  fp = open("test_files/%s" % filename, "r")
  timer = Timer()
  sum = 0
  handle = pyeclib.init(num_data, num_parity, w, type)

  whole_file_str = fp.read()

  orig_fragments=pyeclib.encode(handle, whole_file_str)

  fragments = orig_fragments[:]

  for i in range(iterations):
    num_missing = num_parity
    missing_idxs = []
    for j in range(num_missing):
      idx = random.randint(0, (num_data+num_parity)-1)
      missing_idxs.append(idx)
      fragments[idx] = '\0' * len(fragments[0])
    missing_idxs.append(-1)

    timer.start()
    decoded_fragments = pyeclib.decode(handle, fragments[:num_data], fragments[num_data:], missing_idxs, len(fragments[0]))
 
    sum += timer.stop_and_return()
  
    for j in range(num_data+num_parity):
      if orig_fragments[j] != decoded_fragments[j]:
        fd_orig = open("orig_fragments", "w")
        fd_decoded = open("decoded_fragments", "w")

        fd_orig.write(orig_fragments[j])
        fd_decoded.write(decoded_fragments[j])

        fd_orig.close()
        fd_decoded.close()
        print "Fragment %d was not reconstructed!!!" % j
        sys.exit(2)


  return sum / iterations

def get_throughput(avg_time, size_str):
  size_desc = size_str.split("-")  

  size = float(size_desc[0])

  if (size_desc[1] == 'M'):
    return size / avg_time
  elif (size_desc[1] == 'K'):
    return (size / 1000.0) / avg_time
  
num_datas = [10, 10, 10] 
num_parities = [2, 3, 4]
iterations=100
#type="rs_cauchy_orig"
type="rs_vand"

sizes = ["10-K", "100-K", "1-M", "10-M"]

setup(sizes)

for i in range(len(num_datas)):
  y=[]
  for size_str in sizes:
    avg_time = time_encode(num_datas[i], num_parities[i], 16, type, size_str, iterations)
    #y.append(get_throughput(avg_time, size_str))
    print avg_time

for i in range(len(num_datas)):
  y=[]
  for size_str in sizes:
    avg_time = time_decode(num_datas[i], num_parities[i], 16, type, size_str, iterations)
    y.append(get_throughput(avg_time, size_str))
    print "Decode (%s): " % size_str, get_throughput(avg_time, size_str)

cleanup(sizes)
