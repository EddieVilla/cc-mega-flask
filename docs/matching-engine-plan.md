# C++ Matching Engine — Design & Build Plan

A learning project: a limit order book matching engine in C++ that will
later feed monitoring dashboards in this Flask app. This plan is split into
phases. Each phase ends in a **checkpoint**: something you can run and see,
plus a set of unit tests that must pass before moving on.

---

## 1. Goals and non-goals

**Goals**
- Correct price-time priority (FIFO) matching for limit and market orders.
- Multiple symbols, each with its own order book.
- Deterministic: the same input sequence always produces the same output.
  This makes testing and replay easy.
- Tested at each step with GoogleTest.
- Produces a clean event stream (acks, fills, cancels, book updates) that a
  Flask dashboard can consume later.

**Non-goals (for now)**
- Real networking or exchange protocols (FIX, ITCH, OUCH).
- Persistence and crash recovery (listed as a stretch goal).
- Microsecond-level tuning before correctness is solid.

---

## 2. Tech stack

| Concern         | Choice                                               |
|-----------------|------------------------------------------------------|
| Language        | C++20                                                |
| Build           | CMake (≥ 3.21)                                       |
| Unit tests      | GoogleTest (pulled in via `FetchContent`)            |
| Benchmarks      | Google Benchmark (Phase 7)                           |
| Sanitizers      | ASan + UBSan in a `Debug` build preset               |
| Formatting/lint | `clang-format`, `clang-tidy` (optional)              |
| Output format   | JSON Lines (one event per line), via `nlohmann/json` |

---

## 3. Core design decisions

These decisions are made up front because they are hard to change later.

1. **Prices are integers.** Store prices as `int64_t` ticks (e.g. 1 tick =
   $0.01, so $101.25 → `10125`). Floating point equality and rounding cause
   subtle matching bugs.
2. **Quantities are integers.** `int64_t` shares/contracts.
3. **Time is a sequence number.** Priority comes from a monotonically
   increasing sequence number the engine assigns, not wall-clock time.
   Wall-clock timestamps come from an injectable `Clock` interface so tests
   can use a fake clock.
4. **Single-threaded matching per book.** Each book is touched by one thread
   only. Concurrency lives at the edges (input queue, output queue), not
   inside matching logic.
5. **Engine emits events; it does not print or log.** All output goes through
   an `EventListener` interface. The CLI, tests, and future Flask bridge are
   just different listeners.
6. **Trades execute at the resting order's price.** The aggressor gets price
   improvement if it crosses.

---

## 4. Proposed layout

The engine lives in its own directory so it stays independent of the Flask
app.

```
engine/
├── CMakeLists.txt
├── CMakePresets.json
├── include/me/
│   ├── types.hpp          # Price, Qty, OrderId, Side, OrderType, TimeInForce
│   ├── order.hpp          # Order struct
│   ├── events.hpp         # Ack, Reject, Trade, Cancel, BookUpdate ...
│   ├── price_level.hpp    # FIFO queue of orders at one price
│   ├── order_book.hpp     # one symbol's bids + asks
│   ├── matching_engine.hpp# many books + routing + sequencing
│   ├── event_listener.hpp # output interface
│   └── clock.hpp          # real + fake clocks
├── src/                   # .cpp files matching the headers above
├── apps/
│   ├── me_cli.cpp         # interactive REPL
│   ├── me_replay.cpp      # file in → JSONL events out
│   └── me_simulate.cpp    # random order flow generator
├── tests/
│   ├── test_types.cpp
│   ├── test_price_level.cpp
│   ├── test_order_book.cpp
│   ├── test_matching.cpp
│   ├── test_lifecycle.cpp
│   ├── test_engine.cpp
│   ├── test_invariants.cpp  # randomized / property tests
│   └── golden/              # input scenarios + expected output
└── bench/
    └── bench_matching.cpp
```

---

## 5. Data structures

