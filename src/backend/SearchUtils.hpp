/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "Position.hpp"
#include "Move.hpp"
#include "MoveOrder.hpp"
#include "Time.hpp"
#include "Score.hpp"
#include "TranspositionTable.hpp"
#include "Accumulator.hpp" 

namespace search::utils {

/* Keeping our search limitations here.
*/
class SearchLimits {
public:
    SearchLimits() = default;

    _NODISCARD _FORCEINLINE bool isTimeLimit(enumColor side) const { 
        return side == WHITE ? wtime : btime;
    }

    int32_t           depth  = 0;
    clk::milliseconds wtime  = 0,
                      btime  = 0;
    clk::milliseconds winc   = 0, 
                      binc   = 0;
    ull               nodes  = 0;
    // `qnodes` is maximum bound of quiescent nodes in search.
    // This is not implemented in UCI protocol.
    ull               qnodes = 0;
    // Toggle analysis mode (may help when Pv lines are short)
    bool              analysis_mode = false;
};

struct PvInfo;

/* Keeping search statistics and current data here.
*/
class SearchResults {
public:
    SearchResults() = default;

    void clear();

    void printBestMove();
    void print(const std::array<PvInfo, MaxSelDepth>& root_pv_line, 
               uint16_t pv_len, 
               const tt::TranspositionTable& tt);
    void printShort();
    void printPV(const std::array<PvInfo, MaxSelDepth>& root_pv_line, 
                 uint16_t pv_len);

#if defined(LEAF_COLLECT_SEARCH_STATS)
    void printSearchStats();
#endif

    // Basic search statistics
    int32_t           depth      = 0,
                      seldepth   = 0;
    sc::Score         score_cp   = 0;
    // `nodes_cnt` - total nodes count in entire search tree
    // with quiescent search
    ull               nodes_cnt  = 0,
    // `qnodes_cnt` - nodes count in quiescent search
                      qnodes_cnt = 0;
    size_t            tt_entries = 0;
    Move32b           best_move  = NullMove;
    clk::milliseconds duration   = 0;
    std::array<ull, MaxDepth + 1> 
                      nodes_per_depth = {};
    std::array<clk::milliseconds, MaxDepth + 1> 
                      time_per_depth  = {};

    // Extendend search statistics 
#if defined(LEAF_COLLECT_SEARCH_STATS)
    ull       pv_nodes_cnt      = 0,
              npv_nodes_cnt     = 0,
              cut_nodes_cnt     = 0,
              all_nodes_cnt     = 0;

    ull       tt_probe_cnt      = 0,
              qtt_probe_cnt     = 0,
              tt_cut_cnt        = 0,
              qtt_cut_cnt       = 0,
              qttmove_probe_cnt = 0;

    ull       ttmove_cut_cnt    = 0,
              qttmove_cut_cnt   = 0;

    ull       beta_cut_cnt      = 0;
    ull       qbeta_cut_cnt     = 0;

    ull       nmeval_cnt        = 0;
    ull       qeval_cnt         = 0;

    ull       rep_call_cnt      = 0;
    ull       rep_cnt           = 0;

    ull       cuckoo_rep_cnt    = 0;

    ull       reduced_search_cnt = 0,
              reduced_search_fail_high = 0,
              reduced_search_fail_low = 0;

    ull       null_moves_cnt     = 0;
    ull       null_zungzwang_detected = 0;

    ull       syzygy_tb_probe_cnt = 0;
    ull       syzygy_tb_cuts      = 0;

    std::array<ull, MaxNodeMoves>   
              move_cut_cnt = {};
    std::array<ull, MaxNodeMoves>   
              move_reduced_cnt = {};
    std::array<ull, MaxNodeMoves>   
              move_reduced_fail_high_cnt = {};
    std::array<float, MaxNodeMoves> 
              move_reduction_sum = {};
#endif
};

/* Accumulator chain inside tree stack of nodes
*/
struct AccumulatorCluster {
    nn::AccumulatorCache accum_cache;
    AccumulatorCluster*  prev_cluster;
    AccumulatorCluster*  next_cluster;
};

struct PvInfo {    
    Move16b   best_move = NullMove;
    sc::Score score = sc::Undef;
};

/* We store crucial info about current node
*  in NodeInfo class. We accumulate nodes of branch in 
*  search in stack.
*/
class NodeInfo {
public:
    NodeInfo();

    void clear();

    enumColor                 side2move;
    mvo::MoveOrder            move_picker;
    Position::ReversibleState state;
    Move32b                   move;
    Move32b                   best_move;
    sc::Score                 score;
    sc::Score                 eval;
    int32_t                   improving;
    bool                      can_move;
    sc::Score                 best_score;
    bool                      check;
    uint8_t                   moves_searched;
    uint8_t                   move_index;
    tt::TTBound               bound;
    AccumulatorCluster        cluster;
    std::array<PvInfo, MaxSelDepth> 
                              pv_line;
    uint16_t                  pv_line_len;
    bool                      is_cut;
    bool                      mate_thread;
    mvo::SMoveScore           move_score;
    mvo::hist::ContinuationSubtable*
                              continuation_subtable_ptr;
};

class SearchStack {
public:
    SearchStack();

    SearchStack(const SearchStack&)            = delete;
    SearchStack(SearchStack&&)                 = delete;
    SearchStack& operator=(const SearchStack&) = delete;
    SearchStack& operator=(SearchStack&&)      = delete;

    void clear(mem::AlignedSharedPtr<mvo::HistoryTablesCluster> history_buffer);

    NodeInfo*       getRootNode();
    const NodeInfo* getRootNode() const;
    NodeInfo*       getPreRootNode();
    const NodeInfo* getPreRootNode() const;
    const NodeInfo* getNode(unsigned ply) const;

    AccumulatorCluster* getCleanAccumulatorCluster(AccumulatorCluster* const accum_cluster);

    void updateDirtyAccumulators(AccumulatorCluster* const clean_accum_cluster,
                                 AccumulatorCluster* const accum_cluster);
private:
    static constexpr size_t         _Count = MaxSelDepth;
    mem::AlignedUniquePtr<NodeInfo> _stack;
};

_INLINE const NodeInfo* SearchStack::getNode(unsigned ply) const {
    assert(ply < _Count);
    return _stack.get() + ply + 1;
}

_INLINE NodeInfo* SearchStack::getRootNode() {
    return _stack.get() + 1;
}

_INLINE const NodeInfo* SearchStack::getRootNode() const {
    return _stack.get() + 1;
}

_INLINE NodeInfo* SearchStack::getPreRootNode() {
    return _stack.get();
}

_INLINE const NodeInfo* SearchStack::getPreRootNode() const {
    return _stack.get();
}

} // namespace search::utils
