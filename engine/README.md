# Matching engine

A C++20 limit order book matching engine. See
[`../docs/matching-engine-plan.md`](../docs/matching-engine-plan.md) for the
design and the phased build plan.

**Status:** Phases 0–2 are done. Orders can be validated, rested on a book,
cancelled, and displayed. Matching is Phase 3.

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
```

## Try the CLI

`me_cli` is a shell around one order book for a symbol called `DEMO`
(prices in cents):

```sh
./build/debug/me_cli                          # type commands at a prompt
./build/debug/me_cli tests/cli/book_basics.in # run a script, echoing each command
```

```
> ADD BUY 100 @ 100.02
ACK #1 DEMO BUY 100 @ 100.02 GTC
> ADD SELL 200 @ 100.05
ACK #2 DEMO SELL 200 @ 100.05 GTC
> BOOK
DEMO  orders: 2  bid levels: 1  ask levels: 1
  side       price         qty  orders
   ASK      100.05         200       1
--------------------------------------  spread 0.03
   BID      100.02         100       1
> CANCEL 1
CANCELLED #1 DEMO BUY 100 @ 100.02 GTC
```

Type `HELP` for all commands. Lines starting with `#` are comments, which
is handy in scripts.

## Code map

| File | What it holds |
|---|---|
| `include/me/types.hpp` | `Price`, `Qty`, `OrderId`, `Side`, `OrderType`, `TimeInForce` |
| `include/me/price.hpp` | `parse_price` / `format_price`: decimal text ↔ fixed-point integer |
| `include/me/order.hpp` | the `Order` struct |
| `include/me/instrument.hpp` | per-symbol rules (decimals, tick, lot) and a registry |
| `include/me/validation.hpp` | `validate()` and `RejectReason` |
| `include/me/price_level.hpp` | FIFO queue of orders at one price |
| `include/me/order_book.hpp` | bids and asks for one symbol, cancel, depth |
| `include/me/ladder.hpp` | text rendering of a book |
| `apps/me_cli.cpp` | the interactive shell |
| `tests/` | GoogleTest unit tests; `tests/cli/` holds CLI golden tests |
