#pragma once

#include "board.hpp"
#include "move_generation.hpp"
#include "move_making.hpp"
#include "debug.hpp"
#include <cstdint>

namespace perft
{

uint64_t Perft(Board& board, int depth)
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
        moving::MakeMove(board, moves.list[i]);
        nodes += Perft(board, depth - 1);
        moving::UndoMove(board, moves.list[i]);
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

        moving::MakeMove(board, move);
        uint64_t nodes = (depth <= 1) ? 1 : Perft(board, depth - 1);
        moving::UndoMove(board, move);

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


inline uint64_t PerftDebug(Board& board, int depth)
{
    if (depth == 0)
    {
        return 1;
    }

    MoveList moves;
    generation::legal::GenerateMoves<GenType::kAll>(board, moves);

    if (depth == 0)
    {
        return moves.move_count;
    }

    uint64_t nodes = 0;
    for (size_t i = 0; i < moves.move_count; ++i)
    {
        const Move move = moves.list.at(i);

        std::cout << "\ndepth " << depth << " | making move " << (i + 1) << "/" << moves.move_count << ": ";
        PrintMove(move);

        moving::MakeMove(board, move);

        PrintPieceBoard(board);
        PrintStatus(board.GetCurStatus());

        std::cout << "[enter to continue]";
        std::cin.get();

        nodes += PerftDebug(board, depth - 1);

        moving::UndoMove(board, move);

        PrintPieceBoard(board);
        PrintStatus(board.GetCurStatus());

        std::cout << "[enter to continue]";
        std::cin.get();
    }
    return nodes;
}

void ProfilePerft()
{

    struct TestCase 
    {
        std::string fen;
        int depth;
        size_t expected;
        const char* name;
    };

    const TestCase tests[] = 
    {
        {kStartingFen, 6, 119060324, "Perft1"},
        {kPerft2, 5, 193690690, "Perft2"},
        {kPerft3, 7, 178633661, "Perft3"},
        {kPerft4, 6, 706045033 , "Perft4"},
        {kPerft5, 5, 89941194 , "Perft5"},
        {kPerft6, 5,  164075551 , "Perft6" }
    };

    constexpr size_t kRounds = 3;

    for (const auto& test : tests)
    {
        std::cout << "=== " << test.name << " (depth " << test.depth << ") ===\n";
        std::chrono::milliseconds total_time{};
        for (size_t i = 0; i < kRounds; ++i)
        {
            Board board{test.fen};
            auto start = std::chrono::high_resolution_clock::now();
            auto nodes = perft::Perft(board, test.depth);
            auto end = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            total_time += elapsed;
            std::cout << "Run " << (i + 1) << ": " << elapsed.count() << " ms"
                    << (nodes == test.expected ? " " : "  MISMATCH: " + std::to_string(nodes))
                    << '\n';
        }
        std::cout << "Avg: " << (total_time.count() / static_cast<int>(kRounds)) << "\n\n";
    }
}

} // namespace perft


