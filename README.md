# Perfnetto

Perfnetto wraps the [Perfetto tracing SDK](https://perfetto.dev/docs/instrumentation/tracing-sdk)
to make it possible to trace Python code.

> [!NOTE]  
> This is a very new project and certainly rough around the edges.
> Please report any issues you find – I hope it's useful in any case.

## Installation

## From PyPI

Prebuilt binary wheels are available for Linux and macOS for recent versions of Python on PyPI,
so you can use `pip` or `uv pip` to install it.

### Building from source

Source packages are not available on PyPI (so `pip install perfnetto` won't work out of the box)
because of the submodule dependency on the Perfetto SDK.

## Usage

The high-level API is the `with perfnetto.tracing() as ctx:` context manager.

Code run within this block is traced into a `perfnetto-(TIME).dump` file, unless you specify another name.
Perfnetto will log the name of the output file onto the standard Python logger as it finishes.

When tracing, you can use `with perfnetto.region(name):` to mark a region.
You can also set instant marks with `perfnetto.instant(name)`.

Once you have a dump file, view it with [ui.perfetto.dev](https://ui.perfetto.dev/).

## License

Since the majority of this code is a wrapper around the Perfetto library, it is licensed under the same license.
This project is licensed under the Apache License, Version 2.0.
See the `LICENSE` file for more details.
