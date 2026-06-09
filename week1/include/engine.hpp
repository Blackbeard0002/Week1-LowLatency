#pragma once

#include "strategy.hpp"
#include <string>
#include <vector>

namespace csot{
    std::vector<Tick> load_ticks(const std::string& path);

    void run_engine(Strategy& strategy, const std::vector<Tick>& ticks);
}