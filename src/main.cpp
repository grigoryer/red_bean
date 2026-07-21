#include "debug.hpp"
#include "board.hpp"
#include "eval.hpp"
#include "global.hpp"
#include "move.hpp"
#include "perft.hpp"
#include "move_making.hpp"
#include "move_generation.hpp"
#include <chrono>
#include <cstddef>
#include "search.hpp"
#include <iostream>
#include <ratio>
#include <string>


auto TestSearchSpeed()
{
    Board board{kPerft2};
    Search searcher(board);

    PrintPieceBoard(board);
    PrintStatus(board.GetCurStatus());
    auto start = std::chrono::high_resolution_clock::now();
    Move move = searcher.FindBestMove(6);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\nNodes: " << searcher.nodes_searched;
    std::cout << "\nTime: " << elapsed.count() << "ms";
    PrintPieceBoard(board);
    PrintStatus(board.GetCurStatus());
    PrintMove(move);

    return elapsed;
}


int main() 
{    

    auto time = TestSearchSpeed();
    return 0;
}


/*

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
    */