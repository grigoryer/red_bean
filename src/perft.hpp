#pragma once

#include "board.hpp"
#include "move_generation.hpp"
#include "move_making.hpp"
#include "debug.hpp"
#include <cstdint>

namespace perft
{

[[nodiscard]] inline uint64_t Perft(Board& board, int depth)
{
    if (depth == 0)
    {
        return 1;
    }

    // Replace with your actual generator call / move list type.
    MoveList moves;
    generation::legal::GenerateMoves<GenType::kAll>(board, moves);

    if (moves.move_count == 0) //mate
    {
        return 0;
    }
    // Bulk counting: at depth 1 the answer is just the number of legal moves,
    // no need to make/unmake them. Big speedup.
    if (depth == 1)
    {
        return moves.move_count;
    }

    uint64_t nodes = 0;

    for (size_t i = 0; i < moves.move_count; ++i)
    {
        moving::make_move(board, moves.list.at(i));
        nodes += Perft(board, depth - 1);
        moving::undo_move(board, moves.list.at(i));
    }
    return nodes;
}

// Prints per-root-move node counts in UCI format for diffing against
// stockfish's `go perft N` output.
inline uint64_t PerftDivide(Board& board, int depth)
{
    MoveList moves;
    generation::legal::GenerateMoves<GenType::kAll>(board, moves);

    uint64_t total = 0;
    for (size_t i = 0; i < moves.move_count; ++i)
    {
        const Move move = moves.list.at(i);

        moving::make_move(board, move);
        uint64_t nodes = (depth <= 1) ? 1 : Perft(board, depth - 1);
        moving::undo_move(board, move);

        // UCI long algebraic: e2e4, e1g1, e7e8q — matches stockfish's output.
        std::cout << SquareToString(move.GetFrom()) << SquareToString(move.GetTo());
        if (move.IsPromotion())
        {
            switch (move.GetPromo())
            {
                case Piece::kQueen:  std::cout << 'q'; break;
                case Piece::kRook:   std::cout << 'r'; break;
                case Piece::kBishop: std::cout << 'b'; break;
                case Piece::kKnight: std::cout << 'n'; break;
                default: break;
            }
        }
        std::cout << ": " << nodes << '\n';

        total += nodes;
    }
    std::cout << "\nNodes searched: " << total << '\n';
    return total;
}


[[nodiscard]] inline uint64_t PerftDebug(Board& board, int depth)
{
    if (depth == 0)
    {
        return 1;
    }

    MoveList moves;
    generation::legal::GenerateMoves<GenType::kAll>(board, moves);

    if (depth == 1)
    {
        return moves.move_count;
    }

    uint64_t nodes = 0;
    for (size_t i = 0; i < moves.move_count; ++i)
    {
        const Move move = moves.list.at(i);

        std::cout << "\ndepth " << depth << " | making move " << (i + 1) << "/" << moves.move_count << ": ";
        PrintMove(move);

        moving::make_move(board, move);

        PrintPieceBoard(board);
        PrintStatus(board.GetCurStatus());

        std::cout << "[enter to continue]";
        std::cin.get();

        nodes += PerftDebug(board, depth - 1);

        moving::undo_move(board, move);
    }
    return nodes;
}

} // namespace perft


