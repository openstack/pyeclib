from ec import ECDriver
import unittest

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

if __name__ == '__main__':
    unittest.main()
    
