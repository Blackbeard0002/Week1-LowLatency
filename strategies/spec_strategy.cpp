#include "strategy.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <vector>

#if defined(__GNUC__) || defined(__clang__)
#define CSOT_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define CSOT_ALWAYS_INLINE inline
#endif

namespace {

inline constexpr uint32_t WINDOW = 64;
inline constexpr uint32_t MAX_SYMBOLS = 64;
inline constexpr double ENTRY_Z = 2.0;
inline constexpr double EXIT_Z = 0.5;
inline constexpr uint32_t ORDER_QTY = 1;
inline constexpr double EPSILON_STDDEV = 1e-9;
inline constexpr uint32_t INVALID_SYMBOL = MAX_SYMBOLS;

struct SymbolState {
    double mids[WINDOW]{};
    uint32_t count = 0;
    uint32_t head = 0;
    int32_t position = 0;
    double sum = 0.0;
    double sum_sq = 0.0;
};

struct SymbolSlot {
    std::string_view symbol{};
    bool used = false;
};

CSOT_ALWAYS_INLINE uint32_t parse_sym_id(std::string_view sym) {
    if (sym.size() < 4 || sym[0] != 'S' || sym[1] != 'Y' || sym[2] != 'M') {
        return INVALID_SYMBOL;
    }

    uint32_t id = 0;
    for (std::size_t i = 3; i < sym.size(); ++i) {
        const char c = sym[i];
        if (c < '0' || c > '9') {
            return INVALID_SYMBOL;
        }
        id = id * 10U + static_cast<uint32_t>(c - '0');
        if (id >= MAX_SYMBOLS) {
            return INVALID_SYMBOL;
        }
    }
    return id;
}

CSOT_ALWAYS_INLINE std::vector<csot::Order> rolling_kernel(SymbolState& st,
                                                          double mid,
                                                          double bid_px,
                                                          double ask_px,
                                                          std::string_view symbol) {
    if (st.count < WINDOW) {
        st.mids[st.head] = mid;
        st.sum += mid;
        st.sum_sq += mid * mid;
        st.head = (st.head + 1U) & (WINDOW - 1U);
        st.count += 1U;
        if (st.count < WINDOW) {
            return {};
        }
    } else {
        const double old = st.mids[st.head];
        st.mids[st.head] = mid;
        st.sum += (mid - old);
        st.sum_sq += (mid * mid - old * old);
        st.head = (st.head + 1U) & (WINDOW - 1U);
    }

    const double mean = st.sum / static_cast<double>(WINDOW);
    double variance = st.sum_sq / static_cast<double>(WINDOW) - mean * mean;
    if (variance < 0.0) {
        variance = 0.0;
    }
    const double stddev = std::sqrt(variance);
    if (stddev < EPSILON_STDDEV) {
        return {};
    }

    const double d = mid - mean;
    const double entry_band = ENTRY_Z * stddev;
    const double exit_band = EXIT_Z * stddev;

    if (st.position == 0) {
        if (d >= entry_band) {
            return {csot::Order{csot::Order::Side::SELL, symbol, bid_px, ORDER_QTY}};
        }
        if (d <= -entry_band) {
            return {csot::Order{csot::Order::Side::BUY, symbol, ask_px, ORDER_QTY}};
        }
        return {};
    }

    if (st.position > 0 && d <= exit_band && d >= -exit_band) {
        return {csot::Order{
            csot::Order::Side::SELL,
            symbol,
            bid_px,
            static_cast<uint32_t>(st.position)}};
    }
    if (st.position < 0 && d <= exit_band && d >= -exit_band) {
        return {csot::Order{
            csot::Order::Side::BUY,
            symbol,
            ask_px,
            static_cast<uint32_t>(-st.position)}};
    }

    return {};
}

class SpecStrategy final : public csot::Strategy {
public:
    void on_init() override {
        states_.fill(SymbolState{});
        symbols_.fill(SymbolSlot{});
        num_symbols_ = 0;
    }

    std::vector<csot::Order> on_tick(const csot::Tick& t) override {
        const uint32_t id = resolve_symbol_id(t.symbol);
        return rolling_kernel(states_[id], (t.bid_px + t.ask_px) * 0.5, t.bid_px, t.ask_px,
                              symbols_[id].symbol);
    }

    void on_fill(const csot::Order& o, double fill_price, uint32_t fill_qty) override {
        (void)fill_price;
        const uint32_t id = find_symbol_id(o.symbol);
        if (id == INVALID_SYMBOL) {
            return;
        }

        if (o.side == csot::Order::Side::BUY) {
            states_[id].position += static_cast<int32_t>(fill_qty);
        } else {
            states_[id].position -= static_cast<int32_t>(fill_qty);
        }
    }

private:
    CSOT_ALWAYS_INLINE uint32_t resolve_symbol_id(std::string_view sym) {
        const uint32_t parsed = parse_sym_id(sym);
        if (parsed < MAX_SYMBOLS) {
            if (!symbols_[parsed].used) {
                symbols_[parsed].symbol = sym;
                symbols_[parsed].used = true;
                if (parsed >= num_symbols_) {
                    num_symbols_ = parsed + 1U;
                }
            }
            return parsed;
        }

        for (uint32_t i = 0; i < num_symbols_; ++i) {
            if (symbols_[i].symbol == sym) {
                return i;
            }
        }

        const uint32_t id = num_symbols_++;
        symbols_[id].symbol = sym;
        symbols_[id].used = true;
        return id;
    }

    CSOT_ALWAYS_INLINE uint32_t find_symbol_id(std::string_view sym) const {
        const uint32_t parsed = parse_sym_id(sym);
        if (parsed < MAX_SYMBOLS && symbols_[parsed].used && symbols_[parsed].symbol == sym) {
            return parsed;
        }

        for (uint32_t i = 0; i < num_symbols_; ++i) {
            if (symbols_[i].used && symbols_[i].symbol == sym) {
                return i;
            }
        }
        return INVALID_SYMBOL;
    }

    std::array<SymbolState, MAX_SYMBOLS> states_{};
    std::array<SymbolSlot, MAX_SYMBOLS> symbols_{};
    uint32_t num_symbols_ = 0;
};

}  // namespace

extern "C" csot::Strategy* create_strategy() {
    return new SpecStrategy();
}