```
OrderBook
├── bids: std::map<Price, PriceLevel, std::greater<>>   // best = begin()
├── asks: std::map<Price, PriceLevel, std::less<>>      // best = begin()
└── index: std::unordered_map<OrderId, Locator>         // O(1) cancel lookup

PriceLevel
├── orders: std::list<Order>   // FIFO; front = highest priority
└── total_qty: Qty             // cached for fast depth queries

Locator = { Side, Price, std::list<Order>::iterator }
```

Complexity for this first version:

| Operation         | Cost                                  |
|-------------------|---------------------------------------|
| Add (new level)   | O(log L), L = number of price levels  |
| Add (existing)    | O(log L) lookup + O(1) append         |
| Cancel            | O(1) average via `index`              |
| Best bid/ask      | O(1)                                  |
| Match one fill    | O(1) amortized                        |

This version is simple and correct. Phase 7 swaps in faster structures
(intrusive lists, object pools, flat price arrays) behind the same interface,
and the existing tests confirm nothing broke.

---

## 6. Phased plan

### Phase 0 — Project scaffolding

**Build**
- `engine/CMakeLists.txt` with a `me_core` library, a `me_tests` executable,
  and GoogleTest via `FetchContent`.
- `CMakePresets.json` with `debug` (ASan/UBSan on) and `release` presets.
- One placeholder test.
- A `Makefile` or `scripts/test.sh` shortcut:
  `cmake --preset debug && cmake --build --preset debug && ctest --preset debug`.

**Checkpoint 0:** `ctest` runs and one test passes.

**Tests:** `TEST(Sanity, Builds) { EXPECT_TRUE(true); }`

---

### Phase 1 — Domain types

**Build**
- `types.hpp`: strong typedefs or aliases for `Price`, `Qty`, `OrderId`,
  `SeqNum`, `Symbol`; enums `Side {Buy, Sell}`,
  `OrderType {Limit, Market}`, `TimeInForce {GTC, IOC, FOK}`.
- Helpers to convert between human prices and ticks:
  `to_ticks("101.25") → 10125`, `to_string(10125) → "101.25"`.
- `Order` struct: id, symbol, side, type, tif, price, qty, remaining_qty,
  seq, timestamp.
- `validate(const Order&)` returns `std::optional<RejectReason>`: zero or
  negative qty, non-positive limit price, price not on tick, unknown symbol.

**Checkpoint 1:** a small `main` constructs a few orders and prints them.

**Tests** (`test_types.cpp`)
- Price round-trips: `to_string(to_ticks(s)) == s` for several values.
- Off-tick prices are rejected (`"101.255"` with a $0.01 tick).
- Validation rejects zero/negative quantity and a limit order without a price.
- A market order with no price passes validation.

---

### Phase 2 — Order book with no matching

Build the passive book first: orders rest, cancel, and can be inspected.
Matching comes in the next phase.

**Build**
- `PriceLevel`: `push_back`, `erase(iterator)`, `front`, `total_qty`,
  `empty`.
- `OrderBook`: `add_resting(Order)`, `cancel(OrderId)`, `best_bid()`,
  `best_ask()`, `spread()`, `depth(Side, n_levels)` → vector of
  `{price, total_qty, order_count}`.
- A `print_ladder(const OrderBook&)` helper that renders:

```
        BIDS            |            ASKS
  qty    price          |   price     qty
  300    100.02         |  100.05     200
  150    100.01         |  100.06     500
```

**Checkpoint 2 — first visible output:** `me_cli` accepts
`ADD BUY 100 @ 100.02`, `CANCEL 3`, and `BOOK`, and prints the ladder.
Orders that would cross just rest for now; the ladder shows a crossed book.

**Tests** (`test_price_level.cpp`, `test_order_book.cpp`)
- An empty book has no best bid or ask.
- Adding bids at 100, 101, 99 gives best bid 101; asks work the same way
  with the lowest price winning.
