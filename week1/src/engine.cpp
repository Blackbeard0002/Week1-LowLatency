#include "engine.hpp"
#include<vector>
#include<string> 
#include <fstream>
#include <sstream> 
#include <stdexcept>
#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_set>
#include "histogram.hpp"

namespace csot{
    std::vector<Tick> load_ticks(const std::string& path){
        std::ifstream in(path);
        if(!in.is_open()){
            throw std::runtime_error("Failed to open CSV");
        }

        std::vector<Tick> ticks;
        std::string line;

        if(!std::getline(in, line)){
            return ticks;
        }

        while(std::getline(in, line)){
            if(line.empty()){
                continue;
            }
            std::stringstream ss(line);
            std::string field;

            Tick t{};

            if(!std::getline(ss, field,','))continue;
            t.timestamp_ns = std::stoull(field);

            if(!std::getline(ss, field,','))continue;
            std::string symbol_stored = field;

            if(!std::getline(ss, field,','))continue;
            t.bid_px = std::stod(field);

            if(!std::getline(ss, field,','))continue;
            t.ask_px = std::stod(field);

            if(!std::getline(ss, field,','))continue;
            t.bid_qty = static_cast<uint32_t>(std::stoul(field));

            if(!std::getline(ss, field,','))continue;
            t.ask_qty = static_cast<uint32_t>(std::stoul(field));

            static std::unordered_set<std::string> symbol_pool;
            auto [it, inserted] = symbol_pool.emplace(symbol_stored);
            (void)inserted;
            t.symbol = *it;

            ticks.push_back(t);
        }

        return ticks;
    }

    void run_engine(Strategy& strategy, const std::vector<Tick>& ticks){
        LatencyHistogram histogram;
        strategy.on_init();
        for(const auto& tick : ticks){
            const auto t1 = std::chrono::steady_clock::now();
            const std::vector<Order> orders = strategy.on_tick(tick);
            const auto t2 = std::chrono::steady_clock::now();

            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
            histogram.record(ns);
            for(const auto& order : orders){
                strategy.on_fill(order, order.price, order.qty);
            }
        }
        std::cout << ticks.size() << " ticks processed" << std::endl;
        histogram.print(std::cout);
    }
}

