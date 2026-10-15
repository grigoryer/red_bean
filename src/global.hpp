#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <string_view>
#include <string>


const std::string kStartingFen{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};
const std::string kPerft2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
const std::string kPerft3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ";
const std::string kPerft4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
const std::string kPerft5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
const std::string kPerft6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ";

using Bitboard = uint64_t;
using Hash = uint64_t;

constexpr uint8_t kSquareCount = 64;
constexpr uint8_t kPieceCount = 6;  
constexpr uint8_t kColumnCount = 8;
constexpr uint8_t kRowCount = 8;
constexpr uint8_t kColorCount = 2;

constexpr Bitboard kFullBB = 0xFFFFFFFFFFFFFFFFULL;
constexpr Bitboard kEmptyBB = 0ULL;
// Color Enum Class
enum class Color : uint8_t
{
    kWhite,
    kBlack,
    kAll
};

// Piece Enum Class
enum class Piece : uint8_t
{
    kQueen,
    kRook,
    kBishop,
    kKnight,
    kPawn,
    kKing,
    kNone
};

constexpr std::array<int8_t, kPieceCount> kPhaseWeights = { 4, 2, 1, 1, 0, 0 };  // Q R B N P K
constexpr int8_t kMaxPhase = 24;

constexpr Piece NumToPiece(size_t piece)
{
    return static_cast<Piece>(piece);
};

enum class CastlePerm : uint8_t
{
    kNone = 0,
    kWhiteKing = 1,
    kWhiteQueen = 2,
    kWhiteBoth = 3,
    kBlackKing = 4,
    kKingBoth = 5,
    kQueenBoth = 10,
    kBlackQueen = 8,
    kBlackBoth = 12,
    kAll = 15
};



// Column Row and Square constant globals
enum class Column : uint8_t
{
    A, B, C, D, E, F, G, H
};

constexpr std::array<Bitboard, kColumnCount> kColumnsBitboards
{
    // Zeroth index is the left side column and shifting left by 1 each time moves it to the next column
    0x0101010101010101ULL,
    (0x0101010101010101ULL << 1),
    (0x0101010101010101ULL << 2),
    (0x0101010101010101ULL << 3),
    (0x0101010101010101ULL << 4),
    (0x0101010101010101ULL << 5),
    (0x0101010101010101ULL << 6),
    (0x0101010101010101ULL << 7),
};

enum class Row : uint8_t
{
    _1, _2, _3, _4, _5, _6, _7, _8
};

constexpr std::array<Bitboard, kRowCount> kRowsBitboards
{
    // Zeroth index is the bottom row and shifting left by 8 times row index moves it up 8.
    0xFFULL,
    0xFFULL << 8,
    0xFFULL << 16,
    0xFFULL << 24,
    0xFFULL << 32,
    0xFFULL << 40,
    0xFFULL << 48,
    0xFFULL << 56
};

enum class Square : uint8_t
{
    a1,b1,c1,d1,e1,f1,g1,h1,
    a2,b2,c2,d2,e2,f2,g2,h2,
    a3,b3,c3,d3,e3,f3,g3,h3,
    a4,b4,c4,d4,e4,f4,g4,h4,
    a5,b5,c5,d5,e5,f5,g5,h5,
    a6,b6,c6,d6,e6,f6,g6,h6,
    a7,b7,c7,d7,e7,f7,g7,h7,
    a8,b8,c8,d8,e8,f8,g8,h8,
    kNoSquare
};

constexpr std::array<std::string_view, kSquareCount> kSquareToStringTable
{
    "a1","b1","c1","d1","e1","f1","g1","h1",
    "a2","b2","c2","d2","e2","f2","g2","h2",
    "a3","b3","c3","d3","e3","f3","g3","h3",
    "a4","b4","c4","d4","e4","f4","g4","h4",
    "a5","b5","c5","d5","e5","f5","g5","h5",
    "a6","b6","c6","d6","e6","f6","g6","h6",
    "a7","b7","c7","d7","e7","f7","g7","h7",
    "a8","b8","c8","d8","e8","f8","g8","h8",
};

enum class GenType : uint8_t
{
    kQuiet, // non-captures, non-promotions
    kNoisy, // captures + promotions
    kAll
};

constexpr size_t ToIndex(const Square sqr) 
{
  return static_cast<size_t>(sqr);
}

constexpr Square ToSquare(const size_t sqr) 
{
  return static_cast<Square>(sqr);
}

constexpr std::string_view SquareToString(const Square sqr)
{
    return kSquareToStringTable.at(ToIndex(sqr));
}

constexpr Row GetSquareRow(const Square sqr)
{
    return static_cast<Row>(static_cast<int>(sqr) / 8);
}

constexpr Column GetSquareCol(const Square sqr)
{
    return static_cast<Column>(static_cast<int>(sqr) % 8);
}

constexpr Bitboard ToBitboard(Square sqr)
{
    return 1ULL << (static_cast<size_t>(sqr));
}

constexpr Bitboard ToBitboard(Row row)
{
    return kRowsBitboards.at(static_cast<size_t>(row));
}

constexpr Bitboard ToBitboard(Column col)
{
    return kColumnsBitboards.at(static_cast<size_t>(col));
}


template <typename EnumT, typename = std::enable_if_t<std::is_enum_v<EnumT>>>
constexpr EnumT& operator++(EnumT& enu_obj)
{
    using Underlying = std::underlying_type_t<EnumT>; // grab underlying int type of the enum
    enu_obj = static_cast<EnumT>(static_cast<Underlying>(enu_obj) + 1);
    return enu_obj;
}


// Bit Helpers
constexpr Color GetEnemyColor(Color color)
{
    return static_cast<Color>(static_cast<int>(color) ^ 1);
}

constexpr bool GetBit(const Bitboard& bb, const Square sqr) 
{
    return (bb & (1ULL << static_cast<int>(sqr))) != 0;
}

constexpr void SetBit(Bitboard& bb, const Square sqr) 
{
    bb |= (1ULL << static_cast<int>(sqr));
}

constexpr Square GetLsb(const Bitboard& bb) 
{
    return static_cast<Square>(__builtin_ctzll(bb));
}

constexpr void PopBit(Bitboard& bb, const Square sqr) 
{
    bb &= ~(1ULL << static_cast<int>(sqr));
}

constexpr Square PopLsb(Bitboard& bb) 
{
    Square sqr = GetLsb(bb);
    bb &= ~(1ULL << static_cast<int>(sqr));
    return sqr;
}

constexpr size_t BitCount(const Bitboard& bb) 
{
    return __builtin_popcountll(bb);
}

constexpr bool MoreThanOneBit(const Bitboard& bb) 
{
    return __builtin_popcountll(bb) > 1;
}