from enum import Enum
from enum import unique


# Erasure Code backends supported as of this PyECLib API rev
@unique
class PyECLib_EC_Types(Enum):
    # Note: the Enum start value defaults to 1 as the starting value and not 0
    # 0 is False in the boolean sense but enum members evaluate to True
    jerasure_rs_vand = 1
    jerasure_rs_cauchy = 2
    flat_xor_hd = 3
    isa_l_rs_vand = 4
    shss = 5
    liberasurecode_rs_vand = 6
    isa_l_rs_cauchy = 7
    libphazr = 8
    isa_l_rs_vand_inv = 9
    isa_l_rs_lrc = 10


# Output of Erasure (en)Coding process are data "fragments".  Fragment data
# integrity checks are provided by a checksum embedded in a header (prepend)
# for each fragment.


# The following Enum defines the schemes supported for fragment checksums.
# The checksum type is "none" unless specified.
@unique
class PyECLib_FRAGHDRCHKSUM_Types(Enum):
    # Note: the Enum start value defaults to 1 as the starting value and not 0
    # 0 is False in the boolean sense but enum members evaluate to True
    none = 1
    inline_crc32 = 2
