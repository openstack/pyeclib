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

def time_decode(num_data, num_parity, w, type, file_size, iterations, hd):
  filename = "test_file.%s" % file_size
  fp = open("test_files/%s" % filename, "r")
  timer = Timer()
  sum = 0
  handle = pyeclib.init(num_data, num_parity, w, type)

  whole_file_str = fp.read()

  fragments=pyeclib.encode(handle, whole_file_str)

  orig_fragments = fragments[:]

  for i in range(iterations):
    missing_idxs = []
    num_missing = hd-1
    for j in range(num_missing):
      idx = random.randint(0, (num_data+num_parity)-1)
      while idx in missing_idxs:
        idx = random.randint(0, (num_data+num_parity)-1)
      missing_idxs.append(idx)
      fragments[idx] = '\0' * len(fragments[0])

    timer.start()
    decoded_fragments = pyeclib.decode(handle, fragments[:num_data], fragments[num_data:], missing_idxs, len(fragments[0]))
    sum += timer.stop_and_return()
    
    fragments = decoded_fragments

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

    decoded_fragments = None


  return sum / iterations

def test_reconstruct(num_data, num_parity, w, type, file_size, iterations):
  filename = "test_file.%s" % file_size
  fp = open("test_files/%s" % filename, "r")
  timer = Timer()
  sum = 0
  handle = pyeclib.init(num_data, num_parity, w, type)
  
  whole_file_str = fp.read()

  orig_fragments=pyeclib.encode(handle, whole_file_str)

  fragments = orig_fragments[:]

  for i in range(iterations):
    num_missing = 1
    missing_idxs = []
    for j in range(num_missing):
      idx = random.randint(0, (num_data+num_parity)-1)
      while idx in missing_idxs:
        idx = random.randint(0, (num_data+num_parity)-1)
      missing_idxs.append(idx)
      fragments[idx] = '\0' * len(fragments[0])

    timer.start()
    reconstructed_fragment = pyeclib.reconstruct(handle, fragments[:num_data], fragments[num_data:], missing_idxs, missing_idxs[0], len(fragments[0]))
 
    sum += timer.stop_and_return()
    
    decoded_fragments = pyeclib.decode(handle, fragments[:num_data], fragments[num_data:], missing_idxs, len(fragments[0]))
    fragments = decoded_fragments
  
    if orig_fragments[missing_idxs[0]] != reconstructed_fragment:
      fd_orig = open("orig_fragments", "w")
      fd_decoded = open("decoded_fragments", "w")

      fd_orig.write(orig_fragments[missing_idxs[0]])
      fd_decoded.write(reconstructed_fragment)
      fd_orig.close()
      fd_decoded.close()
      print "Fragment %d was not reconstructed!!!" % missing_idxs[0]
      sys.exit(2)


  return sum / iterations

def test_get_fragment_partition(num_data, num_parity, w, type, file_size, iterations):
  filename = "test_file.%s" % file_size
  fp = open("test_files/%s" % filename, "r")
  handle = pyeclib.init(num_data, num_parity, w, type)

  whole_file_str = fp.read()
  
  fragments=pyeclib.encode(handle, whole_file_str)

  for i in range(iterations):
    missing_fragments = random.sample(fragments, 3)
    avail_fragments = fragments[:]
    missing_fragment_idxs = []
    for missing_frag in missing_fragments:
      missing_fragment_idxs.append(fragments.index(missing_frag))
      avail_fragments.remove(missing_frag)

    (data_frags, parity_frags, missing_idxs) = pyeclib.get_fragment_partition(handle, avail_fragments)

    missing_fragment_idxs.sort() 
    missing_idxs.sort()

    if missing_fragment_idxs != missing_idxs:
      print "Missing idx mismatch in test_get_fragment_partition: %s != %s\n" % (missing_fragment_idxs, missing_idxs)  
      sys.exit()


    decoded_fragments = pyeclib.decode(handle, data_frags, parity_frags, missing_idxs, len(data_frags[0]))


