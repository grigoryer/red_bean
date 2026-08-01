#include "attacks.hpp"
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
#include <thread>

auto TestSearchSpeed(Board board, Search& searcher)
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    PrintPieceBoard(board);
    PrintStatus(board.GetCurStatus());
    auto start = std::chrono::high_resolution_clock::now();
    Move move = searcher.FindBestMove(100000, 6);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\nNodes: " << searcher.nodes_searched;
    std::cout << "\nTime: " << elapsed.count() << "ms";

    PrintStatus(board.GetCurStatus());
    searcher.ttable_->ProfileTableSpread();

    return elapsed;
}


int main() 
{    
    // UCI engine{};
    // engine.Loop();

    std::atomic<bool> flag;
    Board board{kPerft2};
    Search searcher{board, flag};
    TestSearchSpeed(board, searcher);
    
    return 0;
}