- Two orders at the same price keep FIFO order.
- `total_qty` updates on add and cancel.
- Cancelling the last order at a level removes the level.
- Cancelling an unknown id returns `false` / `NotFound`.
- `depth(Buy, 3)` returns levels in correct order with correct aggregates.

---

### Phase 3 — Matching core

**Build**
- `OrderBook::submit(Order) → std::vector<Event>` (or push events into an
  `EventListener`).
- Limit order algorithm:
  1. While the incoming order has remaining qty **and** the best opposite
     level crosses (buy price ≥ best ask, or sell price ≤ best bid):
     - Take the front order at that level.
     - `fill = min(incoming.remaining, resting.remaining)`.
     - Emit `Trade{aggressor_id, resting_id, price=resting.price, qty=fill}`.
     - Reduce both; pop the resting order if filled; drop the level if empty.
  2. If qty remains, rest it on the book (for GTC limit orders).
- Market orders: same loop with no price check; never rest. Leftover qty is
  cancelled with reason `NoLiquidity`.
- `Trade` event carries: trade id, symbol, price, qty, buy order id,
  sell order id, aggressor side, seq, timestamp.

**Checkpoint 3 — the engine trades:** in `me_cli`:

```
> ADD SELL 100 @ 100.05
ACK  id=1
> ADD SELL 200 @ 100.06
ACK  id=2
> ADD BUY 250 @ 100.10
ACK  id=3
TRADE 100 @ 100.05  (buy=3 sell=1)
TRADE 150 @ 100.06  (buy=3 sell=2)
> BOOK
  ...ask 50 @ 100.06 remains...
```

**Tests** (`test_matching.cpp`) — write these as table-driven scenarios.
- A non-crossing order rests with no trades.
- An exact match fully fills both sides and leaves the book empty.
- A partial fill of the resting order leaves the remainder at the front of
  its level (priority is kept).
- A partial fill of the aggressor rests the remainder at its limit price.
- One aggressor sweeps multiple levels; trades execute at each resting price.
- Time priority: two resting orders at the same price fill in arrival order.
- Price priority beats time priority.
- Price improvement: a buy at 101 against an ask at 100 trades at 100.
- A market order on an empty book emits a cancel/reject, with no trades.
- A market order larger than available liquidity fills what it can and
  cancels the rest.
- **Conservation invariant:** the sum of trade qty equals the qty removed
  from the book plus the qty consumed from the aggressor.

---

### Phase 4 — Order lifecycle and time-in-force

**Build**
- A full event model in `events.hpp`:
  `OrderAccepted`, `OrderRejected{reason}`, `Trade`,
  `OrderCancelled{reason}`, `OrderReplaced`, `OrderFilled` (terminal).
- **Cancel** by id.
- **Modify / cancel-replace:**
  - Reducing qty at the same price **keeps** time priority.
  - Increasing qty or changing price **loses** priority (new seq number).
  - A price change can make the order cross, so it goes through matching.
- **Time in force:**
  - `GTC`: rest the remainder.
  - `IOC`: fill what's possible immediately and cancel the rest.
  - `FOK`: check available liquidity up to the limit price *before* touching
    the book. Fill entirely or reject with no side effects.
- Order state machine:
  `New → (PartiallyFilled)* → Filled | Cancelled | Rejected`.

**Checkpoint 4:** `me_cli` supports `MODIFY id qty [price]`, `IOC`/`FOK`
flags, and an `ORDERS` command that lists live orders with state and
remaining qty.

**Tests** (`test_lifecycle.cpp`)
- Cancelling a partially filled order removes only the remaining qty.
- Cancelling a filled order is rejected.
- Reducing qty keeps the order's place in the queue (check its position).
- Increasing qty moves it to the back of the level.
- A price modify that crosses produces trades.
- IOC: partial fill, then cancel event for the remainder; nothing rests.
- FOK with enough liquidity across several levels fills fully.
- FOK with insufficient liquidity is rejected and the book is **unchanged**
  (compare depth snapshots before and after).
- Each order's event sequence matches the state machine (no fill after
  cancel, no two terminal events).

