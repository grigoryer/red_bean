#pragma once

#include "board.hpp"
#include "global.hpp"
#include "move.hpp"
#include "search_constants.hpp"
#include "tt.hpp"
#include <cstdint>
#include <memory>
    
using ContRow = std::array<std::array<MoveScore, kSquareCount>, kPieceCount>;

class Search
{
    std::atomic<bool>& stop_flag_;

    std::array<Score, kMaxPly + kEvalStackOffset> prev_evals_ = {kNegInf}; 

    std::array<std::array<ExtMove, kMaxPly>, kMaxPly> pv_table_{}; // stores the move list at ply.
    std::array<int, kMaxPly> pv_length_{}; // shows how many of moves in pv are valid.
    
    std::array<std::array<ExtMove, 2>, kMaxPly> killers_{}; // killer move table, 2 killer moves for ply.

    uint8_t tt_age_{}; // for tt

    bool use_node_limit_{};     // only check the limit when "go nodes" was given
    uint64_t node_limit_{};
    uint64_t total_nodes_{};    // nodes for the whole go, not reset per iteration

    // history heurstic for non capture moves. Piece to color
    struct PieceTo { Piece piece{Piece::kNone}; Square to{}; };
    
    std::array<std::array<std::array<MoveScore, kSquareCount>, kPieceCount>, kColorCount> history_heuristic_{};
    std::array<std::array<ContRow, kSquareCount>, kPieceCount> cont_history_{};
    std::array<PieceTo, kMaxPly> move_stack_{};

    void CountNode();
    Score QuesSearch(Board& board, Score alpha, Score beta, uint8_t depth, uint8_t ply);

    void UpdatePv(uint8_t ply, ExtMove move);
    void BoostMoves(Board& board, MoveList& current_moves, uint8_t ply);
    ContRow* ContHistRow(uint8_t ply, uint8_t back);
    void AgeHistory();
    void UpdateHistory(Board& board, ExtMove move, MoveScore bonus, uint8_t ply);
    void UpdateKillers(Board& board, ExtMove move, uint8_t depth, uint8_t ply);

    bool SeeGe(Board& board, ExtMove move, MoveScore threshold);

    Score NegaMax(Board& board, Score alpha, Score beta, uint8_t depth, uint8_t ply, bool nmp_flag);
    std::pair<ExtMove, Score> NegaMaxWrapper(Board& board, int depth, Score alpha, Score beta, ExtMove prev_best);
    
public:
    std::shared_ptr<TTable> ttable_;
    uint64_t nodes_searched{};
    
    ExtMove FindBestMove(Board& board, int64_t soft_ms, size_t depth, size_t start_depth, size_t id_, uint64_t node_limit);
    void ResetSearch();

    explicit Search(std::atomic<bool>& stop_flag, std::shared_ptr<TTable> tt);
};