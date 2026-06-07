#include <iostream>
#include <string> 
#include <vector>
#include <iomanip>
#include <memory>

#include "engine.hpp"  
#include "strategy.hpp"

int main(int argc, char** argv){
    const std::string path = argc > 1 ? argv[1] : "data/tiny.csv";

    std::vector<csot::Tick> ticks;
    try{
        ticks = csot::load_ticks(path);
    }catch(const std::exception& e){
        std::cerr << "Error loading ticks: " << e.what() << std::endl;
        return 1;
    }
    if(ticks.empty()){
        std::cerr << "No ticks loaded from " << path << std::endl;
        return 1;
    }
    
    std::unique_ptr<csot::Strategy> strategy(create_strategy());
    if(!strategy){
        std::cerr << "Error creating strategy" << std::endl;
        return 1;
    }
    csot::run_engine(*strategy, ticks);

    return 0;
}
