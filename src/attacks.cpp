
#include "global.hpp"
#include "attacks.hpp"


namespace
{
constexpr std::array<std::array<Bitboard, kSquareCount>, kColorCount> GeneratePawnAttacks()
{
    constexpr int kNWShift = 7;
    constexpr int kNEShift = 9;
    constexpr int kSWShift = -7;
    constexpr int kSEShift = -9;

    std::array<std::array<Bitboard, kSquareCount>, kColorCount> attack_table{};

    // White Pawns
    for (size_t sqr{}; sqr < kSquareCount; sqr++)
    {
        Bitboard attack{};
    
        if (GetSquareRow(ToSquare(sqr)) == Row::_8) { continue; }

        // Left attack
        if (GetSquareCol(ToSquare(sqr)) != Column::A)
        { 
            attack |= (ToBitboard(static_cast<Square>(sqr + kNWShift)));
        }

        // Right attack
        if (GetSquareCol(ToSquare(sqr)) != Column::H)
        { 
            attack |= (ToBitboard(static_cast<Square>(sqr + kNEShift)));
        }

        attack_table.at(static_cast<size_t>(Color::kWhite)).at(sqr) = attack;
    }

    // Black Pawns
    for (size_t sqr{}; sqr < kSquareCount; sqr++)
    {
        Bitboard attack{};
    
        if (GetSquareRow(ToSquare(sqr)) == Row::_1) { continue; }

        // Right attacks
        if (GetSquareCol(ToSquare(sqr)) != Column::H)
        { 
            attack |= (ToBitboard(static_cast<Square>(sqr + kSWShift)));
        }

        // Left attack
        if (GetSquareCol(ToSquare(sqr)) != Column::A)
        { 
            attack |= (ToBitboard(static_cast<Square>(sqr + kSEShift)));
        }

        attack_table.at(static_cast<size_t>(Color::kBlack)).at(sqr) = attack;
    }

    return attack_table;
}

constexpr std::array<Bitboard, kSquareCount> GenerateKingAttacks()
{
    std::array<Bitboard, kSquareCount> attack_table{};

    constexpr int kNShift = 8;
    constexpr int kSShift = -8;
    constexpr int kEShift = 1;
    constexpr int kWShift = -1;

    constexpr int kNWShift = kNShift + kWShift;
    constexpr int kNEShift = kNShift + kEShift;
    constexpr int kSWShift = kSShift + kWShift;
    constexpr int kSEShift = kSShift + kEShift;

    for (size_t sqr{}; sqr < kSquareCount; sqr++)
    {
        Bitboard attack{};

        Row sqr_row = GetSquareRow(ToSquare(sqr));
        Column sqr_col = GetSquareCol(ToSquare(sqr));
    
        // Generate North Squares
        if (sqr_row != Row::_8) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kNShift)));
        }

        // Generate South Squares
        if (sqr_row != Row::_1) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kSShift)));
        }

        // Generate East Squares
        if (sqr_col != Column::H) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kEShift)));
        }

        // Generate West Squares
        if (sqr_col != Column::A) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kWShift)));
        }

        // Generate corner Squres
        if (sqr_col != Column::A && sqr_row != Row::_1) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kSWShift)));
        }

        if (sqr_col != Column::A && sqr_row != Row::_8) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kNWShift)));
        }

        if (sqr_col != Column::H && sqr_row != Row::_1) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kSEShift)));
        }

        if (sqr_col != Column::H && sqr_row != Row::_8) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kNEShift)));
        }


        attack_table.at(sqr) = attack;
    }

    return attack_table;
};

