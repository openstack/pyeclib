# Copyright (c) 2026, NVIDIA
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.  THIS SOFTWARE IS
# PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
# NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from typing import Any

# PyECLib Exceptions


# Generic ECDriverException
class ECDriverError(Exception):
    def __init__(self, error: Any):
        try:
            self.error_str = str(error)
        except Exception:
            self.error_str = (
                "Error retrieving the error message from %s"
                % error.__class__.__name__
            )

    def __str__(self) -> str:
        return self.error_str


class ECDriverErrorWithPosition(ECDriverError):
    def __init__(self, error: str, idx: int):
        super().__init__(error)
        self.position = idx

    def __str__(self) -> str:
        return f"{self.error_str} (position {self.position})"


# More specific exceptions, mapped to liberasurecode error codes


# Specified EC backend is not supported by PyECLib/liberasurecode
class ECBackendNotSupported(ECDriverError):
    pass


# Unsupported EC method
class ECMethodNotImplemented(ECDriverError):
    pass


# liberasurecode backend init error
class ECBackendInitializationError(ECDriverError):
    pass


# Specified backend instance is invalid/unavailable
class ECBackendInstanceNotAvailable(ECDriverError):
    pass


# Specified backend instance is busy
class ECBackendInstanceInUse(ECDriverError):
    pass


# Invalid parameter passed to a method
class ECInvalidParameter(ECDriverError):
    pass


# Invalid or incompatible fragment metadata
class ECInvalidFragmentMetadata(ECDriverError):
    pass


# Fragment checksum verification failed
class ECBadFragmentChecksum(ECDriverError):
    pass


# Insufficient fragments specified for decode or reconstruct operation
class ECInsufficientFragments(ECDriverError):
    pass


# Out of memory
class ECOutOfMemory(ECDriverError):
    pass
