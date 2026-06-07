# Week 1 — CSoT Low Latency Track

This is my Week 1 submission for the **CSoT'26 Low Latency** track.

The task was to build a tick replay engine and implement the fixed mean-reversion strategy from `STRATEGY_SPEC.md`. Everyone must produce the same order stream; the leaderboard ranks correct submissions by latency.

## What is in this folder

- `src/` — CSV loader + replay engine + `main`
- `include/` — headers (`strategy.hpp`, `engine.hpp`, `histogram.hpp`)
- `strategies/` — `spec_strategy.cpp` (leaderboard strategy) and `null_strategy.cpp` (baseline)
- `bench/` — Google Benchmark for the strategy hot path
- `data/` — tick generator + sample CSV files

## Build

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

Optional (needs `libbenchmark-dev`):

```bash
cmake --build build/release --target bench_runner
```

## Run

Latency histogram (per-tick `on_tick` time):

```bash
./build/release/quant_runner data/tiny.csv
./build/release/quant_runner data/synthetic_small.csv
```

Generate data if needed:

```bash
python3 data/gen.py --rows 10000 --out data/synthetic_small.csv
python3 data/gen.py --rows 10000000 --out data/synthetic_large.csv
```

Build the strategy plugin for the judge:

```bash
cmake --build build/release --target spec_strategy
```

Upload: `build/release/libspec_strategy.so`

## Optimizations I made

These are all in `strategies/spec_strategy.cpp` unless noted.

1. **O(1) rolling window stats** — keep running `sum` and `sum_sq` instead of looping over 64 mids every tick.
2. **No `unordered_map` on the hot path** — use `std::array<SymbolState, 64>` with a dense symbol id (spec allows up to 64 symbols).
3. **Fast `SYM*` symbol parsing** — parse `SYM0`, `SYM1`, etc. directly instead of hashing strings every tick.
4. **`always_inline` hot helpers** — `rolling_kernel`, `resolve_symbol_id`, and `parse_sym_id` are forced inline to reduce call overhead (helps tail latency).
5. **Algebraic z-score checks** — compare `mid - mean` against `ENTRY_Z * stddev` / `EXIT_Z * stddev` instead of computing `z = (...)/stddev` when possible.
6. **Stable symbol storage in the loader** (`engine.cpp`) — intern symbol strings so `Tick::symbol` string_views stay valid for the whole run.
7. **Release build flags** — `-O3`, LTO/IPO, and `-fno-exceptions -fno-rtti` on the `spec_strategy` shared library.

I also tried reusing a member `vector` scratch buffer for orders, but that made throughput worse (extra move every tick), so I reverted it.

## Profiling notes

`perf` does not work properly on my WSL2 setup, so I used **Callgrind + KCachegrind** instead. See `notes.txt` for the exact commands.

On my profile, most of the cost sits inside `SpecStrategy::on_tick` (rolling stats, symbol lookup, and vector return path).

## Environment

- OS: WSL2 / Ubuntu
- Compiler: g++ (C++20)
- Build type used for submission: Release
