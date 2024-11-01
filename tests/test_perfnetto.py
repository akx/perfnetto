import time

import perfnetto


def test_tracing(tmp_path, monkeypatch):
    monkeypatch.setenv("PERFNETTO_OUTPUT_DIR", str(tmp_path))
    with perfnetto.tracing() as ctx:
        for x in range(10):
            with perfnetto.region(f"attempt {x}"):
                perfnetto.instant("Hop!")
                import json

                json.dumps({"a": 1, "b": 2})
                time.sleep(0.1)
    with open(ctx.output_path, "rb") as fp:
        assert fp.read(1) == b"\n"
