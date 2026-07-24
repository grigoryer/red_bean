#pragma once

#include "board.hpp"
#include "search_globals.hpp"
#include "search.hpp"
#include <queue>


inline std::string MoveToString(ExtMove move)
{
    
    std::string move_str;
    move_str.reserve(5);
    move_str += SquareToString(move.GetFrom());
    move_str += SquareToString(move.GetTo());

    if (move.IsPromotion())
    {
        switch (move.GetPromo()) 
        {
            case Piece::kQueen: move_str += "q"; break; 
            case Piece::kRook: move_str += "r"; break; 
            case Piece::kBishop: move_str += "b"; break; 
            case Piece::kKnight: move_str += "n"; break; 
        }
    }
   return move_str;
}


class UCI
{
    Board board_;
    std::atomic<bool> stop_flag{true};
    std::atomic<bool> quit_flag{false};
    Search searcher_;
    

    std::condition_variable search_cv;
    std::mutex board_mutex;

    std::condition_variable queue_cv;
    std::queue<std::string> cmd_queue;
    std::mutex queue_mutex;

    void SetPosition(const std::string& input);
    void ExecuteSearch(const std::string& args);

    void CommandWorker();
public:
    void Loop();
    UCI() : searcher_(board_, stop_flag) {} ;
};
