#include "cache_sim.hpp"

#include <array>
#include <cstdint>

namespace {

constexpr int L1_SETS = 64;
constexpr int L1_WAYS = 8;
constexpr int L2_SETS = 512;
constexpr int L2_WAYS = 8;

constexpr int L1_SET_MASK = L1_SETS - 1;
constexpr int L2_SET_MASK = L2_SETS - 1;

template <int NumSets, int NumWays>
struct CacheLevel {
    static constexpr int SETS = NumSets;
    static constexpr int WAYS = NumWays;

    std::array<std::uint64_t, SETS * WAYS> tags{};
    std::array<std::uint8_t, SETS * WAYS> valid{};
    std::array<std::uint8_t, SETS * WAYS> dirty{};
    std::array<std::uint32_t, SETS * WAYS> lru_stamp{};
    std::array<std::uint32_t, SETS> stamp{};

    [[nodiscard]] int find(int set, std::uint64_t tag) const {
        const int base = set * WAYS;
        for (int w = 0; w < WAYS; ++w) {
            const int idx = base + w;
            if (valid[idx] && tags[idx] == tag) {
                return w;
            }
        }
        return -1;
    }

    void touch(int set, int way) {
        const int idx = set * WAYS + way;
        lru_stamp[idx] = ++stamp[set];
    }

    [[nodiscard]] int victim_way(int set) const {
        const int base = set * WAYS;
        for (int w = 0; w < WAYS; ++w) {
            if (!valid[base + w]) {
                return w;
            }
        }

        int victim = 0;
        std::uint32_t oldest = lru_stamp[base];
        for (int w = 1; w < WAYS; ++w) {
            const std::uint32_t age = lru_stamp[base + w];
            if (age < oldest) {
                oldest = age;
                victim = w;
            }
        }
        return victim;
    }

    void set_line(int set, int way, bool is_dirty, std::uint64_t tag) {
        const int idx = set * WAYS + way;
        valid[idx] = 1;
        dirty[idx] = is_dirty ? 1 : 0;
        tags[idx] = tag;
    }

    bool is_valid(int set, int way) const { return valid[set * WAYS + way] != 0; }

    bool is_dirty(int set, int way) const { return dirty[set * WAYS + way] != 0; }

    void set_dirty(int set, int way, bool is_dirty) {
        dirty[set * WAYS + way] = is_dirty ? 1 : 0;
    }

    std::uint64_t get_tag(int set, int way) const { return tags[set * WAYS + way]; }
};

using L1Cache = CacheLevel<L1_SETS, L1_WAYS>;
using L2Cache = CacheLevel<L2_SETS, L2_WAYS>;

class CacheSimImpl final : public csot::CacheSim {
public:
    void on_init() override {
        l1_ = L1Cache{};
        l2_ = L2Cache{};
    }

    csot::CacheStats run(const csot::MemAccess* acc, std::size_t n) override {
        csot::CacheStats st{};

        for (std::size_t i = 0; i < n; ++i) {
            const std::uint64_t address = acc[i].address;
            const bool wr = acc[i].is_write != 0;

            if (wr) {
                ++st.writes;
            } else {
                ++st.reads;
            }

            const std::uint64_t b = address >> 6;
            const int s1 = static_cast<int>(b & L1_SET_MASK);
            const std::uint64_t t1 = b >> 6;

            const int w1 = l1_.find(s1, t1);
            if (w1 >= 0) {
                ++st.l1_hits;
                l1_.touch(s1, w1);
                if (wr) {
                    l1_.set_dirty(s1, w1, true);
                }
                continue;
            }

            ++st.l1_misses;

            const int s2 = static_cast<int>(b & L2_SET_MASK);
            const std::uint64_t t2 = b >> 9;

            const int w2 = l2_.find(s2, t2);
            if (w2 >= 0) {
                ++st.l2_hits;
                l2_.touch(s2, w2);
            } else {
                ++st.l2_misses;
                const int v = l2_.victim_way(s2);
                if (l2_.is_valid(s2, v) && l2_.is_dirty(s2, v)) {
                    ++st.dirty_writebacks;
                }
                l2_.set_line(s2, v, false, t2);
                l2_.touch(s2, v);
            }

            const int v1 = l1_.victim_way(s1);
            if (l1_.is_valid(s1, v1) && l1_.is_dirty(s1, v1)) {
                const std::uint64_t bv = (l1_.get_tag(s1, v1) << 6) | static_cast<std::uint64_t>(s1);
                const int s2v = static_cast<int>(bv & L2_SET_MASK);
                const std::uint64_t t2v = bv >> 9;

                const int wv = l2_.find(s2v, t2v);
                if (wv >= 0) {
                    l2_.set_dirty(s2v, wv, true);
                } else {
                    const int vv = l2_.victim_way(s2v);
                    if (l2_.is_valid(s2v, vv) && l2_.is_dirty(s2v, vv)) {
                        ++st.dirty_writebacks;
                    }
                    l2_.set_line(s2v, vv, true, t2v);
                    l2_.touch(s2v, vv);
                }
            }

            l1_.set_line(s1, v1, wr, t1);
            l1_.touch(s1, v1);
        }

        return st;
    }

private:
    L1Cache l1_{};
    L2Cache l2_{};
};

}  // namespace

extern "C" csot::CacheSim* create_cache_sim() {
    return new CacheSimImpl();
}