constexpr Bitboard GenerateKnightAttack(size_t sqr)
{
     // Long means two square jumps in either south or north
    // Short means one square jumps in either south or north
    constexpr int kNELongShift = 17;
    constexpr int kNEShortShift = 10;

    constexpr int kNWLongShift = 15;
    constexpr int kNWShortShift = 6;

    constexpr int kSELongShift = -15;
    constexpr int kSEShortShift = -6;

    constexpr int kSWLongShift = -17;
    constexpr int kSWShortShift = -10;


    Bitboard attack{};

    Row sqr_row = GetSquareRow(ToSquare(sqr));
    Column sqr_col = GetSquareCol(ToSquare(sqr));

    if (sqr_row != Row::_8)
    {
        if (sqr_row != Row::_7 && sqr_col != Column::A) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kNWLongShift)));
        }

        if (sqr_row != Row::_7 && sqr_col != Column::H) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kNELongShift)));
        }

        if (sqr_col != Column::B && sqr_col != Column::A) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kNWShortShift)));
        }

        if (sqr_col != Column::G && sqr_col != Column::H) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kNEShortShift)));
        }
    }
    

    if (sqr_row != Row::_1)
    {
        if (sqr_row != Row::_2 && sqr_col != Column::A) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kSWLongShift)));
        }

        if (sqr_row != Row::_2 && sqr_col != Column::H) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kSELongShift)));
        }

        if (sqr_col != Column::B && sqr_col != Column::A) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kSWShortShift)));
        }

        if (sqr_col != Column::G && sqr_col != Column::H) 
        {
            attack |= (ToBitboard(static_cast<Square>(sqr + kSEShortShift)));
        }
    }

    return attack;
}

constexpr std::array<Bitboard, kSquareCount> GenerateKnightAttacks()
{
    std::array<Bitboard, kSquareCount> attack_table{};


    for (size_t sqr{}; sqr < kSquareCount; sqr++)
    {
        attack_table.at(sqr) = GenerateKnightAttack(sqr);
    }

    return attack_table;
};



/*
generates occupancy based on index of that square unqiue occupancies of (2^10 for bishops and 2^12 for rooks).
index represents which bits are flipped on. if index = 15 bits 1,2,4,8 are on. that means the first 4 bits are occupied
Run through possible occupied bits, from bit mask, so if bishop is on a1 that is 6 possible bits. 
Then check if this index has this bit flipped on or of, if on add this to our occupancy.

Bascailly add the lsb bit location. (5 lsb = 2^5) and push the 5th lsb in the attack mask to on.
*/
constexpr Bitboard GenOccupancy(Bitboard attack_mask, const size_t index)
{
    //blank occupancy map
    Bitboard occupancy = 0ULL;
    
    //total amount of bits added 2^bitCount in mask
    size_t MaskBitCount = BitCount(attack_mask);

    //for each relevant bit check if it is with the index.
    for(size_t count{}; count < MaskBitCount; count++)
    {
        //find square of bit and pop it.
        Square sqr = PopLsb(attack_mask);

        //if bit is flipped on in this index add it to our occuapncy lsb.
        if((index & (1 << count)) != 0)
        {
            occupancy |= ToBitboard(sqr);
        }
    }

    return occupancy;
}


constexpr size_t kBishopRelevantBitCount = 1024;


