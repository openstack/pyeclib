import pyeclib

class ECPyECLibDriver(object):
  def __init__(self, k, m, type, w=0, hd=-1):
    self.ec_rs_vand = "rs_vand"
    self.ec_rs_cauchy_orig = "rs_cauchy_orig"
    self.ec_flat_xor = "flat_xor"
    self.ec_types = [self.ec_rs_vand, self.ec_rs_cauchy_orig, self.ec_flat_xor]
    self.ec_valid_xor_params = ["12_6_4", "10_5_3"]
    self.k = k
    self.m = m
    self.w = w
    self.hd = hd
    if type in self.ec_types:
      self.type = type
    else:
      raise ECDriverError("%s is not a valid EC type for PyECLib!")

    if type == self.ec_flat_xor:
      param_str = "%d_%d_%d" % (k, m, hd)
      if param_str not in self.ec_valid_xor_params:
        raise ECDriverError("%s are not valid flat-XOR code params for PyECLib!" % param_str)
      self.type = type + "_" + param_str

    self.handle = pyeclib.init(self.k, self.m, self.w, self.type)

  def encode(self, bytes):
    return pyeclib.encode(self.handle, bytes) 
  
  def decode(self, fragment_payloads):
    try:
      ret_string = pyeclib.fragments_to_string(self.handle, fragment_payloads)
    except:
      raise ECDriverError("Error in ECPyECLibDriver.decode")

    if ret_string is None:
      (data_frags, parity_frags, missing_idxs) = pyeclib.get_fragment_partition(self.handle, fragment_payloads)
      decoded_fragments = pyeclib.decode(self.handle, data_frags, parity_frags, missing_idxs, len(data_frags[0]))
      ret_string = pyeclib.fragments_to_string(self.handle, decoded_fragments)

    return ret_string
  
  def reconstruct(self, fragment_payloads, indexes_to_reconstruct):
    reconstructed_bytes = []

    # Reconstruct the data, then the parity
    # The parity cannot be reconstructed until
    # after all data is reconstructed
    indexes_to_reconstruct.sort()
    _indexes_to_reconstruct = indexes_to_reconstruct[:]

    while len(_indexes_to_reconstruct) > 0:
      index = _indexes_to_reconstruct.pop(0)
      (data_frags, parity_frags, missing_idxs) = pyeclib.get_fragment_partition(self.handle, fragment_payloads)
      reconstructed = pyeclib.reconstruct(self.handle, data_frags, parity_frags, missing_idxs, index, len(data_frags[0]))
      reconstructed_bytes.append(reconstructed)

    return reconstructed_bytes

  def fragments_needed(self, missing_fragment_indexes):
    return pyeclib.get_required_fragments(missing_fragment_indexes) 

  def get_metadata(self, fragment):
    pass

  def verify_stripe_metadata(self, fragment_metadata_list):
    pass

