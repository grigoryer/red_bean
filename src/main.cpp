#include "debug.hpp"
#include "board.hpp"
#include "eval.hpp"
#include "global.hpp"
#include "move.hpp"
#include "perft.hpp"
#include "move_making.hpp"
#include "move_generation.hpp"
#include <atomic>
#include <chrono>
#include <cstddef>
#include "search.hpp"
#include "uci.hpp"
#include <iostream>
#include <ratio>
#include <string>


int main() 
{    
    UCI engine{};
    engine.Loop();
    return 0;
}