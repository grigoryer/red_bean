#pragma once

#include "board.hpp"
#include "eval.hpp"
#include "global.hpp"
#include "move.hpp"
#include "search_globals.hpp"
#include "tt.hpp"
#include "eval.hpp"
#include <cstdint>
#include <memory>


class Search
{
    Board& board_;
    std::atomic<bool>& stop_flag_;

    std::array<std::array<ExtMove, kMaxPly>, kMaxPly> pv_table_{}; // stores the move list at ply.
    std::array<int, kMaxPly> pv_length_{}; // shows how many of moves in pv are valid.

    std::array<std::array<ExtMove, 2>, kMaxPly> killers_{}; // killer move table, 2 killer moves for ply.
    
    // history heurstic for non capture moves.
    std::array<std::array<std::array<MoveScore, kSquareCount>, kSquareCount>, kColorCount> history_heuristic_{};

    void UpdatePv(uint8_t ply, ExtMove move);
    void AgeHistory();
    Score QuesSearch(Score alpha, Score beta, uint8_t depth);


    bool SearchTTMove(ExtMove tt_move, Score& alpha, Score beta, uint8_t depth, uint8_t ply, Score& best_score, ExtMove& best_move, const Status& status, Score& out);
    bool ProbeTTCutoff(const TEntry& entry, Score alpha, Score beta, uint8_t depth, Score& out) const;
    void UpdateQuietHeuristics(ExtMove move, uint8_t depth, uint8_t ply);
    Score NegaMax(Score alpha, Score beta, uint8_t depth, uint8_t ply);


    std::pair<ExtMove, Score> NegaMaxWrapper(int depth, Score alpha, Score beta, ExtMove prev_best);
    void BoostMoves(MoveList& current_moves, uint8_t ply);
    
public:
    std::unique_ptr<TTable> ttable_;
    uint64_t nodes_searched{};
    
    ExtMove FindBestMove(size_t depth = kMaxSearchDepth);
    explicit Search(Board& board, std::atomic<bool>& stop_flag);
};