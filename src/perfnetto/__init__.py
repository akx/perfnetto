import dataclasses
import logging
import os
import time
from contextlib import contextmanager

from . import _perfnetto

log = logging.getLogger("perfnetto")


@dataclasses.dataclass(frozen=True)
class TracingContext:
    output_path: str


@contextmanager
def tracing(
    *,
    all_threads: bool = True,
    output_path: str | None = None,
    session_name: str | None = None,
    size_kb: int = 65536,
) -> TracingContext:
    if not output_path:
        output_path = f"perfnetto-{int(time.time() * 1000)}.dump"
        if output_dir := os.environ.get("PERFNETTO_OUTPUT_DIR"):
            os.makedirs(output_dir, exist_ok=True)
            output_path = os.path.join(output_dir, output_path)

    try:
        _perfnetto.start(
            all_threads=all_threads,
            output_path=output_path,
            session_name=session_name,
            size_kb=size_kb,
        )
        yield TracingContext(output_path=output_path)
    finally:
        _perfnetto.stop(read_data=False)
        log.info("Tracing data saved to %s", output_path)


def instant(name: str):
    return _perfnetto.instant(name)


@contextmanager
def region(name: str):
    _perfnetto.start_region(name)
    try:
        yield
    finally:
        _perfnetto.end_region()
