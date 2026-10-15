#include "incbin.h"
#include "nnue.hpp"
#include "board.hpp"
#include <fstream>

INCBIN(NnueNet, NET_PATH);   // creates gNnueNetData and gNnueNetSize

namespace Nnue
{

std::array<std::array<int16_t, kAccumalatorSize>, kInputLayerSize> accum_weights;
std::array<int16_t, kAccumalatorSize> accum_biases;
std::array<int16_t, kAccumalatorSize> output_weights;
int32_t output_bias;

void InitAccumulator(Accumulator* accumulator, Board& board, Color stm)
{
    accumulator->values_ = accum_biases; // start from the bias

    Bitboard black_pieces = board.GetColorBB(Color::kBlack);
    for (size_t sqr{}; sqr < kSquareCount; sqr++)
    {
        Piece piece = board.GetPieceOnSquare(static_cast<Square>(sqr));

        if (piece == Piece::kNone)
        {
            continue;
        }

        Color color = ((black_pieces >> sqr) & 1) ? Color::kBlack : Color::kWhite;
        Add(accumulator, PieceToIndex(piece, static_cast<Square>(sqr), color, stm));
    }
}


// nnue.cpp

bool InitNet()
{
    const size_t expected = sizeof(accum_weights) + sizeof(accum_biases) + sizeof(output_weights) + sizeof(output_bias);
    
    if (gNnueNetSize != expected)
    {
        return false;
    }

    const unsigned char* p = gNnueNetData;
    std::memcpy(accum_weights.data(), p, sizeof(accum_weights));  p += sizeof(accum_weights);
    std::memcpy(accum_biases.data(), p, sizeof(accum_biases));   p += sizeof(accum_biases);
    std::memcpy(output_weights.data(), p, sizeof(output_weights)); p += sizeof(output_weights);
    std::memcpy(&output_bias, p, sizeof(output_bias));
    return true;
}

}