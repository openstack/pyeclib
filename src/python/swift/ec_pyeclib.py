import pyeclib

class ECPyECLibDriver(object):
  ECPyECLibDriver.ec_rs_vand = "rs_vand"
  ECPyECLibDriver.ec_rs_cauchy_orig = "rs_cauchy_orig"
  ECPyECLibDriver.ec_flat_xor = "flat_xor"
  ECPyECLibDriver.ec_types = [ECPyECLibDriver.ec_rs_vand, ECPyECLibDriver.ec_rs_cauchy_orig, ECPyECLibDriver.ec_flat_xor]
  ECPyECLibDriver.ec_valid_xor_params = ["12_6_4", "10_5_3"]
  def __init__(self, k, m, type, w=-1, hd=-1):
    self.k = k
    self.m = m
    self.w = w
    self.hd = hd
    if type in ECPyECLibDriver.ec_types:
      self.type = type
    else:
      raise ECDriverError("%s is not a valid EC type for PyECLib!")

    if type == ECPyECLibDriver.ec_flat_xor:
      param_str = "%d_%d_%d" % (k, m, hd)
      if param_str not in ECPyECLibDriver.ec_valid_xor_params:
        raise ECDriverError("%s are not valid flat-XOR code params for PyECLib!" % param_str)
      self.type = type + "_" + param_str

    self.handle = pyeclib.init(self.k, self.m, self.w, self.type)

  def encode(bytes):
    return pyeclib.encode(self.handle, bytes) 
  
  def decode(fragment_payloads):
    pass
  
  def reconstruct(available_fragment_payloads, missing_fragment_indexes):
    pass

  def fragments_needed(missing_fragment_indexes):
    pass

  def get_metadata(fragment):
    pass

  def verify_stripe_metadata(fragment_metadata_list):
    pass

