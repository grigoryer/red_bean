#include "debug.hpp"
#include "board.hpp"
#include "global.hpp"
#include "move.hpp"
#include "perft.hpp"
#include "move_making.hpp"
#include "move_generation.hpp"
#include <chrono>
#include <cstddef>
#include <iostream>
#include <ratio>
#include <string>


int main() 
{    
    constexpr int kDepth = 5;
    constexpr size_t kRounds = 1;

    std::chrono::milliseconds total_time{};

        Board board{};
        auto start = std::chrono::high_resolution_clock::now();
        auto nodes = perft::Perft(board, kDepth);
        auto end = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        total_time += elapsed;

    std::cout << nodes << "\n";
    std::cout << total_time.count();

    return 0;
}
