#include <cstddef>
#include <iostream>
#include "debug.hpp"
#include "eval.hpp"
#include "global.hpp"
#include "nnue.hpp"
#include "uci.hpp"


int main() 
{    
    // init neural net
    if (!Nnue::InitNet("./weights/net.bin"))
    {
        return 1;
    }

    UCI engine{1};
    engine.Loop();
    return 0;
}