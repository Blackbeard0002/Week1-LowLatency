# Week 2 — CSoT Low Latency Track

Two-level cache simulator for the **CSoT'26 Low Latency** track. Submit `cache_sim.cpp` to the leaderboard.

## Headline numbers

Measured locally with `./build/cache_sim_runner` on `data/large.trace` (5M accesses, seed 42).

| Build | run() time | Throughput |
|-------|------------|------------|
| Release (`-march=native`) | ~284 ms | ~17.6 M acc/s |
| Judge-like (`-march=x86-64-v2`) | ~244 ms | ~20.5 M acc/s |

Correctness: `diff` against `data/tiny.stats.json` passes on `data/tiny.trace`.

```bash
diff <(./build/cache_sim_runner data/tiny.trace 2>/dev/null) data/tiny.stats.json
```

## Hardware

- Ubuntu 24.04 on WSL2
- Intel Core i7-13620H, g++ 13.3.0
- `perf` does not work on this WSL2 setup; use Callgrind + KCachegrind if profiling (see `TROUBLESHOOTING.md`)

## Build

```bash
cmake -B build -DCSOT_CACHE_SIM_SRC=cache_sim.cpp
cmake --build build -j
```

Judge-like build (portable flags):

```bash
cmake -B build-judge -DCSOT_JUDGE_BUILD=ON -DCSOT_CACHE_SIM_SRC=cache_sim.cpp
cmake --build build-judge -j
```

## Run

```bash
./build/cache_sim_runner data/tiny.trace
./build/cache_sim_runner data/large.trace
```

Generate `large.trace` if needed:

```bash
python3 data/gen_trace.py --accesses 5000000 --seed 42 --out data/large.trace
```

## Submit

Upload `cache_sim.cpp` to [csot-low-latency.devclub.in](https://csot-low-latency.devclub.in/).
