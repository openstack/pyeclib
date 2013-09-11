from ec import ECDriver

null_driver = ECDriver("ec_null.ECNullDriver", k=8, m=2)

stripe_driver = ECDriver("ec_stripe.ECStripingDriver", k=8, m=0)


