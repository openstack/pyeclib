from typing import (
    NewType,
    Sequence,
    TypedDict,
)
from typing_extensions import NotRequired

def get_liberasurecode_version() -> int: ...
def check_backend_available(
    backend_id: int,
) -> bool: ...

PyECLibHandle = NewType("PyECLibHandle", object)

def init(
    k: int,
    m: int,
    backend_id: int,
    hd: int,
    inline_chksum: int,
    algsig_checksum: int,
    validate: bool,
    local_parity: int,
) -> PyECLibHandle: ...
def destroy(instance: PyECLibHandle) -> None: ...
def encode(instance: PyECLibHandle, data: bytes) -> list[bytes]: ...
def decode(
    instance: PyECLibHandle,
    fragments: Sequence[bytes],
    fragment_length: int,
    ranges: list[tuple[int, int]] | None = None,
    force_metadata_checks: bool = False,
) -> bytes: ...
def reconstruct(
    instance: PyECLibHandle,
    fragments: Sequence[bytes],
    fragment_length: int,
    index_to_rebuild: int,
) -> bytes: ...

class CheckMetadataResultDict(TypedDict):
    status: int
    reason: NotRequired[str]
    bad_fragments: NotRequired[list[int]]

def check_metadata(
    instance: PyECLibHandle,
    fragments: Sequence[bytes],
) -> CheckMetadataResultDict: ...

class MetadataDict(TypedDict):
    index: int
    size: int
    orig_data_size: int
    checksum_type: str
    checksum: str  # hex-encoded
    checksum_mismatch: bool
    backend_id: str
    backend_version: int

def get_metadata(
    instance: PyECLibHandle,
    fragment: bytes,
    formatted: int,
) -> bytes | MetadataDict: ...
def get_required_fragments(
    instance: PyECLibHandle,
    reconstruct_list: list[int],
    exclude_list: list[int],
) -> list[int]: ...

class SegmentInfoDict(TypedDict):
    segment_size: int
    last_segment_size: int
    fragment_size: int
    last_fragment_size: int
    num_segments: int

def get_segment_info(
    instance: PyECLibHandle,
    data_length: int,
    segment_size: int,
) -> SegmentInfoDict: ...
