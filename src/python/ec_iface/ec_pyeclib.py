
# Copyright (c) 2013, Kevin Greenan (kmgreen2@gmail.com)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.  THIS SOFTWARE IS PROVIDED BY
# THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import pyeclib

class ECPyECLibDriver(object):
  def __init__(self, k, m, type):
    self.ec_rs_vand = "rs_vand"
    self.ec_rs_cauchy_orig = "rs_cauchy_orig"
    self.ec_flat_xor_3 = "flat_xor_3"
    self.ec_flat_xor_4 = "flat_xor_4"
    self.ec_types = [self.ec_rs_vand, self.ec_rs_cauchy_orig, self.ec_flat_xor_3, self.ec_flat_xor_4]
    self.ec_valid_xor_params = ["12_6_4", "10_5_3"]
    self.ec_rs_vand_best_w = 16
    self.ec_default_w = 32
    self.ec_rs_cauchy_best_w = 4
    self.k = k
    self.m = m
    if type in self.ec_types:
      self.type = type
    else:
      raise ECDriverError("%s is not a valid EC type for PyECLib!")

    if self.type == self.ec_rs_vand:
      self.w = self.ec_rs_vand_best_w
      self.hd = self.m + 1
    elif self.type == self.ec_rs_cauchy_orig:
      self.w = self.ec_rs_cauchy_best_w
      self.hd = self.m + 1
    elif self.type == self.ec_flat_xor_3:
      self.w = self.ec_default_w
      self.hd = 3
    elif self.type == self.ec_flat_xor_4:
      self.w = self.ec_default_w
      self.hd = 4
    else:
      self.w = self.ec_default_w

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