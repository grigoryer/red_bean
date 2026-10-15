#pragma once

#include "global.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

constexpr size_t kInputLayerSize = 768;
constexpr size_t kAccumalatorSize = 1024;
constexpr int32_t kScale = 400;
constexpr int32_t QA = 255;
constexpr int32_t QB = 64;

class Board; // forward declaration

// Square: 0 - 63
// PieceType: queen = 0, rook = 1, bishop = 2, knight = 3, pawn = 4, king = 5
// Side: White = 0, Black = 1

class Accumulator
{
public:
    std::array<int16_t, kAccumalatorSize> values_{};
};

class AccumulatorPair
{
public:
    Accumulator white_accumulator_;
    Accumulator black_accumulator_;
};

// Side-to-move relative: our pieces first, their pieces second, board flipped when black is to move.
inline size_t PieceToIndex(Piece piece, Square sqr, Color color, Color stm)
{
    const size_t rel_color = (color == stm) ? 0 : 1;
    const size_t rel_sqr = (stm == Color::kWhite) ? static_cast<size_t>(sqr)
                                                  : (static_cast<size_t>(sqr) ^ 56);
    return (rel_color * 384) + (static_cast<size_t>(piece) * 64) + rel_sqr;
}

namespace Nnue
{
    // input -> accumulator
    // [input][accumulator_weight]
    extern std::array<std::array<int16_t, kAccumalatorSize>, kInputLayerSize> accum_weights;
    extern std::array<int16_t, kAccumalatorSize> accum_biases;

    // accumulator -> output
    extern std::array<int16_t, kAccumalatorSize> output_weights;
    extern int32_t output_bias;

    // Centipawns from the side to move's point of view.
    inline int Forward(const Accumulator& acc)
    {
        int32_t sum = 0;

        for (size_t i{}; i < kAccumalatorSize; ++i)
        {
            int32_t hidden = std::clamp<int32_t>(acc.values_[i], 0, QA);
            sum += hidden * output_weights[i];
        }

        return static_cast<int>((static_cast<int64_t>(sum) + output_bias) * kScale / (QA * QB));
    }

    inline void Add(Accumulator* accumulator, size_t input_idx)
    {
        for (size_t i{}; i < kAccumalatorSize; i++)
        {
            accumulator->values_[i] += accum_weights[input_idx][i];
        }
    }

    inline void Sub(Accumulator* accumulator, size_t input_idx)
    {
        for (size_t i{}; i < kAccumalatorSize; i++)
        {
            accumulator->values_[i] -= accum_weights[input_idx][i];
        }
    }

    // Rebuilds the accumulator from scratch: bias + every active feature. Defined in nnue.cpp.
    void InitAccumulator(Accumulator* accumulator, Board& board, Color stm);
    bool InitNet();
}