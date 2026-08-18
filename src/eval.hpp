#pragma once 

#include "attacks.hpp"
#include "board.hpp"
#include "global.hpp"
#include "search_globals.hpp"


namespace evaluation
{

    Score Evaluate(Board& board);
}