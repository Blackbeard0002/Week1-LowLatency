#include "strategy.hpp"
#include <vector>

namespace{
    class NullStrategy final : public csot::Strategy{
    public:
        std::vector<csot::Order> on_tick(const csot::Tick& t) override {
            (void)t;
            return {};
        }
    };
}

extern "C" csot::Strategy* create_strategy(){
    return new NullStrategy();
}