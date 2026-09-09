# MESI Multi-Core Cache Coherence Simulator

A from-scratch C++17 simulator of a multi-core system with private L1 caches
kept coherent via the **MESI** protocol (Modified / Exclusive / Shared /
Invalid), driven by a trace of interleaved memory operations across cores.

Grew out of a single-core cache-simulation assignment; rebuilt as a
multi-core coherence project to explore how real CPUs keep per-core caches
consistent over a shared bus.

## Features

- **4 coherence states per line, per core** (MESI), with correct handling of:
  - Silent local hits (read hit on any state, write hit on M)
  - `E → M` on a write hit to an exclusive line (no bus transaction needed)
  - `S → M` upgrade on a write hit to a shared line (invalidates other sharers)
  - Read miss that finds a `Modified` remote copy → owner **flushes** to RAM
    and downgrades to `S`; requester loads and also becomes `S`
  - Read miss with no other valid copies → requester becomes `E`
  - Write miss (read-for-ownership): any remote `M` copy is flushed and
    invalidated, remote `E`/`S` copies are invalidated, requester becomes `M`
- **Configurable set-associative caches per core** (default 4-way), each with
  its own pseudo-LRU (tree-based, 3-bit encoding for the classic 4-way case;
  true-LRU fallback for any other associativity)
- **Trace-driven, globally ordered input** — an interleaved trace file
  (`CYCLE CORE CMD ADDR VALUE`) gives a single bus order across all cores,
  which the simulator treats as atomic bus transactions
- **Config file + CLI overrides** (`sim.cfg`, or any `--config PATH`, with
  individual flags like `--cores`, `--cache-size`, `--assoc` taking priority)
- **Per-core and aggregate statistics**: hit/miss counts, hit rate,
  invalidations sent/received, write-backs, cache-to-cache transfers, and an
  estimated **AMAT** (Average Memory Access Time) from a configurable
  hit-time/miss-penalty cost model
- **Verbose mode** (`--verbose`) prints a line-by-line coherence trace —
  useful for teaching/demoing exactly how MESI transitions happen
- CMake build + a regression test script + GitHub Actions CI

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/mesi_sim --config sim.cfg
```

Or compile directly:

```bash
g++ -O2 -std=c++17 -Iinclude -o mesi_sim src/*.cpp
./mesi_sim --config sim.cfg
```

## Usage

```
Usage: mesi_sim [--config PATH] [options]

Options (override config file / defaults):
  --config PATH            key=value config file (default: sim.cfg if present)
  --ram-size N             RAM size in bytes
  --cache-size N           per-core cache size in bytes
  --line-size N            cache line size in bytes
  --assoc N                set associativity (ways per set)
  --cores N                number of cores
  --trace PATH             interleaved trace file
  --output PATH            H/M output file
  --stats PATH             statistics output file
  --hit-time N             hit time in cycles (for AMAT estimate)
  --miss-penalty N         miss penalty in cycles (for AMAT estimate)
  --verbose                print per-operation coherence trace to stdout
  --help                   show this message
```

Config file values (`sim.cfg`) are loaded first; any matching CLI flag
overrides them, so you can keep a checked-in default config and tweak one
parameter at a time from the command line for experiments.

## Trace file format

```
# CYCLE CORE CMD ADDR VALUE
0  0 R 00002000 00
1  1 W 00002000 11
```

- `CYCLE` is a logical/annotation timestamp (shown in `--verbose` output);
  **line order in the file is the actual global bus order** the simulator
  uses — it is not re-sorted by `CYCLE`.
- `CMD` is `R` or `W`. `VALUE` is an 8-bit hex byte (ignored for `R`).

`traces/sample_trace.txt` is a hand-built 4-core, 12-operation trace designed
to walk through every MESI transition at least once — it doubles as the
project's regression test (see `tests/run_tests.sh`).

## Example: watching coherence happen

```bash
./mesi_sim --config sim.cfg --verbose
```

```
[cycle     0] core0 R 0x00002000 -> MISS new_state=E  (read miss, sole copy (Exclusive))
[cycle     3] core1 R 0x00002000 -> MISS new_state=S  (read miss, shared with other core(s))
[cycle     4] core1 W 0x00002000 -> HIT  new_state=M  (write hit, S->M upgrade (invalidated other sharers))
[cycle     5] core0 R 0x00002000 -> MISS new_state=S  (read miss, shared with other core(s))
[cycle     7] core2 W 0x00002000 -> MISS new_state=M  (write miss (RFO), invalidated other copies)
[cycle     8] core1 R 0x00002000 -> MISS new_state=S  (read miss, shared with other core(s))
```

Note how ownership of the same cache line moves between cores, each
transition flushing dirty data and invalidating stale copies exactly as MESI
requires — with no two cores ever holding conflicting `M`/`E` copies at once.

## Output

- **`trace.output`** — one `H`/`M` character per trace operation, in order
- **`stats.txt`** — per-core hit/miss/invalidation/write-back counts, plus
  aggregate hit rate and estimated AMAT

## Testing

```bash
./tests/run_tests.sh
```

Builds the simulator, runs it against the bundled sample trace, and checks
the resulting hit/miss sequence against the hand-verified expected result.
CI (`.github/workflows/ci.yml`) runs the same check via CMake on every push.

## Design notes / simplifications

- The simulator models **logical bus ordering**, not cycle-accurate timing:
  since the trace already gives a total order across cores, each request is
  processed as a single atomic transaction — equivalent to a bus with no
  contention or queuing delay. A natural next step would be to add per-core
  issue timestamps and simulate bus arbitration/contention explicitly.
- Cache **data** is stored as flat `unsigned char` arrays per core (as in
  real hardware); only per-line bookkeeping (state, tag, LRU info) uses
  higher-level containers, to keep the "memory" honestly byte-addressed while
  keeping the rest of the code maintainable.
- Only **4-way** associativity uses the classic 3-bit tree pseudo-LRU; other
  associativities fall back to true LRU via timestamps, so `--assoc` can be
  freely changed for experiments without touching the algorithm.

## Possible extensions

- Bus contention / arbitration model with per-request latency
- A second cache level (L2) with an inclusion policy
- Miss classification (cold / capacity / conflict / coherence)
- CSV sweep across configurations + plotting script for miss-rate curves
- Reading real memory traces (e.g. from `valgrind --tool=lackey`)
