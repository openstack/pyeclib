class ECNullDriver(object):
  def __init__(self, k, m):
    self.k = k
    self.m = m

  def encode(bytes):
    pass
  
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

