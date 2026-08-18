#include <cstddef>
#include <iostream>
#include "debug.hpp"
#include "eval.hpp"
#include "global.hpp"
#include "uci.hpp"


int main() 
{    
    UCI engine{};
    engine.Loop();
    return 0;
}