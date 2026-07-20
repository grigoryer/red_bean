#pragma once
#include "global.hpp"

enum class MoveType : uint8_t
{
    kNormal = 0,
    kPromotion = 1,
    kEnPassant = 2,
    kCastle = 3
};

class Move
{
public:
    Move() = default;

    // Primary construction path — packs a consistent move in one shot.
    constexpr Move(Square from, Square to, MoveType type = MoveType::kNormal, Piece promo = Piece::kQueen)
        : data_(static_cast<uint16_t>((static_cast<uint16_t>(from) << kFromShift) | (static_cast<uint16_t>(to) << kToShift) |
        (PromoBits(promo) << kPromoShift) | (static_cast<uint16_t>(type) << kTypeShift)))
    {  }

    [[nodiscard]] constexpr Square GetFrom() const { return static_cast<Square>((data_ & kFromMask) >> kFromShift); }
    [[nodiscard]] constexpr Square GetTo() const { return static_cast<Square>((data_ & kToMask) >> kToShift); }
    [[nodiscard]] constexpr MoveType GetType() const { return static_cast<MoveType>((data_ & kTypeMask) >> kTypeShift); }

    [[nodiscard]] constexpr Piece GetPromo() const
    {
        return static_cast<Piece>(((data_ & kPromoMask) >> kPromoShift) + static_cast<int>(Piece::kQueen));
    }

    [[nodiscard]] constexpr bool IsPromotion() const { return GetType() == MoveType::kPromotion; }
    [[nodiscard]] constexpr bool IsEnPassant() const { return GetType() == MoveType::kEnPassant; }
    [[nodiscard]] constexpr bool IsCastle() const { return GetType() == MoveType::kCastle; }
    [[nodiscard]] constexpr uint16_t Raw() const { return data_; }

    [[nodiscard]] constexpr bool operator==(const Move& other) const { return data_ == other.data_; }
    [[nodiscard]] constexpr bool operator!=(const Move& other) const { return data_ != other.data_; }

private:
    static constexpr uint8_t kFromShift = 0;
    static constexpr uint8_t kToShift = 6;
    static constexpr uint8_t kPromoShift = 12;  // 2 bits: N,B,R,Q (offset from kKnight)
    static constexpr uint8_t kTypeShift = 14;  // 2 bits: MoveType

    static constexpr uint16_t kFromMask = 0x003F;  // bits 0–5
    static constexpr uint16_t kToMask = 0x0FC0;  // bits 6–11
    static constexpr uint16_t kPromoMask = 0x3000;  // bits 12–13
    static constexpr uint16_t kTypeMask = 0xC000;  // bits 14–15

    static constexpr uint16_t PromoBits(Piece piece)
    {
        return static_cast<uint16_t>(static_cast<int>(piece) - static_cast<int>(Piece::kQueen));
    }

    uint16_t data_;
};


class MoveList
{
public:
    std::array<Move, 256> list;
    size_t move_count{};

    void addMove(Move move)
    {
        list[move_count++] = move;
    }
};