constexpr std::array<Bitboard, kSquareCount> bishop_magics = 
{
    0xFFEDF9FD7CFCFFFFULL, 0xFC0962854A77F576ULL, 0x5822022042000000ULL, 0x2CA804A100200020ULL,
    0x0204042200000900ULL, 0x2002121024000002ULL, 0xFC0A66C64A7EF576ULL, 0x7FFDFDFCBD79FFFFULL,
    0xFC0846A64A34FFF6ULL, 0xFC087A874A3CF7F6ULL, 0x1001080204002100ULL, 0x1810080489021800ULL,
    0x0062040420010A00ULL, 0x5028043004300020ULL, 0xFC0864AE59B4FF76ULL, 0x3C0860AF4B35FF76ULL,
    0x73C01AF56CF4CFFBULL, 0x41A01CFAD64AAFFCULL, 0x040C0422080A0598ULL, 0x4228020082004050ULL,
    0x0200800400E00100ULL, 0x020B001230021040ULL, 0x7C0C028F5B34FF76ULL, 0xFC0A028E5AB4DF76ULL,
    0x0020208050A42180ULL, 0x001004804B280200ULL, 0x2048020024040010ULL, 0x0102C04004010200ULL,
    0x020408204C002010ULL, 0x02411100020080C1ULL, 0x102A008084042100ULL, 0x0941030000A09846ULL,
    0x0244100800400200ULL, 0x4000901010080696ULL, 0x0000280404180020ULL, 0x0800042008240100ULL,
    0x0220008400088020ULL, 0x04020182000904C9ULL, 0x0023010400020600ULL, 0x0041040020110302ULL,
    0xDCEFD9B54BFCC09FULL, 0xF95FFA765AFD602BULL, 0x1401210240484800ULL, 0x0022244208010080ULL,
    0x1105040104000210ULL, 0x2040088800C40081ULL, 0x43FF9A5CF4CA0C01ULL, 0x4BFFCD8E7C587601ULL,
    0xFC0FF2865334F576ULL, 0xFC0BF6CE5924F576ULL, 0x80000B0401040402ULL, 0x0020004821880A00ULL,
    0x8200002022440100ULL, 0x0009431801010068ULL, 0xC3FFB7DC36CA8C89ULL, 0xC3FF8A54F4CA2C89ULL,
    0xFFFFFCFCFD79EDFFULL, 0xFC0863FCCB147576ULL, 0x040C000022013020ULL, 0x2000104000420600ULL,
    0x0400000260142410ULL, 0x0800633408100500ULL, 0xFC087E8E4BB2F736ULL, 0x43FF9E4EF4CA2C89ULL,
};

constexpr std::array<int, kSquareCount> bishop_shift = 
{
    58, 59, 59, 59, 59, 59, 59, 58, 
    59, 59, 59, 59, 59, 59, 59, 59, 
    59, 59, 57, 57, 57, 57, 59, 59, 
    59, 59, 57, 55, 55, 57, 59, 59, 
    59, 59, 57, 55, 55, 57, 59, 59, 
    59, 59, 57, 57, 57, 57, 59, 59, 
    59, 59, 59, 59, 59, 59, 59, 59, 
    58, 59, 59, 59, 59, 59, 59, 58
};


constexpr Bitboard MaskBishopAttacks(const Square sqr) 
{
    Bitboard bishop_mask = 0ULL;

    //row col
    int row{};
    int col{};

    int target_row = static_cast<int>(ToIndex(sqr)) / kRowCount;
    int target_col = static_cast<int>(ToIndex(sqr)) % kColumnCount;

    // north east
    for(row = target_row + 1, col = target_col + 1; row < static_cast<int>(Row::_8) && col < static_cast<int>(Column::H); row++, col++) 
    { 
        bishop_mask |= 1ULL << (row * kRowCount + col); 
    }
    // south west
    for(row = target_row - 1, col = target_col - 1; row > static_cast<int>(Row::_1) && col > static_cast<int>(Column::A); row--, col--) 
    { 
        bishop_mask |= 1ULL << (row * kRowCount + col);
    }
    // north west
    for(row = target_row + 1, col = target_col - 1; row < static_cast<int>(Row::_8) && col > static_cast<int>(Column::A); row++, col--) 
    { 
        bishop_mask |= 1ULL << (row * kRowCount + col); 
    }
    // south east
    for(row = target_row - 1, col = target_col + 1; row > static_cast<int>(Row::_1) && col < static_cast<int>(Column::H); row--, col++) 
    { 
        bishop_mask |= 1ULL << (row * kRowCount + col); 
    }

    return bishop_mask;
}

