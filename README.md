# Week 1 — CSoT Low Latency Track

Tick replay engine + mean-reversion strategy for the **CSoT'26 Low Latency** track. Correctness is fixed by the spec; the leaderboard ranks by per-tick latency.

## Headline latency numbers

Measured locally with `./build/release/quant_runner` (Release build, `std::chrono::steady_clock` per tick). Histogram buckets report upper bounds (`≤`), so real values sit inside the bucket below each line.

**Dataset: `data/synthetic_small.csv` (10,000 ticks)**

| Percentile | Latency |
|------------|---------|
| p50        | ≤ 128 ns |
| p90        | ≤ 128 ns |
| p99        | ≤ 128 ns |
| p999       | ≤ 1024 ns |

**Dataset: `data/synthetic_large.csv` (10,000,000 ticks)**

| Percentile | Latency |
|------------|---------|
| p50        | ≤ 64 ns |
| p90        | ≤ 128 ns |
| p99        | ≤ 256 ns |
| p999       | ≤ 1024 ns |

Run date: 2026-06-07. Re-run on your machine before comparing to the leaderboard — local numbers drift with CPU frequency, background load, and virtualization.

```bash
./build/release/quant_runner data/synthetic_small.csv
./build/release/quant_runner data/synthetic_large.csv
```

## Hardware notes

Numbers above were captured on:

| Item | Value |
|------|-------|
| OS | Ubuntu 24.04 on **WSL2** (Linux 6.6.87, microsoft-standard kernel) |
| CPU | Intel Core **i7-13620H** (13th Gen, 8 cores / 16 threads) |
| L1d cache | 48 KiB × 8 |
| L2 cache | 1280 KiB × 8 |
| L3 cache | 24 MiB |
| RAM | 8 GiB (WSL2 VM) |
| Compiler | **g++ 13.3.0** (C++20), **CMake 3.28.3** |

Caveats worth knowing:

- **WSL2 adds virtualization noise.** The curriculum recommends bare-metal Linux for stable benchmarks. Treat these numbers as a local baseline, not a guarantee on the judge hardware.
- **`perf` hardware counters do not work reliably on WSL2.** For cache misses and call graphs, use Callgrind + KCachegrind instead (see `notes.txt`).
- **Pin CPU governor / close heavy apps** before benchmarking if you want repeatable p99 numbers.
- The judge loads `libspec_strategy.so` via `dlopen`; local `quant_runner` links the strategy statically — latency shape should match, but absolute ns will differ on judge machines.

## Build

### Prerequisites

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake python3
```

Optional (Google Benchmark + profiling GUI):

```bash
sudo apt-get install -y libbenchmark-dev kcachegrind valgrind
```

### Configure and build (Release)

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

Individual targets:

```bash
cmake --build build/release --target quant_runner    # latency histogram
cmake --build build/release --target spec_strategy   # judge .so
cmake --build build/release --target bench_runner      # needs libbenchmark-dev
```

Profile build (debug symbols for KCachegrind):

```bash
cmake -S . -B build/profile -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/profile
```

### Run

```bash
# quick sanity check
./build/release/quant_runner data/tiny.csv

# headline latency runs
./build/release/quant_runner data/synthetic_small.csv
./build/release/quant_runner data/synthetic_large.csv

# generate data if missing
python3 data/gen.py --rows 10000 --out data/synthetic_small.csv
python3 data/gen.py --rows 10000000 --out data/synthetic_large.csv
```

### Submit to the judge

```bash
cmake --build build/release --target spec_strategy
ls -lh build/release/libspec_strategy.so
```

Upload `build/release/libspec_strategy.so`.

## Project layout

| Path | Purpose |
|------|---------|
| `src/` | CSV loader, replay engine, `main` |
| `include/` | `strategy.hpp`, `engine.hpp`, `histogram.hpp` |
| `strategies/` | `spec_strategy.cpp` (leaderboard), `null_strategy.cpp` (baseline) |
| `bench/` | Google Benchmark hot-path harness |
| `data/` | tick generator + CSV fixtures |

## Optimizations

All in `strategies/spec_strategy.cpp` unless noted.

1. **O(1) rolling window** — running `sum` / `sum_sq` instead of rescanning 64 mids each tick.
2. **No `unordered_map` on the hot path** — `std::array<SymbolState, 64>` with dense symbol ids.
3. **Fast `SYM*` parsing** — direct digit parse for `SYM0`…`SYM3`.
4. **`always_inline` helpers** — `rolling_kernel`, `resolve_symbol_id`, `parse_sym_id`.
5. **Algebraic z-score checks** — compare against `ENTRY_Z * stddev` without dividing every tick.
6. **Stable symbol storage** (`engine.cpp`) — intern strings so `Tick::symbol` views stay valid.
7. **Release flags on `.so`** — `-O3`, LTO, `-fno-exceptions -fno-rtti`.

Tried a member `vector` scratch buffer for orders; throughput got worse (extra move every tick), so it was reverted.

## Profiling

See `notes.txt` for Callgrind/KCachegrind commands, Google Benchmark usage, and troubleshooting. Most self time in my profile sits in `SpecStrategy::on_tick` (rolling stats, symbol lookup, vector return path).