---

### Phase 5 — Multi-symbol engine and replay

**Build**
- `MatchingEngine`:
  - Owns `std::unordered_map<Symbol, OrderBook>`.
  - Holds a symbol registry with tick size and lot size per symbol.
  - Assigns global order ids and sequence numbers.
  - Exposes one entry point `process(const Command&)`, where `Command` is a
    `std::variant<NewOrder, CancelOrder, ModifyOrder>`.
  - Forwards all events to registered `EventListener`s.
- A text command format, one command per line:

```
NEW  AAPL BUY  LIMIT GTC 100 189.50 client=alice
NEW  AAPL SELL MARKET IOC 50        client=bob
CANCEL 17
MODIFY 18 qty=40 price=189.55
```

- `me_replay <input.txt>` writes every event as a JSON line to stdout.
- A `JsonlListener` that serializes events.

**Checkpoint 5 — reproducible sessions:** run a scenario file and get a
JSONL event log. Running it twice gives byte-identical output (the fake
clock makes this possible).

**Tests** (`test_engine.cpp` + golden files)
- Orders are routed to the correct book; symbols do not interfere.
- An unknown symbol is rejected.
- Order ids are unique and increasing across symbols.
- **Golden-file tests:** for each `tests/golden/*.in`, run the engine and
  compare output with `*.expected.jsonl`. Adding a regression test means
  adding a file pair.

---

### Phase 6 — Market data and statistics

This is the phase that gives Flask something to show.

**Build**
- **L1 (top of book):** best bid/ask, sizes, spread, mid. Emitted when it
  changes.
- **L2 (depth):** top N levels per side as a snapshot, and incremental
  `LevelUpdate{side, price, new_total_qty}` events.
- **Trade tape:** last N trades per symbol.
- **Per-symbol stats:** last price, volume, trade count, VWAP, session
  high/low, and OHLC bars bucketed by time (e.g. 1 second or 1 minute).
- **Engine metrics:** orders/sec, trades/sec, live order count, and a
  latency histogram of `process()` time (a simple bucketed histogram is
  enough).
- `me_simulate`: a random order-flow generator (random-walk mid price,
  Poisson arrivals, mix of limit/market/cancel) driving the engine.

**Checkpoint 6 — dashboard-ready output:** `me_simulate --symbols AAPL,MSFT
--rate 1000` streams JSONL for trades, L1, L2 snapshots, and metrics, and
writes a snapshot file (e.g. `state.json`) once a second. A dashboard can be
built on this output as-is.

**Tests**
- A known trade sequence gives the correct VWAP, volume, high, and low.
- OHLC bucketing puts trades in the right bars (use the fake clock).
- L1 updates are emitted only when the top of book actually changes.
- Applying L2 incremental updates to an earlier snapshot gives the current
  snapshot.
- Simulator with a fixed RNG seed gives deterministic output.

---

### Phase 7 — Correctness hardening and performance

**Build**
- **Reference model test:** a deliberately naive `ReferenceBook` (a plain
  vector, sorted on every operation) that is obviously correct. Feed the
  same random command stream to both books and assert that events and depth
  are identical after every step. This catches most matching bugs.
- **Invariant checks** (in debug builds, after every command):
  - The book is never crossed at rest (best bid < best ask).
  - Every level's `total_qty` equals the sum of its orders.
  - `index` size equals the total resting order count, with no dangling
    iterators.
  - No resting order has zero remaining qty.
- **Benchmarks** with Google Benchmark: add, cancel, aggressive sweep,
  and mixed realistic flow. Record throughput and p50/p99/p99.9 latency.
- **Optimizations**, one at a time, measured against the benchmarks:
  1. An object pool / free list for `Order` (no allocation per order).
  2. An intrusive doubly linked list in place of `std::list`.
  3. A flat array of levels indexed by `(price - base) / tick` for dense
     books, in place of `std::map`.
  4. Pre-reserved hash maps, or a vector index keyed on dense order ids.
