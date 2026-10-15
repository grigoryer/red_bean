#include "nnue.hpp"
#include "board.hpp"
#include <fstream>

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


    bool InitNet(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            return false;
        }
 
        const auto expected_size = static_cast<std::streamoff>(sizeof(accum_weights) + sizeof(accum_biases) + sizeof(output_weights) + sizeof(output_bias));
        if (file.tellg() != expected_size)
        {
            return false;
        }
 
        file.seekg(0);
        file.read(reinterpret_cast<char*>(accum_weights.data()), sizeof(accum_weights));
        file.read(reinterpret_cast<char*>(accum_biases.data()), sizeof(accum_biases));
        file.read(reinterpret_cast<char*>(output_weights.data()), sizeof(output_weights));
        file.read(reinterpret_cast<char*>(&output_bias), sizeof(output_bias));

        return true;
    }
}