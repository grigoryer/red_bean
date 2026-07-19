#include "zobrist.hpp"
#include "board.hpp"
#include "global.hpp"
#include <cassert>
#include <cstddef>
#include <random>


namespace Zobrist
{

namespace
{

template <size_t N>
[[nodiscard]] std::array<Hash, N> InitHashTable(uint64_t seed)
{
    std::mt19937_64 engine{seed};
    std::array<Hash, N> hash{};
    for (auto& value : hash)
    {
        value = static_cast<Hash>(engine());
    }
    return hash;
}

[[nodiscard]] std::array<std::array<std::array<Hash, kSquareCount>, kPieceCount>, kColorCount> InitPieceHash()
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


constexpr size_t kCastlingCount = 16;
const std::array<std::array<std::array<Hash, kSquareCount>, kPieceCount>, kColorCount> piece_hash = InitPieceHash();
const std::array<Hash, kCastlingCount> castling_hash = InitHashTable<kCastlingCount>(0x9E3779B97F4A7C15ULL);
const std::array<Hash, kColorCount> color_hash = InitHashTable<kColorCount>(0xD1B54A32D192ED03ULL);
const std::array<Hash, kSquareCount> enpassant_hash = InitHashTable<kSquareCount>(0xA24BAED4963EE407ULL);

}  // namespace


Hash ZobEnpassant(Square ep_sqr)
{
    if (ep_sqr == Square::kNoSquare)
    {
        return 0;
    }
    return enpassant_hash.at(static_cast<size_t>(ep_sqr));
}

Hash ZobPiece(Piece piece, Square sqr, Color color)
{
    return piece_hash.at(static_cast<size_t>(color)).at(static_cast<size_t>(piece)).at(static_cast<size_t>(sqr)); 
}

Hash ZobCastling(CastlePerm castling)
{
    auto index = static_cast<size_t>(castling);
    assert(index <= 16);
    return castling_hash.at(index);
}
    
Hash ZobColor(Color color)
{
    return color_hash.at(static_cast<size_t>(color));
}

void InitailizBoardHash(Board &board)
{
    Hash new_hash{};
    auto& status = board.GetCurStatus();
    new_hash ^= ZobCastling(status.castling_);
    new_hash ^= ZobColor(status.cur_color_);
    new_hash ^= ZobEnpassant(status.enpassant_);
    
    for (size_t sqr_i{}; sqr_i < kSquareCount; ++sqr_i)
    {
        const auto square = static_cast<Square>(sqr_i);
        const auto piece = board.GetPieceOnSquare(square);
        if (piece == Piece::kNone)
        {
            continue;
        }

        const Color color = (board.GetColorBB(Color::kWhite) & ToBitboard(square)) != 0
                                ? Color::kWhite
                                : Color::kBlack;
        new_hash ^= ZobPiece(piece, square, color);
    }

    status.hash_ = new_hash;
}

}  // namespace Zobrist