def test_fragments_to_string(num_data, num_parity, w, type, file_size):
  filename = "test_file.%s" % file_size
  fp = open("test_files/%s" % filename, "r")
  handle = pyeclib.init(num_data, num_parity, w, type)

  whole_file_str = fp.read()
  
  fragments=pyeclib.encode(handle, whole_file_str)

  concat_str = pyeclib.fragments_to_string(handle, fragments[:num_data])

  if concat_str != whole_file_str:
    print "String does not equal the original string (len(orig) = %d, len(new) = %d\n" % (len(whole_file_str), len(concat_str))


def test_get_required_fragments(num_data, num_parity, w, type):
  handle = pyeclib.init(num_data, num_parity, w, type)
  
  #
  # MDS codes need any k fragments
  #
  if type in ["rs_vand", "rs_cauchy_orig"]:
    expected_fragments = [i for i in range(num_data + num_parity)]
    missing_fragments = [] 

    #
    # Remove between 1 and num_parity
    #
    for i in range(random.randint(0, num_parity-1)):
      missing_fragment = random.sample(expected_fragments, 1)[0]
      missing_fragments.append(missing_fragment)
      expected_fragments.remove(missing_fragment)

    expected_fragments = expected_fragments[:num_data]
    required_fragments = pyeclib.get_required_fragments(handle, missing_fragments)

    if expected_fragments != required_fragments:
      print "Unexpected required fragments list (exp != req): %s != %s" % (expected_fragments, required_fragments)
      sys.exit(2)  

def get_throughput(avg_time, size_str):
  size_desc = size_str.split("-")  

  size = float(size_desc[0])

  if (size_desc[1] == 'M'):
    return size / avg_time
  elif (size_desc[1] == 'K'):
    return (size / 1000.0) / avg_time
  
num_datas = [12, 12, 12] 
num_parities = [2, 3, 4]
iterations=100

rs_types = [("rs_vand", 16), ("rs_cauchy_orig", 4)]
xor_types = [("flat_xor", 12, 6, 4), ("flat_xor", 10, 5, 3)]

sizes = ["101-K", "202-K", "303-K"]

setup(sizes)

for (type, k, m, hd) in xor_types:
  print "Running tests for %s k=%d, m=%d\n" % (type, k, m)

  type_str = "%s_%d_%d_%d" % (type, k, m, hd)

  for size_str in sizes:
    avg_time = time_encode(k, m, 0, type_str, size_str, iterations)
    print "Encode (%s): " % size_str, get_throughput(avg_time, size_str)
  
  for size_str in sizes:
    avg_time = time_decode(k, m, 0, type_str, size_str, iterations, 3)
    print "Decode (%s): " % size_str, get_throughput(avg_time, size_str)
  
  for size_str in sizes:
    avg_time = test_reconstruct(k, m, 0, type_str, size_str, iterations)
    print "Reconstruct (%s): " % size_str, get_throughput(avg_time, size_str)

for (type, w) in rs_types:
  print "Running tests for %s w=%d\n" % (type, w)
  
  for i in range(len(num_datas)):
    for size_str in sizes:
      test_get_fragment_partition(num_datas[i], num_parities[i], w, type, size_str, iterations)

  for i in range(len(num_datas)):
    for size_str in sizes:
      test_fragments_to_string(num_datas[i], num_parities[i], w, type, size_str)

  for i in range(len(num_datas)):
    test_get_required_fragments(num_datas[i], num_parities[i], w, type)

  for i in range(len(num_datas)):
    for size_str in sizes:
      avg_time = time_encode(num_datas[i], num_parities[i], w, type, size_str, iterations)
      print "Encode (%s): " % size_str, get_throughput(avg_time, size_str)

  for i in range(len(num_datas)):
    for size_str in sizes:
      avg_time = time_decode(num_datas[i], num_parities[i], w, type, size_str, iterations, num_parities[i]+1)
      print "Decode (%s): " % size_str, get_throughput(avg_time, size_str)
  
  for i in range(len(num_datas)):
    for size_str in sizes:
      avg_time = test_reconstruct(num_datas[i], num_parities[i], w, type, size_str, iterations)
      print "Reconstruct (%s): " % size_str, get_throughput(avg_time, size_str)


cleanup(sizes)
