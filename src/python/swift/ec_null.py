class ECNullDriver(object):
  def __init__(self, k, m):
    self.k = k
    self.m = m

  def encode(self, bytes):
    pass
  
  def decode(self, fragment_payloads):
    pass
  
  def reconstruct(self, available_fragment_payloads, missing_fragment_indexes, destination_index):
    pass

  def fragments_needed(self, missing_fragment_indexes):
    pass

  def get_metadata(self, fragment):
    pass

  def verify_stripe_metadata(self, fragment_metadata_list):
    pass

