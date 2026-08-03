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

    std::array<Score, kMaxPly + kEvalStackOffset> prev_evals_ = {kNegInf}; 

    std::array<std::array<ExtMove, kMaxPly>, kMaxPly> pv_table_{}; // stores the move list at ply.
    std::array<int, kMaxPly> pv_length_{}; // shows how many of moves in pv are valid.
    
    std::array<std::array<ExtMove, 2>, kMaxPly> killers_{}; // killer move table, 2 killer moves for ply.
    
    uint8_t age{}; // for tt

    // history heurstic for non capture moves.
    // Piece to color
    std::array<std::array<std::array<MoveScore, kSquareCount>, kPieceCount>, kColorCount> history_heuristic_{};

    Score QuesSearch(Score alpha, Score beta, uint8_t depth, uint8_t ply);
    void UpdatePv(uint8_t ply, ExtMove move);
    void BoostMoves(MoveList& current_moves, uint8_t ply);
    void AgeHistory();
    void UpdateHistory(ExtMove move, MoveScore bonus);
    void UpdateKillers(ExtMove move, uint8_t depth, uint8_t ply);
    bool SeeGe(ExtMove move, MoveScore threshold);

    Score NegaMax(Score alpha, Score beta, uint8_t depth, uint8_t ply, bool nmp_flag);
    std::pair<ExtMove, Score> NegaMaxWrapper(int depth, Score alpha, Score beta, ExtMove prev_best);
    
public:
    std::unique_ptr<TTable> ttable_;
    uint64_t nodes_searched{};
    
    ExtMove FindBestMove(int64_t soft_ms, size_t depth = kMaxSearchDepth);

    void ResetSearch();

    void PrintHistoryStats() const;
    explicit Search(Board& board, std::atomic<bool>& stop_flag);
};