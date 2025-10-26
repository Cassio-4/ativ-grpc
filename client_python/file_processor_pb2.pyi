from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from collections.abc import Mapping as _Mapping
from typing import ClassVar as _ClassVar, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class FileChunk(_message.Message):
    __slots__ = ("content", "filename", "success", "is_last_chunk")
    CONTENT_FIELD_NUMBER: _ClassVar[int]
    FILENAME_FIELD_NUMBER: _ClassVar[int]
    SUCCESS_FIELD_NUMBER: _ClassVar[int]
    IS_LAST_CHUNK_FIELD_NUMBER: _ClassVar[int]
    content: bytes
    filename: str
    success: bool
    is_last_chunk: bool
    def __init__(self, content: _Optional[bytes] = ..., filename: _Optional[str] = ..., success: bool = ..., is_last_chunk: bool = ...) -> None: ...

class ResizeImageRequest(_message.Message):
    __slots__ = ("width", "height")
    WIDTH_FIELD_NUMBER: _ClassVar[int]
    HEIGHT_FIELD_NUMBER: _ClassVar[int]
    width: int
    height: int
    def __init__(self, width: _Optional[int] = ..., height: _Optional[int] = ...) -> None: ...

class ImageStreamRequest(_message.Message):
    __slots__ = ("format", "metadata", "chunk")
    FORMAT_FIELD_NUMBER: _ClassVar[int]
    METADATA_FIELD_NUMBER: _ClassVar[int]
    CHUNK_FIELD_NUMBER: _ClassVar[int]
    format: str
    metadata: ResizeImageRequest
    chunk: FileChunk
    def __init__(self, format: _Optional[str] = ..., metadata: _Optional[_Union[ResizeImageRequest, _Mapping]] = ..., chunk: _Optional[_Union[FileChunk, _Mapping]] = ...) -> None: ...
