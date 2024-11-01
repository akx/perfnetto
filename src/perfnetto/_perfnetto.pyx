# distutils: language=c++
# cython: language_level=3
# cython: cache=True

from libcpp cimport bool
from libcpp.vector cimport vector
from cpython.bytes cimport PyBytes_FromStringAndSize


class PerfnettoError(RuntimeError):
    pass


cdef extern from "./perfetto_wrapper.cc":
    void pw_init_perfetto(bool use_system_backend)
    void pw_instant(const char * name)
    void pw_start_region(const char * name)
    void pw_end_region()
    int pw_start_tracing(bool all_threads, int size_kb, const char * output_path)
    vector[char] pw_stop_tracing(bool read_data)


def init(*, use_system_backend: bool = False) -> None:
    pw_init_perfetto(use_system_backend=use_system_backend)


def start(*, all_threads: bool = False, size_kb: int = 65536, output_path: str | None = None) -> None:
    cdef char* output_path_cstr = NULL
    if output_path:
        output_path_bytes = output_path.encode("utf-8")
        output_path_cstr = output_path_bytes
    ret = pw_start_tracing(all_threads=all_threads, size_kb=size_kb, output_path=output_path_cstr)
    if ret == 42:
        raise PerfnettoError("A trace is already in progress")
    elif ret != 0:
        raise PerfnettoError(f"Failed to start tracing, error code: {ret}")


def stop(*, read_data: bool) -> bytes:
    byte_vec = pw_stop_tracing(read_data=read_data)
    # TODO: this does a copy :(
    return PyBytes_FromStringAndSize(byte_vec.data(), byte_vec.size())



def instant(name: str) -> None:
    cdef char * name_cstr = NULL
    name_bytes = name.encode("utf-8")
    name_cstr = name_bytes
    pw_instant(name_cstr)


def start_region(name: str) -> None:
    cdef char* name_cstr = NULL
    name_bytes = name.encode("utf-8")
    name_cstr = name_bytes
    pw_start_region(name_cstr)


def end_region() -> None:
    pw_end_region()
