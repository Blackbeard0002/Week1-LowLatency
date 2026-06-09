#include <memory>
#include <vector>

#include <benchmark/benchmark.h>

#include "engine.hpp"
#include "strategy.hpp"

namespace {
const std::vector<csot::Tick>& ticks_small() {
    static const std::vector<csot::Tick> ticks =
        csot::load_ticks("data/synthetic_small.csv");
    return ticks;
}
}  // namespace

static void BM_SpecStrategy_OnTick(benchmark::State& state) {
    const auto& ticks = ticks_small();

    for (auto _ : state) {
        std::unique_ptr<csot::Strategy> strategy(create_strategy());
        strategy->on_init();

        uint64_t order_count = 0;
        for (const auto& tick : ticks) {
            std::vector<csot::Order> orders = strategy->on_tick(tick);
            benchmark::DoNotOptimize(orders);
            order_count += orders.size();

            for (const auto& o : orders) {
                strategy->on_fill(o, o.price, o.qty);
            }
        }

        benchmark::DoNotOptimize(order_count);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(ticks.size()));
}

BENCHMARK(BM_SpecStrategy_OnTick);

BENCHMARK_MAIN();