// For each ray (NE, SW, NW, SE) stop at the first occupied square; the rest is blocked.
constexpr Bitboard GenBishopAttacksWithOccupancy(const Bitboard& occ, const Square sqr) 
{
    Bitboard attacks = 0ULL;

    // row col
    int row = 0;
    int col = 0;

    int target_row = static_cast<int>(ToIndex(sqr) / kRowCount);
    int target_col = static_cast<int>(ToIndex(sqr) % kColumnCount);

    // north east
    for(row = target_row + 1, col = target_col + 1; row <= static_cast<int>(Row::_8) && col <= static_cast<int>(Column::H); row++, col++) 
    {
        attacks |= 1ULL << (row * kRowCount + col);
        if((occ & (1ULL << (row * kRowCount + col))) != 0) { break; }
    }
    // south west
    for(row = target_row - 1, col = target_col - 1; row >= static_cast<int>(Row::_1) && col >= static_cast<int>(Column::A); row--, col--) 
    {
        attacks |= 1ULL << (row * kRowCount + col);
        if((occ & (1ULL << (row * kRowCount + col))) != 0) { break; }
    }
    // north west
    for(row = target_row + 1, col = target_col - 1; row <= static_cast<int>(Row::_8) && col >= static_cast<int>(Column::A); row++, col--) 
    {
        attacks |= 1ULL << (row * kRowCount + col);
        if((occ & (1ULL << (row * kRowCount + col))) != 0) { break; }
    }
    // south east
    for(row = target_row - 1, col = target_col + 1; row >= static_cast<int>(Row::_1) && col <= static_cast<int>(Column::H); row--, col++) 
    {
        attacks |= 1ULL << (row * kRowCount + col);
        if((occ & (1ULL << (row * kRowCount + col))) != 0) { break; }
    }

    return attacks;
}


constexpr std::array<std::array<Bitboard, kBishopRelevantBitCount>, kSquareCount> GenerateBishopAttacks()
{
    std::array<std::array<Bitboard, kBishopRelevantBitCount>, kSquareCount> attack_table{};

    for (size_t sqr{}; sqr < kSquareCount; sqr++) //Square 
    {
        Bitboard mask = MaskBishopAttacks(ToSquare(sqr));
        size_t bits = BitCount(mask);
    
        for(size_t i{}; i < (1 << bits); i++) // Count of relevant bits.
        {
            Bitboard occ = GenOccupancy(mask, i);
            Bitboard attack = GenBishopAttacksWithOccupancy(occ, ToSquare(sqr));

            size_t index = {(occ * bishop_magics.at(sqr)) >> bishop_shift.at(sqr)};
            attack_table.at(sqr).at(index) = attack;
        } 
    }

    return attack_table;
};



constexpr size_t kRookRelevantBitCount = 4096;

//Slider piece generation
constexpr std::array<Bitboard, kSquareCount> rook_magics = 
{
    0xA180022080400230ULL, 0x0040100040022000ULL, 0x0080088020001002ULL, 0x0080080280841000ULL,
    0x4200042010460008ULL, 0x04800A0003040080ULL, 0x0400110082041008ULL, 0x008000A041000880ULL,
    0x10138001A080C010ULL, 0x0000804008200480ULL, 0x00010011012000C0ULL, 0x0022004128102200ULL,
    0x000200081201200CULL, 0x202A001048460004ULL, 0x0081000100420004ULL, 0x4000800380004500ULL,
    0x0000208002904001ULL, 0x0090004040026008ULL, 0x0208808010002001ULL, 0x2002020020704940ULL,
    0x8048010008110005ULL, 0x6820808004002200ULL, 0x0A80040008023011ULL, 0x00B1460000811044ULL,
    0x4204400080008EA0ULL, 0xB002400180200184ULL, 0x2020200080100380ULL, 0x0010080080100080ULL,
    0x2204080080800400ULL, 0x0000A40080360080ULL, 0x02040604002810B1ULL, 0x008C218600004104ULL,
    0x8180004000402000ULL, 0x488C402000401001ULL, 0x4018A00080801004ULL, 0x1230002105001008ULL,
    0x8904800800800400ULL, 0x0042000C42003810ULL, 0x008408110400B012ULL, 0x0018086182000401ULL,
    0x2240088020C28000ULL, 0x001001201040C004ULL, 0x0A02008010420020ULL, 0x0010003009010060ULL,
    0x0004008008008014ULL, 0x0080020004008080ULL, 0x0282020001008080ULL, 0x50000181204A0004ULL,
    0x48FFFE99FECFAA00ULL, 0x48FFFE99FECFAA00ULL, 0x497FFFADFF9C2E00ULL, 0x613FFFDDFFCE9200ULL,
    0xFFFFFFE9FFE7CE00ULL, 0xFFFFFFF5FFF3E600ULL, 0x0010301802830400ULL, 0x510FFFF5F63C96A0ULL,
    0xEBFFFFB9FF9FC526ULL, 0x61FFFEDDFEEDAEAEULL, 0x53BFFFEDFFDEB1A2ULL, 0x127FFFB9FFDFB5F6ULL,
    0x411FFFDDFFDBF4D6ULL, 0x0801000804000603ULL, 0x0003FFEF27EEBE74ULL, 0x7645FFFECBFEA79EULL,
};

