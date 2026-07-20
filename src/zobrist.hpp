#pragma once

#include "board.hpp"
#include "global.hpp"
#include <cassert>
#include <cstddef>
#include <random>

namespace Zobrist
{
    

template <size_t N>
inline std::array<Hash, N> InitHashTable(uint64_t seed)
{
    std::mt19937_64 engine{seed};
    std::array<Hash, N> hash{};
    for (auto& value : hash)
    {
        value = static_cast<Hash>(engine());
    }
    return hash;
}

inline std::array<std::array<std::array<Hash, kSquareCount>, kPieceCount>, kColorCount> InitPieceHash()
{
    std::mt19937_64 engine{0x1D8E4E27C2905DE1ULL};
    std::array<std::array<std::array<Hash, kSquareCount>, kPieceCount>, kColorCount> hash{};
    for (auto& color : hash)
    {
        for (auto& piece : color)
        {
            for (auto& square : piece)
            {
                square = static_cast<Hash>(engine());
            }
        }
    }
    return hash;
}


inline constexpr size_t kCastlingCount = 16;
inline const std::array<std::array<std::array<Hash, kSquareCount>, kPieceCount>, kColorCount> piece_hash = InitPieceHash();
inline const std::array<Hash, kCastlingCount> castling_hash = InitHashTable<kCastlingCount>(0x9E3779B97F4A7C15ULL);
inline const std::array<Hash, kColorCount> color_hash = InitHashTable<kColorCount>(0xD1B54A32D192ED03ULL);
inline const std::array<Hash, kSquareCount> enpassant_hash = InitHashTable<kSquareCount>(0xA24BAED4963EE407ULL);


inline Hash ZobEnpassant(Square ep_sqr)
{
    if (ep_sqr == Square::kNoSquare)
    {
        return 0;
    }
    return enpassant_hash[static_cast<size_t>(ep_sqr)];
}

inline Hash ZobPiece(Piece piece, Square sqr, Color color)
{
    return piece_hash[static_cast<size_t>(color)][static_cast<size_t>(piece)][static_cast<size_t>(sqr)]; 
}

inline Hash ZobCastling(CastlePerm castling)
{
    auto index = static_cast<size_t>(castling);
    return castling_hash[index];
}
    
inline Hash ZobColor(Color color)
{
    return color_hash[static_cast<size_t>(color)];
}
void InitailizBoardHash(Board &board);
}  // namespace Zobrist