- **Threading:** a gateway thread parses commands and pushes them into a
  lock-free single-producer/single-consumer ring buffer. The engine thread
  consumes it, and events go out through a second SPSC queue to a publisher
  thread. Matching itself stays single-threaded.

**Checkpoint 7:** a benchmark report (before/after table for each
optimization) and a 1M-command randomized differential test that passes
under ASan/UBSan.

**Tests**
- `test_invariants.cpp`: randomized differential tests against
  `ReferenceBook` with many seeds.
- SPSC queue tests: single-threaded push/pop, wrap-around, full/empty, and a
  two-thread stress test that checks ordering and no loss.
- All earlier tests still pass after each optimization.

---

### Phase 8 — Bridge to Flask (prep only)

Flask work is deferred, but the engine's boundary should be settled now.
Options, simplest first:

| Option | How it works | Pros | Cons |
|---|---|---|---|
| **A. Files** | Engine writes JSONL events plus a periodic `state.json`; Flask reads them | Trivial and debuggable | Polling; no commands from the UI |
| **B. SQLite** | Engine writes trades, snapshots, and metrics into SQLite; Flask queries it | Historical queries and charts are easy | Write load at high rates |
| **C. Socket / ZeroMQ** | Engine publishes events; a Flask background worker subscribes | Live streaming; can accept commands | More moving parts |
| **D. pybind11** | Compile the engine as a Python module and call it in-process | Python can drive the engine directly | Couples lifetimes; the GIL and threads need care |

**Recommendation:** start with **A** (it falls out of Phase 5/6), move to
**B** when you want historical charts, and try **D** later as its own
learning exercise. Because everything goes through `EventListener`, each
option is a new listener class with no change to the engine.

---

## 7. Stretch goals

- Stop and stop-limit orders (a trigger book keyed on last trade price).
- Iceberg (reserve) orders: show a slice, replenish, and lose priority on
  each refresh.
- Self-trade prevention (cancel newest, cancel oldest, or cancel both).
- Opening/closing auctions: a call period, then uncross at the price that
  maximizes executed volume.
- Journaling and recovery: append every command to a log and rebuild state
  by replaying it (easy because the engine is deterministic).
- Pro-rata matching as an alternative to FIFO, selected per symbol.
- Circuit breakers and price bands (reject orders too far from last price).

---

## 8. Testing strategy summary

| Layer | Technique | Where |
|---|---|---|
| Types and validation | Plain unit tests | Phase 1 |
| Book mechanics | Unit tests on small, hand-built books | Phase 2 |
| Matching rules | Table-driven scenario tests | Phases 3–4 |
| End-to-end behaviour | Golden JSONL files | Phase 5 onward |
| Statistics | Known-answer tests with a fake clock | Phase 6 |
| Deep correctness | Randomized differential testing plus invariants | Phase 7 |
| Memory and UB bugs | ASan/UBSan on every debug test run | Always |
| Performance regressions | Google Benchmark baselines | Phase 7 |

Rule of thumb: **no phase is done until its tests pass under the sanitizer
build**, and every bug found later gets a golden-file or unit test before
it's fixed.

---

## 9. Checkpoint summary

| # | You can see... | Key tests |
|---|---|---|
| 0 | `ctest` green | Build sanity |
| 1 | Orders printed from `main` | Price ticks, validation |
| 2 | Ladder view of a (non-matching) book in the CLI | FIFO, levels, cancel |
| 3 | Trades printed live in the CLI | Price-time priority, partial fills, sweeps |
| 4 | Modify/IOC/FOK in the CLI, order status list | Lifecycle state machine, FOK atomicity |
| 5 | Scenario file → deterministic JSONL log | Multi-symbol routing, golden files |
| 6 | Simulator streaming L1/L2/trades/metrics | VWAP, OHLC, L2 reconstruction |
| 7 | Benchmark report, 1M-step differential test | Reference model, invariants, SPSC |
| 8 | (Flask later) reads engine output | Listener integration tests |