// relevant occupancy bits depending on square. (64 - relevant occupancy bits)
constexpr std::array<int, kSquareCount> rook_shift = 
{
    52, 53, 53, 53, 53, 53, 53, 52, 
    53, 54, 54, 54, 54, 54, 54, 53, 
    53, 54, 54, 54, 54, 54, 54, 53, 
    53, 54, 54, 54, 54, 54, 54, 53, 
    53, 54, 54, 54, 54, 54, 54, 53, 
    53, 54, 54, 54, 54, 54, 54, 53, 
    53, 54, 54, 54, 54, 54, 54, 53, 
    52, 53, 53, 53, 53, 53, 53, 52
};

//create the masks for the sliding pieces, excluding edges. 
//We only need locations where a blocking piece would impact attack, meaning edges are excluded.
constexpr Bitboard MaskRookAttacks(const Square sqr) 
{
    Bitboard rook_mask = 0ULL;

    //row col
    int row{};
    int col{};

    int target_row = static_cast<int>(ToIndex(sqr)) / kRowCount;
    int target_col = static_cast<int>(ToIndex(sqr)) % kColumnCount;

    // generate the attack mask excluding walls row and files as they do not restrict movement
    // north                                             
    for(row = target_row + 1; row < static_cast<int>(Row::_8); row++) { rook_mask |= 1ULL << (row * kRowCount + target_col); }
    // south
    for(row = target_row - 1; row > static_cast<int>(Row::_1); row--) { rook_mask |= 1ULL << (row * kRowCount + target_col); }
    // east
    for(col = target_col + 1; col < static_cast<int>(Column::H); col++) { rook_mask |= 1ULL << (target_row * kRowCount + col); }
    // west
    for(col = target_col - 1; col > static_cast<int>(Column::A); col--) { rook_mask |= 1ULL << (target_row * kRowCount + col); }

    return rook_mask;
}

// Generate rook attacks for a given occupancy, on the fly.
// Unlike the masks, rays run all the way to the edge (inclusive): a blocker
// on an edge square still stops the ray, so we can't drop the walls here.
constexpr Bitboard GenRookAttacksWithOccupancy(const Bitboard& occ, const Square sqr) 
{
    Bitboard attacks = 0ULL;

    // row col
    int row = 0;
    int col = 0;

    int target_row = static_cast<int>(ToIndex(sqr) / kRowCount);
    int target_col = static_cast<int>(ToIndex(sqr) % kColumnCount);

    // north
    for(row = target_row + 1; row <= static_cast<int>(Row::_8); row++) 
    {
        attacks |= 1ULL << (row * kRowCount + target_col);
        if((occ & (1ULL << (row * kRowCount + target_col))) != 0) { break; }
    }
    // south
    for(row = target_row - 1; row >= static_cast<int>(Row::_1); row--) 
    {
        attacks |= 1ULL << (row * kRowCount + target_col);
        if((occ & (1ULL << (row * kRowCount + target_col))) != 0) { break; }
    }
    // east
    for(col = target_col + 1; col <= static_cast<int>(Column::H); col++) 
    {
        attacks |= 1ULL << (target_row * kRowCount + col);
        if((occ & (1ULL << (target_row * kRowCount + col))) != 0) { break; }
    }
    // west
    for(col = target_col - 1; col >= static_cast<int>(Column::A); col--) 
    {
        attacks |= 1ULL << (target_row * kRowCount + col);
        if((occ & (1ULL << (target_row * kRowCount + col))) != 0) { break; }
    }

    return attacks;
}

