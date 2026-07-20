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
    
    Board board{kPerft2};
    
    constexpr size_t kExpectedNodes = 193690690;
    constexpr int kDepth = 5;
    constexpr size_t kRounds = 5;

    std::chrono::milliseconds total_time{};

    for (size_t i = 0; i < kRounds; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto nodes = perft::Perft(board, kDepth);
        auto end = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        total_time += elapsed;


        std::cout << "Run " << (i + 1) << ": " << elapsed.count() << " ms"
                << (nodes == kExpectedNodes ? " " : "  MISMATCH: " + std::to_string(nodes))
                << '\n';
    }

    std::cout << "Avg: " << (total_time.count() / static_cast<int>(kRounds)) << "\n";

    return 0;
}