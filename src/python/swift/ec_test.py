from ec import ECDriver
import unittest
import pyeclib
import random
import string
import sys
import os

class TestNullDriver(unittest.TestCase):

  def setUp(self):
    self.null_driver = ECDriver("ec_null.ECNullDriver", k=8, m=2)

  def tearDown(self):
    pass

  def test_null_driver(self):
    self.null_driver.encode('')
    self.null_driver.decode([])
    

class TestStripeDriver(unittest.TestCase):
  def setUp(self):
    self.stripe_driver = ECDriver("ec_stripe.ECStripingDriver", k=8, m=0)

  def tearDown(self):
    pass
    
class TestPyECLibDriver(unittest.TestCase):

  def __init__(self, *args):
    self.file_sizes = ["100-K"]
    self.num_iterations = 1

    unittest.TestCase.__init__(self, *args)
  
  def setUp(self):

    for size_str in self.file_sizes:
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

  def tearDown(self):
    for size_str in self.file_sizes:
      filename = "test_files/test_file.%s" % size_str
      os.unlink(filename)

  def test_rs(self):

    pyeclib_driver = ECDriver("ec_pyeclib.ECPyECLibDriver", k=12, m=2, type="rs_vand", w=16)

    for file_size in self.file_sizes:
      filename = "test_file.%s" % file_size
      fp = open("test_files/%s" % filename, "r")

      whole_file_str = fp.read()
  
      fp=open("orig", "w")
      fp.write(whole_file_str)
      fp.close()

      orig_fragments=pyeclib_driver.encode(whole_file_str)

      fragments = orig_fragments[:]
      num_missing = 2
      idxs_to_remove = []
      for j in range(num_missing):
        idx = random.randint(0, 13)
        if idx not in idxs_to_remove:
          idxs_to_remove.append(idx)
        
      # Reverse sort the list, so we can always
      # remove from the original index 
      idxs_to_remove.sort(lambda x,y: y-x)
      for idx in idxs_to_remove:
        fragments.pop(idx)

      decoded_string = pyeclib_driver.decode(fragments)
      fp=open("decoded", "w")
      fp.write(decoded_string)
      fp.close()

      self.assertTrue(whole_file_str == decoded_string)

if __name__ == '__main__':
    unittest.main()
