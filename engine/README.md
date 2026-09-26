# Matching engine

A C++20 limit order book matching engine. See
[`../docs/matching-engine-plan.md`](../docs/matching-engine-plan.md) for the
design and the phased build plan.

## Requirements

- CMake 3.21 or newer
- A C++20 compiler (GCC 11+, Clang 14+, or MSVC 19.30+)
- Git (CMake fetches GoogleTest on the first configure)

## Build and test

```sh
cd engine
scripts/test.sh            # debug build with ASan/UBSan, then run tests
scripts/test.sh release    # optimized build, then run tests
```

Or run the steps yourself:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/me_cli
```