constexpr std::array<std::array<Bitboard, kRookRelevantBitCount>, kSquareCount> GenerateRookAttacks()
{
    std::array<std::array<Bitboard, kRookRelevantBitCount>, kSquareCount> attack_table{};

    for (size_t sqr{}; sqr < kSquareCount; sqr++) //Square 
    {
        Bitboard mask = MaskRookAttacks(ToSquare(sqr));
        size_t bits = BitCount(mask);
    
        for(size_t i{}; i < (1 << bits); i++) // Count of relevant bits.
        {
            Bitboard occ = GenOccupancy(mask, i);
            Bitboard attack = GenRookAttacksWithOccupancy(occ, ToSquare(sqr));

            size_t index = {(occ * rook_magics.at(sqr)) >> rook_shift.at(sqr)};
            attack_table.at(sqr).at(index) = attack;
        } 
    }

    return attack_table;
};



constexpr std::array<Bitboard, kSquareCount> kKingAttacks = GenerateKingAttacks();
constexpr std::array<Bitboard, kSquareCount> kKnightAttacks = GenerateKnightAttacks();
constexpr std::array<std::array<Bitboard, kSquareCount>, kColorCount> kPawnAttacks = GeneratePawnAttacks();

constexpr std::array<std::array<Bitboard, kBishopRelevantBitCount>, kSquareCount> kBishopAttacks = GenerateBishopAttacks();
//change of constexpr since it takes too long currently for release change to constexpr
const std::array<std::array<Bitboard, kRookRelevantBitCount>, kSquareCount> kRookAttacks = GenerateRookAttacks();




} //namespace




namespace attacks
{

Bitboard GetPawnAttack(Color color, Square sqr)
{
    return kPawnAttacks[static_cast<size_t>(color)][static_cast<size_t>(sqr)];
};


Bitboard GetKingAttack(Square sqr)
{
    return kKingAttacks[static_cast<size_t>(sqr)];
};


Bitboard GetKnightAttack(Square sqr)
{
    return kKnightAttacks[static_cast<size_t>(sqr)];
};


Bitboard GetBishopAttack(Bitboard occ, Square sqr)
{
    size_t sqr_index = ToIndex(sqr);
    occ &= MaskBishopAttacks(sqr);  
    size_t index{(occ * bishop_magics[sqr_index]) >> bishop_shift[sqr_index]}; 

    return kBishopAttacks[sqr_index][index];
};


Bitboard GetRookAttack(Bitboard occ, Square sqr)
{
    size_t sqr_index = ToIndex(sqr);
    occ &= MaskRookAttacks(sqr);  
    size_t index{(occ * rook_magics[sqr_index]) >> rook_shift[sqr_index]}; 

    return kRookAttacks[sqr_index][index];
};


Bitboard GetQueenAttack(Bitboard occ, Square sqr)
{
    size_t sqr_index = ToIndex(sqr);
    Bitboard bishop_occ = occ & MaskBishopAttacks(sqr);
    Bitboard rook_occ = occ & MaskRookAttacks(sqr);

    size_t bishop_index{(bishop_occ * bishop_magics[sqr_index]) >> bishop_shift[sqr_index]};
    size_t rook_index{(rook_occ * rook_magics[sqr_index]) >> rook_shift[sqr_index]}; 

    return kBishopAttacks[sqr_index][bishop_index] | kRookAttacks[sqr_index][rook_index];
};

} // namespace attacks