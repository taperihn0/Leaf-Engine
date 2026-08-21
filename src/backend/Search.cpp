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

#include "Search.hpp"
#include "NetworkEval.hpp"
#include "Tuning.hpp"
#include "StaticEval.hpp"

#ifdef LEAF_COLLECT_SEARCH_STATS
#include <iomanip>
#endif

#define _TT_PROBE_QSEARCH
#define _TT_PREFETCH_QSEARCH
#define _CUCKOO_DRAW

namespace search {

/* Wrapping SearchLimits onto own, private class used in Search with some utilities
*/
class SearchLimitsWrapper final : public SearchLimits {
public:
    SearchLimitsWrapper() = default;
    explicit SearchLimitsWrapper(const SearchLimits& base);

    void onSearch(const Position& pos);
    _NODISCARD _FORCEINLINE bool isTimeLeft() const;
    _NODISCARD const clk::Timer& getTimer() const;
    _NODISCARD _FORCEINLINE clk::milliseconds getSearchTime() const;
private:
    clk::milliseconds _search_time;
    clk::Timer        _timer;
    enumColor         _side2move;
};

/* Wrapping SearchResults onto own, private class used in Search with another utilities
*/
class SearchResultsWrapper final : public SearchResults {
public:
    SearchResultsWrapper() = default;
    explicit SearchResultsWrapper(const SearchResults& base);

    // `anyNodesLeft` compares current any-node count (both search and quiescent nodes)
    // and returns whether given number is below any-node threshold.
    _NODISCARD _FORCEINLINE bool anyNodesLeft(const SearchLimitsWrapper& limits);

    // `anyQuiesceNodesLeft` compares current quiescent nodes count
    // and returns whether given number is below quiescent-node threshold.
    _NODISCARD _FORCEINLINE bool anyQuiesceNodesLeft(const SearchLimitsWrapper& limits);
};

class TreeStack {
public:
    TreeStack();

    TreeStack(const TreeStack&)            = delete;
    TreeStack(TreeStack&&)                 = delete;
    TreeStack& operator=(const TreeStack&) = delete;
    TreeStack& operator=(TreeStack&&)      = delete;

    void clear(mem::AlignedSharedPtr<mvo::HistoryTablesCluster> history_buffer);

    NodeInfo*       getRootNode();
    const NodeInfo* getRootNode() const;
    NodeInfo*       getPreRootNode();
    const NodeInfo* getPreRootNode() const;
    const NodeInfo* getNode(unsigned ply) const;

    AccumulatorCluster* getCleanAccumulatorCluster(AccumulatorCluster* const accum_cluster,
                                                   NodeInfo* const preroot);

    void updateDirtyAccumulators(AccumulatorCluster* const clean_accum_cluster,
                                 AccumulatorCluster* const accum_cluster);
private:
    static constexpr size_t    _Count = MaxSelDepth;
    mem::AlignedUniquePtr<NodeInfo> _stack;
};

SearchLimitsWrapper::SearchLimitsWrapper(const SearchLimits& base) 
    : SearchLimits(base) {}

void SearchLimitsWrapper::onSearch(const Position& pos) {
    _side2move = pos.getTurn();
    _timer.reset();
    _timer.go();
    _search_time = clk::TimeManager::searchTimeMs(pos, *this);
}

_NODISCARD _FORCEINLINE bool SearchLimitsWrapper::isTimeLeft() const {
    return !isTimeLimit(_side2move) or _timer.getDurationMs() < _search_time;
}

_NODISCARD const clk::Timer& SearchLimitsWrapper::getTimer() const {
    return _timer;
}

_NODISCARD _FORCEINLINE clk::milliseconds SearchLimitsWrapper::getSearchTime() const {
    return _search_time;
}

SearchResultsWrapper::SearchResultsWrapper(const SearchResults& base)
    : SearchResults(base) {}

_NODISCARD _FORCEINLINE bool SearchResultsWrapper::anyNodesLeft(const SearchLimitsWrapper& limits) {
    return !limits.nodes or nodes_cnt < limits.nodes;
}

_NODISCARD _FORCEINLINE bool SearchResultsWrapper::anyQuiesceNodesLeft(const SearchLimitsWrapper& limits) {
    return !limits.qnodes or qnodes_cnt < limits.qnodes;
}

void SearchResults::clear() {
    mem::memSet(this, 0, sizeof(SearchResults));
}

void SearchResults::printBestMove() {
    ASSERT(!best_move.isNullMove(), "Null bestmove");

    std::cout << "bestmove ";
    best_move.print();
    std::cout << '\n';
}

void SearchResults::print(const std::array<PvInfo, MaxSelDepth>& root_pv_line, 
                          uint16_t pv_len, 
                          const tt::TranspositionTable& tt) 
{
    const uint64_t nps = static_cast<uint64_t>((nodes_cnt * 1000.f) / (duration ? duration : 1));

    std::cout  
        << "info depth " << depth            << ' '
        << "seldepth "   << seldepth         << ' '
        << "score "      << score_cp.toStr() << ' '
        << "nodes "      << nodes_cnt        << ' '
        << "time "       << duration         << ' '
        << "nps "        << nps              << ' '
        << "hashfull "   << tt.getHashfull() << ' '
        << "pv ";

    printPV(root_pv_line, pv_len);

    // flush every line
    std::cout << std::endl;

#if defined(LEAF_COLLECT_SEARCH_STATS)
    printSearchStats();
#endif
}

void SearchResults::printShort() {
    std::cout << "Total nodes: " << nodes_cnt << '\n';

    if (!best_move.isNullMove())
        printBestMove();

#if defined(LEAF_COLLECT_SEARCH_STATS)
    printSearchStats();
#endif
}

void SearchResults::printPV(const std::array<PvInfo, MaxSelDepth>& root_pv_line, 
                            uint16_t pv_len) 
{
    for (uint16_t i = 0; i < pv_len; i++) {
        const Move16b m16 = root_pv_line[i].best_move;
        m16.print();
        std::cout << ' ';
    }
}

#if defined(LEAF_COLLECT_SEARCH_STATS)
void SearchResults::printSearchStats() {

#define _MAKE_NONZERO(x)                 \
    do { x = x ? x : 1; } while (false) \

    ull nmnodes = nodes_cnt - qnodes_cnt;

    _MAKE_NONZERO(nodes_cnt);
    _MAKE_NONZERO(qnodes_cnt);
    _MAKE_NONZERO(tt_probe_cnt);
    _MAKE_NONZERO(qtt_probe_cnt);
    _MAKE_NONZERO(beta_cut_cnt);
    _MAKE_NONZERO(qtt_probe_cnt);
    _MAKE_NONZERO(qbeta_cut_cnt);
    _MAKE_NONZERO(nmnodes);

    const float qnodes_rate      = static_cast<float>(qnodes_cnt) / nodes_cnt * 100;
    const float pvnodes_rate     = static_cast<float>(pv_nodes_cnt) / nodes_cnt * 100;
    const float npvnodes_rate    = static_cast<float>(npv_nodes_cnt) / nodes_cnt * 100;
    const float cutnodes_rate    = static_cast<float>(cut_nodes_cnt) / nmnodes * 100;
    const float allnodes_rate    = static_cast<float>(all_nodes_cnt) / nmnodes * 100;

    const float qprobes_rate     = static_cast<float>(qtt_probe_cnt) / tt_probe_cnt * 100;
    const float cuts_rate        = static_cast<float>(tt_cut_cnt) / tt_probe_cnt * 100;
    const float qcuts_rate       = static_cast<float>(qtt_cut_cnt) / qtt_probe_cnt * 100;

    const float ttmove_cut_rate  = static_cast<float>(ttmove_cut_cnt) / beta_cut_cnt * 100;
    const float qttmove_rate     = static_cast<float>(qttmove_probe_cnt) / qtt_probe_cnt * 100;
    const float qttmove_cut_rate = static_cast<float>(qttmove_cut_cnt) / qbeta_cut_cnt * 100;

    const float nmeval_rate      = static_cast<float>(nmeval_cnt) / nmnodes * 100;
    const float qeval_rate       = static_cast<float>(qeval_cnt) / qnodes_cnt * 100;

    const float reduced_search_fail_rate = static_cast<float>(reduced_search_fail_low) / reduced_search_cnt * 100;
    const float reduced_search_suc_rate = static_cast<float>(reduced_search_fail_high) / reduced_search_cnt * 100;

    null_moves_cnt = null_moves_cnt ? null_moves_cnt : 1;

    const float null_zungzwang_rate = static_cast<float>(null_zungzwang_detected) / null_moves_cnt * 100;

    const float syzygy_tb_cuts_rate = static_cast<float>(syzygy_tb_cuts) / syzygy_tb_probe_cnt * 100;

    std::cout << "\n--SEARCH STATISTICS--";

    std::cout
        << "\nQUIESCENT NODES:             " << qnodes_cnt << ", " << qnodes_rate << '%'
        << "\nPV NODES:                    " << pv_nodes_cnt << ", " << pvnodes_rate << '%'
        << "\nNON PV NODES:                " << npv_nodes_cnt << ", " << npvnodes_rate << '%'
        << "\nCUT NODES:                   " << cut_nodes_cnt << ", " << cutnodes_rate << '%'
        << "\nALL NODES:                   " << all_nodes_cnt << ", " << allnodes_rate << '%'
        << "\nTT PROBES:                   " << tt_probe_cnt
        << "\nTT PROBES IN QSEARCH:        " << qtt_probe_cnt << ", " << qprobes_rate << '%'
        << "\nTT CUTS:                     " << tt_cut_cnt << ", " << cuts_rate << '%'
        << "\nTT CUTS IN QSEARCH:          " << qtt_cut_cnt << ", " << qcuts_rate << '%'
        << "\nHASH-MOVE CUT:               " << ttmove_cut_cnt << ", " << ttmove_cut_rate << '%'
        << "\nHASH-MOVE PROBE IN QSEARCH:  " << qttmove_probe_cnt << ", " << qttmove_rate << '%'
        << "\nHASH-MOVE CUT IN QSEARCH:    " << qttmove_cut_cnt << ", " << qttmove_cut_rate << '%'
        << "\nEVAL CALLS IN NEGA-M-SEARCH: " << nmeval_cnt << ", " << nmeval_rate << '%'
        << "\nEVAL CALLS IN QSEARCH:       " << qeval_cnt << ", " << qeval_rate << '%'
        << "\nREPETITION CALLS:            " << rep_call_cnt
        << "\nREPETITION CYCLES:           " << rep_cnt
        << "\nCUCKOO CYCLES:               " << cuckoo_rep_cnt
        << "\nREDUCTION SEARCHES:          " << reduced_search_cnt << ", "
        << "\nREDUCTION SEARCH FAIL LOW:   " << reduced_search_fail_low << ", " << reduced_search_fail_rate << '%'
        << "\nREDUCTION SEARCH FAIL HIGH:  " << reduced_search_fail_high << ", " << reduced_search_suc_rate << '%'
        << "\nZUNGZWANGS DETECTED:         " << null_zungzwang_detected << ", " << null_zungzwang_rate << '%'
        << "\nSYZYGY TB CUTS:              " << syzygy_tb_probe_cnt << ", " << syzygy_tb_cuts_rate << '%'
        << "\n";

    beta_cut_cnt = !beta_cut_cnt ? 1 : beta_cut_cnt;

    // Move index stats
    std::cout << "\n------ MOVE STATS ------";

    for (size_t i = 0; i < MaxNodeMoves / 2; i++) {
        const float ind_cut_rate = static_cast<float>(move_cut_cnt[i]) / beta_cut_cnt * 100;
        const float ind_reduced_fail_high_rate = move_reduced_cnt[i] > 0 ? static_cast<float>(move_reduced_fail_high_cnt[i]) 
                                                                                              / move_reduced_cnt[i] * 100
                                                                         : 0.f;
        const float ind_reduction_avg = move_reduced_cnt[i] > 0 ? move_reduction_sum[i] / move_reduced_cnt[i] 
                                                                : 0.f;

        std::cout << "\nMOVE INDEX " << std::setw(3) << i << " [BETA-CUTOFF, LATE-FAIL-HIGH, AVG-REDUCT]: "
                  << std::setprecision(2) << std::setw(5) << std::fixed << ind_cut_rate << "%, "
                  << std::setprecision(2) << std::setw(5) << std::fixed << ind_reduced_fail_high_rate << "%, "
                  << std::setprecision(2) << std::setw(5) << std::fixed << ind_reduction_avg;
    }

    std::cout << "\n---------------------\n";

#undef _MAKE_NONZERO

}
#endif

NodeInfo::NodeInfo() { clear(); }

void NodeInfo::clear() {
    side2move        = WHITE;
    state            = {};
    best_move = move = NullMove;
    score            = sc::Undef;
    eval             = sc::Undef;
    improving        = 0;
    can_move         = false;
    best_score       = sc::Undef;
    check            = false;
    moves_searched   = 0;
    move_index       = 0;
    bound            = tt::TTBound::NONE;
    is_cut           = false;
    mate_thread      = false;

    cluster.accum_cache.clearBuffers();
    cluster.next_cluster = nullptr;
    cluster.prev_cluster = nullptr;

    mem::fill(pv_line.begin(), pv_line.end(), PvInfo());

    pv_line_len = 0;

    continuation_subtable_ptr = nullptr;
}

void Search::clearHash() {
    _tt.clear();
    _tt.clearHashfull();
}

void Search::resizeHash(size_t tt_size_mb) {
    if (tt_size_mb > 0 and 
        tt_size_mb != _tt.getEntriesCount() * sizeof(tt::TTEntry)) 
    {
        _tt.resize(tt_size_mb);
        _tt.clear();
        _tt.clearHashfull();
    }
}

void Search::onNewGame() {
    clearHash();
    _history_cluster->clear();
    _tree_stack->clear(_history_cluster);
}

TreeStack::TreeStack()
    : _stack(mem::makeAlignedUnique<NodeInfo>(_Count, CachelineSize))
{
    ASSERT(_stack != nullptr, "Failed to allocate memory");
}

void TreeStack::clear(mem::AlignedSharedPtr<mvo::HistoryTablesCluster> history_buffer) {
    ASSERT_NO_LOG(history_buffer);

    mvo::MoveOrder::setHistoryBuffer(history_buffer);

    for (int i = 0; i < static_cast<int>(_Count); i++) {
        NodeInfo& node = _stack.get()[i];
        node.clear();
        node.cluster.prev_cluster = i - 1 >= 0 ? &_stack.get()[i - 1].cluster : nullptr;
        node.cluster.next_cluster = i + 1 < static_cast<int>(_Count) ? &_stack.get()[i + 1].cluster : nullptr;
    }
}

_INLINE const NodeInfo* TreeStack::getNode(unsigned ply) const {
    assert(ply < _Count);
    return _stack.get() + ply + 1;
}

_INLINE NodeInfo* TreeStack::getRootNode() {
    return _stack.get() + 1;
}

_INLINE const NodeInfo* TreeStack::getRootNode() const {
    return _stack.get() + 1;
}

_INLINE NodeInfo* TreeStack::getPreRootNode() {
    return _stack.get();
}

_INLINE const NodeInfo* TreeStack::getPreRootNode() const {
    return _stack.get();
}

AccumulatorCluster* TreeStack::getCleanAccumulatorCluster(AccumulatorCluster* const accum_cluster,
                                                          NodeInfo* const preroot)
{
    for (AccumulatorCluster* prev_accum_cluster = accum_cluster->prev_cluster;
         prev_accum_cluster != &preroot->cluster;
         prev_accum_cluster = prev_accum_cluster->prev_cluster) {

        if (!prev_accum_cluster->accum_cache.isDirty())
            return prev_accum_cluster;
    }

    return &preroot->cluster;
}

void TreeStack::updateDirtyAccumulators(AccumulatorCluster* const clean_accum_cluster,
                                        AccumulatorCluster* const accum_cluster)
{
    for (AccumulatorCluster* prev_cluster = clean_accum_cluster->next_cluster;
         prev_cluster != accum_cluster;
         prev_cluster = prev_cluster->next_cluster) {

        MultiArray<uint16_t, 2, 2> added_features_index;
        MultiArray<uint16_t, 2, 2> removed_features_index;

        nn::AccumulatorCache& accum_cache = prev_cluster->accum_cache;

        for (size_t i = 0; i < accum_cache.added_features_cnt; i++) {
            nn::FeatureData feature_data = accum_cache.added_features[i];

            added_features_index[WHITE][i] = nn::Accumulator::featureIndex<WHITE>(
                                                                    feature_data.sq,
                                                                    feature_data.piece_type,
                                                                    feature_data.side);

            added_features_index[BLACK][i] = nn::Accumulator::featureIndex<BLACK>(
                                                                    feature_data.sq,
                                                                    feature_data.piece_type,
                                                                    feature_data.side);
        }

        for (size_t i = 0; i < accum_cache.removed_features_cnt; i++) {
            nn::FeatureData feature_data = accum_cache.removed_features[i];

            removed_features_index[WHITE][i] = nn::Accumulator::featureIndex<WHITE>(
                                                                    feature_data.sq,
                                                                    feature_data.piece_type,
                                                                    feature_data.side);

            removed_features_index[BLACK][i] = nn::Accumulator::featureIndex<BLACK>(
                                                                    feature_data.sq,
                                                                    feature_data.piece_type,
                                                                    feature_data.side);
        }

        const nn::AccumulatorCache& prev_accum_cache = prev_cluster->prev_cluster->accum_cache;
        assert(prev_accum_cache.isClean());

        accum_cache.accum.update(nn::GlobPackedNetwork,
                                 &prev_accum_cache.accum,
                                 added_features_index[WHITE].data(),
                                 accum_cache.added_features_cnt,
                                 removed_features_index[WHITE].data(),
                                 accum_cache.removed_features_cnt,
                                 WHITE);
        accum_cache.accum.update(nn::GlobPackedNetwork,
                                 &prev_accum_cache.accum,
                                 added_features_index[BLACK].data(),
                                 accum_cache.added_features_cnt,
                                 removed_features_index[BLACK].data(),
                                 accum_cache.removed_features_cnt,
                                 BLACK);
        accum_cache.markClean();
    }
}

Search::Search(tt::TranspositionTable&& tt) 
    : _tt(std::move(tt))
    , _tree_stack(std::make_unique<TreeStack>())
    , _history_cluster(mem::makeAlignedShared<mvo::HistoryTablesCluster>(1, CachelineSize))
{
    ASSERT(_history_cluster != nullptr, "Failed to allocate memory");
    onNewGame();
    _cuckoo_tables.init();
}

Search::~Search() = default;

template <enumInfoLevel InfoLevel>
Move32b Search::findBestMove(Position& pos, 
                             const FullInfoRecord& game, 
                             SearchLimits limits) 
{
    SearchResults search_results;
    const Move32b bm = findBestMove(pos, game, limits, search_results);
    return bm;
}

template <enumInfoLevel InfoLevel>
Move32b Search::findBestMove(Position& pos, 
                             const FullInfoRecord& game, 
                             SearchLimits limits,
                             SearchResults& results) 
{
    ASSERT(1 <= limits.depth and limits.depth <= MaxDepth, "Invalid depth");

    _tt.newGeneration();
    _history_cluster->onSearch();
    
    SearchLimitsWrapper search_limits(limits);
    search_limits.onSearch(pos);

    SearchResultsWrapper search_results(results);
    search_results.best_move = NullMove;

    const Move32b bm = goIterativeDeepening(pos, game, search_limits, search_results, InfoLevel);
    results = search_results;
    return bm;
}

Move32b Search::_findBestMove_unittest(Search& search, 
                                       Position& pos, 
                                       const FullInfoRecord& game, 
                                       SearchLimits limits) 
{
    return search.findBestMove<SEARCH_SHORT_INFO>(pos, game, limits);
}

Move32b Search::goIterativeDeepening(Position& pos,    
                                     const FullInfoRecord& game, 
                                     const SearchLimitsWrapper& limits,
                                     SearchResultsWrapper& search_results,
                                     enumInfoLevel info_lv) 
{    
    NodeInfo* preroot = _tree_stack->getPreRootNode();
    preroot->cluster.accum_cache.accum.refresh(nn::GlobPackedNetwork, pos);
    preroot->cluster.accum_cache.markClean();
    preroot->move = preroot->best_move = game.getMoveCount() > 0 ? game.getCurrentMove() 
                                                                 : NullMove;
    preroot->side2move = !pos.getTurn();
    preroot->mate_thread = false;

    NodeInfo* root = _tree_stack->getRootNode();

    root->cluster.prev_cluster = &preroot->cluster;
    preroot->cluster.next_cluster = &root->cluster;

    _contempt = 0;
    bool unstable = false;

    root->side2move = pos.getTurn();
    root->best_move = NullMove;
    root->best_score = sc::Undef;

    const sc::Score eval = evaluate<PV_NODE>(pos, _tree_stack.get(), 
                                             root, preroot, 
                                             pos.getTurn(), search_results);
    root->eval = eval;

    for (int d = 1; d <= limits.depth; d++) {
        search_results.depth = d;

        /* We approximate how much time we need for 
        *  next search. This is done via Efective Branch Factor (EBF)
        *  estimation. We also consider unstability of the search in order
        *  to setup a rational breaking system when we got too little time
        *  for deeper search.
        */
        if (limits.isTimeLimit(root->side2move) and d - 2 > 0 and 
            search_results.nodes_per_depth[d - 2] > 0) 
        {
            double ef_branch_factor = 1. * search_results.nodes_per_depth[d - 1] / 
                                      search_results.nodes_per_depth[d - 2];

            if (d - 3 > 0 and 
                search_results.nodes_per_depth[d - 3] > 0) 
            {
                double ef_branch_factor2 = 1. * search_results.nodes_per_depth[d - 2] / 
                                             search_results.nodes_per_depth[d - 3];

                ef_branch_factor = (ef_branch_factor + ef_branch_factor2) / 2.;
            }

            // avoid strange instabilities in shallow depths
            ef_branch_factor = std::clamp(ef_branch_factor, 
                                          MinTimeBranchFactor, 
                                          MaxTimeBranchFactor); 

            const clk::milliseconds approx_search_time = static_cast<clk::milliseconds>(search_results.time_per_depth[d - 1] * 
                                                                        ef_branch_factor);

            const float time_margin_mult = unstable ? (UnstableMultMargin / 4.f) : 1.f;

            if (time_margin_mult * limits.getSearchTime() < 4. * approx_search_time / NextDepthTimeRed)
                break;
        }

        const ull               prev_total_node_cnt = d > 1 ? search_results.nodes_cnt : 0;
        const clk::milliseconds prev_total_duration = d > 1 ? search_results.duration  : 0_ms;
        const sc::Score         prev_best_score     = d > 1 ? root->best_score         : sc::Undef;
        const Move32b           prev_best_move      = d > 1 ? root->best_move          : NullMove;

        // Adjust contempt factor based on a corrected evaluation
        const sc::Score corr_eval = correctedEvalScore(eval, prev_best_score);

        _contempt = unstable ? 0 : static_cast<sc::Score::value_type>(corr_eval / ContemptDiv);

        // Inject previous PV line to hash table
        refreshPVinTT(pos, root->pv_line, root->pv_line_len, search_results);

        int aspiration_win = AspirationFirstWindow 
                             - std::min(d, AspirationMaxDepthInfl) * AspirationDepthRate 
                             + unstable * AspirationUnstableFactor;

        sc::Score alpha = -sc::Mate;
        sc::Score beta = +sc::Mate;

        if (abs<sc::Score::value_type>(prev_best_score.value()) < sc::Win.value()) {
            const sc::Score::value_type prev_best_score_abs = abs<sc::Score::value_type>(
                                                           static_cast<sc::Score::value_type>(prev_best_score));
            aspiration_win += prev_best_score_abs * prev_best_score_abs / AspirationWindowScoreDiv;

            if (d >= AspirationSearchDepth)    {
                alpha = std::max<int16_t>(static_cast<int16_t>(prev_best_score) - aspiration_win, 
                                          -sc::Mate.value());
                beta  = std::min<int16_t>(static_cast<int16_t>(prev_best_score) + aspiration_win, 
                                          +sc::Mate.value());
            }
        }

        bool terminate = false;

        assert(AspirationCount > 0);
        assert(aspiration_win > 0);

        for (int i = 1; i <= AspirationCount; i++) {
            terminate = !goSearch(pos, game, limits, search_results, 
                                  alpha, beta);

            if (terminate)
                break;
            else if ((root->best_score > alpha and root->best_score < beta) or
                     root->best_score.isMateScore()) 
                break;
            
            aspiration_win = roundi<float>(AspirationWidenRate / 4.f * aspiration_win);

            if (i + 1 >= AspirationCount or
                aspiration_win >= AspirationMaxWindow) {
                alpha = -sc::Mate;
                beta = +sc::Mate;
                continue;
            }

            if (root->best_score <= alpha) {
                alpha = std::max<int>(static_cast<int>(alpha) - aspiration_win,
                                      -sc::Mate.value());
            }
            else {
                beta = std::min<int>(static_cast<int>(beta) + aspiration_win,
                                     +sc::Mate.value());
            }
        }

        if (terminate) {
            if (search_results.best_move.isNullMove())
                search_results.best_move = root->best_move;

            break;
        }

        if (info_lv == SEARCH_FULL_INFO) {
            search_results.print(root->pv_line, root->pv_line_len, _tt);
        }

        search_results.best_move = root->best_move;

        search_results.nodes_per_depth[d] = search_results.nodes_cnt - prev_total_node_cnt;
        search_results.time_per_depth[d]  = search_results.duration  - prev_total_duration;

        /* Setting up unstability flag */

        unstable = false;

        if (prev_best_score.isValid() and root->best_score.isValid()) {
            const int score_diff = static_cast<int>(root->best_score) - static_cast<int>(prev_best_score);
            unstable |= (abs(score_diff) > UnstableMatMargin);
        }
        
        if (!prev_best_move.isNullMove() and !root->best_move.isNullMove())
            unstable |= (prev_best_move != root->best_move);
    }

    if (info_lv == SEARCH_FULL_INFO or 
        info_lv == SEARCH_ONLY_BM_INFO) {

        if (!search_results.best_move.isNullMove())
            search_results.printBestMove();
    }
    else if (info_lv == SEARCH_SHORT_INFO) {
        search_results.printShort();
    }

    return search_results.best_move;
}

bool Search::goSearch(Position& pos, 
                      const FullInfoRecord& game, 
                      const SearchLimitsWrapper& limits, 
                      SearchResultsWrapper& results,
                      sc::Score alpha, sc::Score beta) 
{
    NodeInfo* root = _tree_stack->getRootNode();

    const sc::Score root_score = -nmSearch<PV_NODE, false, true>(pos, limits, results, game, root, 
                                                                 alpha, beta, 
                                                                 results.depth, 
                                                                 0);

    results.seldepth = std::max(results.seldepth, results.depth);
    
    /* Terminate on resource exhaustion or
    *  invalid root score.
    */
    if (!limits.isTimeLeft() or 
        !results.anyNodesLeft(limits) or 
        !results.anyQuiesceNodesLeft(limits) or
        !root_score.isValid())
        return false;

    results.duration = limits.getTimer().getDurationMs();
    return true;
}

template <enumNode NmNodeType, bool AllowNullMove, bool Root>
sc::Score Search::nmSearch(Position& pos, 
                           const SearchLimitsWrapper& limits, 
                           SearchResultsWrapper& results, 
                           const FullInfoRecord& game, 
                           NodeInfo* node,
                           sc::Score alpha, sc::Score beta, 
                           int depth, int ply) 
{
    assert(0 <= depth and depth <= MaxSelDepth);
    assert(0 <= ply and ply <= MaxSelDepth);
    assert(alpha < beta);

    if constexpr (Root) assert(!ply);
    else                assert(ply > 0);

    NodeInfo* const preroot = _tree_stack->getPreRootNode();
    node->side2move = pos.getTurn();
    node->pv_line_len = 0;

    static constexpr mvo::enumOrderPolicy OrderPolicy = Root ? mvo::ONCE_GEN_LEGAL : mvo::STAGED;
    static constexpr bool               IsPv        = NmNodeType & PV_NODE;

    assert(IsPv or alpha == beta - 1);

    if constexpr (!Root) {
        if (pos.getHalfmoveClock() >= 100 or isInsufficientMaterial(pos))
            return getDrawScore(node);
    }
    
    NodeInfo* const parent_node = node - 1;
    NodeInfo* const grandparent_node = Root ? nullptr : node - 2 ;

    if constexpr (!Root) {
        if (isRepetitionCycle<IsPv>(pos, game, node, ply, results)) {
#if defined(LEAF_COLLECT_SEARCH_STATS)
            results.rep_cnt++;
#endif // LEAF_COLLECT_SEARCH_STATS
            return getDrawScore(node);
        }
    }

#if defined(_CUCKOO_DRAW)

    if constexpr (!Root and !IsPv) {
        const sc::Score draw_score = getDrawScore(node);

        if (beta <= draw_score and canRepetitionDraw(pos, node, ply)) {
#if defined(LEAF_COLLECT_SEARCH_STATS)
            results.cuckoo_rep_cnt++;
#endif // LEAF_COLLECT_SEARCH_STATS
            return draw_score;
        }
    }

#endif // _CUCKOO_DRAW
    
    else if (!Root and (results.nodes_cnt & CheckNodeCount) == 0 and 
             !limits.isTimeLeft()) {
        return -sc::Undef;
    }
    
    else if (!Root and (!results.anyNodesLeft(limits) or
                        !results.anyQuiesceNodesLeft(limits))) {
        return -sc::Undef;
    }

    const uint64_t hash = pos.getZobristKey();

#if defined(LEAF_COLLECT_SEARCH_STATS)
    results.tt_probe_cnt++;
#endif // LEAF_COLLECT_SEARCH_STATS

    tt::TTEntry tt_entry;
    tt_entry.eval = sc::Undef;
    tt_entry.move = NullMove;
    tt_entry.score = sc::Undef;

    const bool tt_hit = _tt.probe(tt_entry, hash, alpha, beta, depth);
    const bool exact_hit = !IsPv and tt_hit;

    if (!Root and exact_hit) {
#if defined(LEAF_COLLECT_SEARCH_STATS)
        results.tt_cut_cnt++;
#endif // LEAF_COLLECT_SEARCH_STATS
        return tt_entry.score;
    }
    
    /* Syzygy tablebase probing -
    *  We probe the tablebase only at high depth, 
    *  when there is a chance to cut off bigger branch.
    *  We need to have zeroed halfmove clock and no castling rights.
    *  Probing is disabled while in root.
    */
    if constexpr (UseSyzygyTablebase) {
        if (SyzygyTablebase::get().isLoaded() and
            depth >= TablebaseProbeDepth and
            pos.getHalfmoveClock() == 0 and
            !pos.getCastlingByColor(WHITE).isAnyPossible() and 
            !pos.getCastlingByColor(BLACK).isAnyPossible() and
            pos.getPiecesCount() <= TablebasePieceCountLimit)
        {
            if constexpr (Root and UseSyzygyTablebaseRoot) {
                SyzygyTablebase::TbWdlInfo wdl;
                uint dtz;
                Move16b tb_move;

                const bool status = SyzygyTablebase::get().probeDtz(pos, wdl, dtz, tb_move);

#if defined(LEAF_COLLECT_SEARCH_STATS)
                results.syzygy_tb_probe_cnt++;
                results.syzygy_tb_cuts += status;
#endif // LEAF_COLLECT_SEARCH_STATS

                if (status) {
                    assert(tb_move != NullMove);
                    assert(wdl != SyzygyTablebase::WDL_INVALID);

                    const Move32b tb_move32 = unpackedMove(pos, tb_move);
                    assert(tb_move32.isPseudoLegal(pos));

                    node->best_move = tb_move32;
                    node->pv_line[0].best_move = packedMove(tb_move32);
                    node->pv_line_len = 1;

                    const sc::Score tb_score = getTablebaseScore(wdl, pos, node, ply);
                    
                    assert(tb_score.isValid());
                    node->best_score = tb_score;

                    return node->best_score;
                }
            }

            if constexpr (!Root) {
                SyzygyTablebase::TbWdlInfo wdl;
                const bool status = SyzygyTablebase::get().probeWdl(pos, wdl);

#if defined(LEAF_COLLECT_SEARCH_STATS)
                results.syzygy_tb_probe_cnt++;
                results.syzygy_tb_cuts += status;
#endif // LEAF_COLLECT_SEARCH_STATS

                if (status) {
                    const sc::Score tb_score = getTablebaseScore(wdl, pos, node, ply);

                    _tt.write(hash,
                              tt::EntryMaxDepth, ply,
                              tt::TTBound::EXACT,
                              tb_score, tt_entry.move, tt_entry.eval);

                    return tb_score;
                }
            }
        }
    }

    if (!depth) {
        return qSearch<QUIESCE_NODE | NmNodeType>(pos, limits, results, node,
                                                  alpha, beta,
                                                  depth,
                                                  ply);
    }

    if constexpr (Root) {
        node->is_cut = false;
        node->check = pos.isInCheck(node->side2move);
    }

    results.nodes_cnt++;

#if defined(LEAF_COLLECT_SEARCH_STATS)
    results.pv_nodes_cnt += IsPv;
    results.npv_nodes_cnt += !IsPv;
    results.cut_nodes_cnt += node->is_cut;
    results.all_nodes_cnt += !node->is_cut;
#endif // LEAF_COLLECT_SEARCH_STATS

    NodeInfo* const child_node = node + 1;
    assert(child_node - preroot < MaxSelDepth);

    node->move = NullMove;
    node->eval = tt_entry.eval;
    node->state = pos.getReversibleState();
    node->improving = 0;
    node->mate_thread = false;

    if (!node->eval.isValid()) {
        node->eval = evaluate<NmNodeType>(pos, _tree_stack.get(), 
                                          node, preroot, 
                                          node->side2move, results);
    }

    Move32b ttm32b = unpackedMove(pos, tt_entry.move);
    Move32b tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b 
                                                : NullMove;

    /* Razoring -
    *  if we're at lower depth and the eval is really low
    *  it means there is high probability no move can increase the beta bar.
    *  To ensure our intuition, we dive into quiescence search to verify the position.
    *  If we fail low, we've got a cutoff.
    */
    if constexpr (!Root and !IsPv) {
        if (!node->check and
            depth <= RazorDepth and
            beta < RazorBetaLimit and
            !grandparent_node->mate_thread and 
            tt_entry.score < sc::KnownWin and
            tt_entry.score > -sc::KnownWin and
            (tt_move.isNullMove() or tt_move.isQuiet()))
        {            
            const int32_t razor_margin = RazorBaseDelta + RazorMultDelta * depth + !node->is_cut * RazorCutDelta;
            int32_t corr_eval = static_cast<int32_t>(correctedEvalScore(node->eval, tt_entry.score));

            if (corr_eval + razor_margin < static_cast<int32_t>(beta)) {
                const sc::Score qscore = qSearch<QUIESCE_NODE | NON_PV_NODE>(pos, limits, results, node,
                                                                             beta - 1, beta,
                                                                             depth - 1,
                                                                             ply);
                
                if (qscore < beta) {
                    return qscore;
                }
            }
        }
    }

    /* Internal Iterative Deepening -
    *  done only in PV Nodes. When no hash move is found for said node, 
    *  we allow to do some shallow research in order to obtain one.
    *  That strategy can only pay off when the move ordering is actually 
    *  very important.
    */
    if constexpr (!Root and IsPv) {
        if (depth >= IidDepth and 
            tt_move.isNullMove() and
            node->is_cut) 
        {
            child_node->is_cut = !node->is_cut;

            _UNUSED const sc::Score iid_score =
                nmSearch<NmNodeType, false>(pos, limits, results, game, node,
                                            alpha, beta,
                                            4 * depth / IidDepthDiv,
                                            ply);

            tt::TTEntry iid_entry;
            iid_entry.eval = sc::Undef;
            iid_entry.move = NullMove;
            iid_entry.score = sc::Undef;

            _UNUSED const bool iid_tt_hit = _tt.probe(iid_entry, hash, alpha, beta, depth);
            
            ttm32b = unpackedMove(pos, iid_entry.move);
            tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b 
                                                : NullMove;
        }
    }

    /* Dynamic Improving implementation -
    *  we're clamping improvement rate to range [-FixedPointMult, +FixedPointMult]
    */
    if constexpr (!Root and !IsPv) {
        if (!node->check) {
            const NodeInfo* prev_eval_node = nullptr;

            if (const NodeInfo* s2m_node = node - 2; 
                node - preroot > 2 and 
                s2m_node->eval.isValid())
                prev_eval_node = s2m_node;

            else if (const NodeInfo* s2m_node = node - 4; 
                     node - preroot > 4 and 
                     s2m_node->eval.isValid())
                prev_eval_node = node - 4;
        
            if (prev_eval_node) {
                const int32_t diff = static_cast<int32_t>(node->eval - prev_eval_node->eval);
                node->improving = std::clamp(prev_eval_node->improving + diff * FixedPointMult / ImprovingRate, 
                                             -FixedPointMult, FixedPointMult);
            }
        }
    }

    /* Reverse Futility Pruning (Static Null Move Pruning) -
    *  basically, when we're doing very well, we can prune.
    *  Idea similar to Standing Pat cutoff in Q-Search.
    */
    if constexpr (!Root and !IsPv) {
        if (!node->check and
            depth <= RfpDepth and
            !grandparent_node->mate_thread and
            (tt_move.isNullMove() or tt_move.isQuiet()))
        {            
            const int16_t quiet_penalty = getRfpQuietHistPenalty(parent_node);

            const int32_t rfp_improving_scale = RfpImprovingSinkMult * -node->improving / 256 + FixedPointMult;
            const int16_t rfp_margin = quiet_penalty + static_cast<int16_t>(rfp_improving_scale * RfpMultDelta * depth / FixedPointMult);

            if (node->eval - std::max<int16_t>(rfp_margin, RfpMarginThreshold) >= beta) {
                const sc::Score rfp_value = (static_cast<int32_t>(node->eval) * (128 - RfpReturnValueWeight) + 
                                             static_cast<int32_t>(beta)       * RfpReturnValueWeight) / 128;
                return rfp_value;
            }
        }
    }

    nn::AccumulatorCache* const accum_cache = &node->cluster.accum_cache;

    /* Null Move Pruning -
    *  if we're doing so well even after not making a move, we must be winning here.
    *  So we can do beta cutoff.
    */
    if constexpr (!Root and AllowNullMove and !IsPv) {

        if (!node->check and 
            depth >= NullDepth and
            pos.getNonPawnMaterial() > 0 and
            beta < sc::KnownWin and
            beta > -sc::KnownWin) {

            const int32_t nmp_improving_scale = NullImprovingSinkMult * -node->improving / 256 + FixedPointMult;
            const int16_t nmp_margin = static_cast<int16_t>(nmp_improving_scale * NullMargin * depth / FixedPointMult);

            if (node->eval - nmp_margin >= beta) {    
                assert(parent_node->move != NullMove);
                
#if defined(LEAF_COLLECT_SEARCH_STATS)
                results.null_moves_cnt++;
#endif // LEAF_COLLECT_SEARCH_STATS

                pos.makeNull(node->state, accum_cache);

                AccumulatorCluster* const curr_cluster = &node->cluster;
                AccumulatorCluster* const next_cluster = curr_cluster->next_cluster;
                AccumulatorCluster* const prev_cluster = curr_cluster->prev_cluster;

                assert(curr_cluster->prev_cluster->next_cluster == curr_cluster);

                node->move = NullMove;
                next_cluster->prev_cluster = prev_cluster;
                
                const int nm_depth = getNullSearchDepth(node->eval, beta, depth);

                child_node->is_cut = !node->is_cut;

                sc::Score score = -nmSearch<NON_PV_NODE, !AllowNullMove>(pos, limits, results, game, child_node,
                                                                    -beta, -beta + 1, 
                                                                    nm_depth, 
                                                                    ply + 1);
                pos.unmakeNull(node->state);
                next_cluster->prev_cluster = curr_cluster;

                if (score <= -sc::MateBound) {
                    node->mate_thread = true;
                }

                const sc::Score nm_score = score;

                /* Unless Null Move Pruning is not handled properly in the endgame, 
                *  verification search is just needed to prevent Zugzwang.
                */

                if (score >= beta and
                    nm_depth >= NullVerifyDepth)
                {
                    const int verify_depth = getNullVerifyDepth(nm_depth);

                    score = nmSearch<NON_PV_NODE, !AllowNullMove>(pos, limits, results, game, node,
                                                             beta - 1, beta,
                                                             verify_depth,
                                                             ply);

#if defined(LEAF_COLLECT_SEARCH_STATS)
                    results.null_zungzwang_detected += score < beta;
#endif // LEAF_COLLECT_SEARCH_STATS
                }

                if (score >= beta) {                    
                    return score;
                }
            }
        }
    }

    uint64_t parent_hash_of_killer = ZHash::Undef;
    const uint64_t parent_hash = Root ? ZHash::Undef : parent_node->state.hash_key;
    Move32b killer = node->move_picker.getKillerMove(parent_hash_of_killer);
    
    // Assert we don't probe junk killer move
    if (Root or 
        killer.isNullMove() or 
        parent_hash_of_killer != parent_hash or
        !killer.isPseudoLegal(pos)) {
        killer = NullMove;
    }
        
    node->move_picker.clear();
    node->move_picker.setHashMove(tt_move);

    _STACK_PARAM_ATTRIBS const int32_t MaxMoveExtension = 1.f * FixedPointMult * MaxMoveExtensionRate / MaxMoveExtensionDiv;
    _STACK_PARAM_ATTRIBS const int32_t MoveCheckExtensionBase = 1.f * FixedPointMult * MoveCheckExtensionRate / MoveCheckExtensionDiv;
    _STACK_PARAM_ATTRIBS const int32_t MateThreadExtensionBase = 1.f * FixedPointMult * MateThreadFracExtensionRate / MateThreadFracExtensionDiv;
    _STACK_PARAM_ATTRIBS const int32_t SingularExtension = 1.f * FixedPointMult * SingularExtensionRate / SingularExtensionDiv;
    _STACK_PARAM_ATTRIBS const int32_t SingularBetaReduction = 1.f * FixedPointMult * SingularBetaExtensionRate / SingularBetaExtensionDiv;

    node->can_move       = false;
    node->score          = sc::Undef;
    node->move           = NullMove;
    node->best_move      = NullMove;
    node->best_score     = -sc::Infinity;
    node->moves_searched = 0;
    node->bound          = tt::TTBound::UPPERBOUND;
    node->move_score     = sc::Undef;

    for (node->move_index = 0; 
         node->move_picker.nextMoveWithPolicy<OrderPolicy, Root>(node, pos, node->move, node->move_score, ply);
         node->move_index++) 
    {
        /* Singular Move -
        *  return obvious move that is the only one in root
        */
        if constexpr (Root and OrderPolicy == mvo::ONCE_GEN_LEGAL) {
            if (!limits.analysis_mode and 
                node->move_picker.getTotalMoves<OrderPolicy>() == 1) {
                results.score_cp = sc::Undef;
                node->best_move = node->move;
                return sc::Undef;
            }
        }

        const uint64_t next_hash = pos.likelyZobristKeyAfterMove(node->move);
        _tt.prefetchBucket(next_hash);

        /* Futility Pruning -
        *  at shallow depths, skip moves that aren't like to rise alpha.
        */
        if constexpr (!Root and !IsPv) {
            if (!node->check and 
                !node->mate_thread and
                alpha < sc::MateBound)
            {
                /* Static Exchange Evaluation Pruning -
                *  prune bad moves accoring to SEE score.
                */
                if (depth <= SeePruneDepth and
                    node->move != tt_move and
                    node->move != killer and
                    node->eval + SeePruneMarginMult * depth < alpha and
                    pos.getNonPawnMaterial() > 0)
                {
                        if (node->move.isCapture() and
                            pos.badStaticExchangeEval(node->move, SeeCapturePruneThreshold * depth))
                        continue;
                    else if (!node->move.isCapture() and
                             node->move_score < SeeQuietScoreThreshold and
                             pos.badStaticExchangeEval(node->move, SeeQuietPruneThreshold * depth))
                        continue;
                }

                /* Move Count Based pruning -
                *  prune quiet moves that come last.
                */
                if (depth <= FutilityDepth and
                    node->moves_searched >= FutilityMoveCount and
                    node->move.isQuiet() and
                    !node->move.isQueenPromotion() and
                    pos.getNonPawnMaterial() > 0) 
                {
                    const int32_t futility_margin = FutilityDelta * depth * depth + 
                                                    node->move_score.quietCentered().value() * 
                                                    FutilityScoreMult / 8192;

                    if (node->eval + futility_margin < alpha) {
                        node->move_picker.skipQuiets();
                        continue;
                    }
                }
            }
        }

        if (!pos.make(node->move, accum_cache)) {
            pos.unmake(node->move, node->state);
            continue;
        }

        node->can_move = true;

        const bool gives_check = child_node->check = pos.isInCheck(!node->side2move);

        int32_t move_extension = 0;
        int32_t move_reduction = 0;

        /* Singular Move Extension -
        *  when we got some relatively strong move from TT,
        *  we try to search it with reduced depth.
        *  When we observe score below beta, we can assume that 
        *  reducing that move might be dangerous (horizon effect).
        *  If so, we try to include another extension.
        */
        if constexpr (!Root) {
            if (depth >= SingularDepth and
                !node->check and
                node->move == tt_move and
                !tt_move.isNullMove() and
                tt_entry.depth >= depth - SingularDepthMargin and
                tt_entry.bound == tt::TTBound::LOWERBOUND and
                tt_entry.score < sc::KnownWin and
                tt_entry.score > -sc::KnownWin) 
            {
                const int singular_depth = std::max<int>((SingularDepthMult * depth - SingularDepthBase) / 256, 1);
                const sc::Score singular_beta = std::max<int16_t>(-sc::MateBound.value() / 2, 
                                                                  static_cast<int>(tt_entry.score) - SingularBetaDepthMult * depth / 16);

                child_node->is_cut = !node->is_cut;

                const sc::Score score = -nmSearch<NON_PV_NODE, true>(pos, limits, results, game, child_node,
                                                                     -singular_beta, -singular_beta + 1,
                                                                     singular_depth,
                                                                     ply + 1);
                if (score < singular_beta) {
                    move_extension += SingularExtension;
                }
                else if (score >= beta and score < sc::KnownWin and score > -sc::KnownWin) {
                    pos.unmake(node->move, node->state);
                    const sc::Score reduced_score = (static_cast<int>(score) * singular_depth + static_cast<int>(beta)) 
                                                        / (singular_depth + 1);
                    return reduced_score;
                }
                else if (score >= singular_beta) {
                    move_extension -= SingularBetaReduction;
                }
            }
        }
        
        if (depth <= ExtensionDepth) {
            if (gives_check)
                move_extension += MoveCheckExtensionBase + node->improving * ImprovingExtensionRate / 256;
                
            if (node->mate_thread)
                move_extension += MateThreadExtensionBase + node->improving * ImprovingExtensionMateRate / 256;
        }

        move_extension = std::clamp<int32_t>(move_extension, 0, MaxMoveExtension);

        bool full_depth_search = !IsPv and !(node->moves_searched > 0);
        bool full_window_search = IsPv and !node->moves_searched;

        /* Dynamic Depth Late Move Reduction -
        *  consider float reduction based on contextual information
        *  about the move.
        */

        if (!full_depth_search and 
            !full_window_search and 
            depth >= LmrDepth and
            node->moves_searched >= LmrMoveCount) 
        {                
            const Piece::enumType pc = node->move.getPiece();

            if (node->move.isQuiet() and !node->move.isPromotion()) {
                move_reduction = (LmrBaseQuietReduction + 
                                  LmrLogQuietDepthMovesMult * std::log(depth) * std::log(node->moves_searched)) * 
                                  FixedPointMult;

                if constexpr (!IsPv) 
                    move_reduction += QuietNotPvNodeReduction * FixedPointMult;

                if (node->is_cut) 
                    move_reduction -= QuietCutNodeReduction * FixedPointMult;

                if (node->check) 
                    move_reduction -= QuietCheckReduction * FixedPointMult;

                if (pc == Piece::PAWN) 
                    move_reduction -= QuietPawnMoveReduction * FixedPointMult;

                if (!tt_move.isNullMove() and tt_move.isCapture())
                    move_reduction += QuietHashCapReduction * FixedPointMult;

                if (!killer.isNullMove() and node->move == killer) 
                    move_reduction -= QuietKillerMoveReduction * FixedPointMult;

                if (node->move_score.isValid()) 
                    move_reduction += mvo::MoveOrder::getQuietDepthReduction(node->move_score) * FixedPointMult;

                move_reduction -= static_cast<int64_t>(move_extension) * move_extension * 
                                    QuietExtensionReduction / FixedPointMult;
                move_reduction -= node->improving * QuietImprovingReductionRate;
                move_reduction /= QuietTotalReductionRate;
            }
            else {
                move_reduction = (LmrBaseCaptureReduction + 
                                  LmrLogCaptureDepthMovesMult * std::log(depth) * std::log(node->moves_searched)) * 
                                  FixedPointMult;

                if constexpr (!IsPv)
                    move_reduction += CaptureNotPvNodeReduction * FixedPointMult;

                if (node->is_cut)
                    move_reduction -= CaptureCutNodeReduction * FixedPointMult;

                if (node->check)
                    move_reduction -= CaptureCheckReduction * FixedPointMult;

                if (!tt_move.isNullMove() and tt_move.isCapture())
                    move_reduction += CaptureHashCapReduction * FixedPointMult;

                if (!killer.isNullMove() and node->move == killer)
                    move_reduction -= CaptureKillerMoveReduction * FixedPointMult;

                if (node->move_score.isValid() and !node->move.isPromotion())
                    move_reduction += mvo::MoveOrder::getCaptureDepthReduction(node->move_score) * FixedPointMult;

                move_reduction -= static_cast<int64_t>(move_extension) * move_extension * 
                                    CaptureExtensionReduction / FixedPointMult;
                move_reduction -= node->improving * CaptureImprovingReductionRate;
                move_reduction /= CaptureTotalReductionRate;
            }
        }
        
        const int reduction = std::clamp<int>((move_reduction + FixedPointMult / 2) / FixedPointMult, 0, depth - 1);
        const int reduct_depth = std::clamp(depth - reduction, 0, depth - 1);

        child_node->is_cut = !node->is_cut;

        /* Principal Variation Search -
        *  Search only fist move with full window.
        *  After that search, every other child node is expected Cut node and
        *  is being search with Null window.
        */
        if (!full_depth_search and !full_window_search) {

            /* Late Move Reduction -
            *  Try to reduce late moves, since they are statistically less interesting.
            *  Prove they fail low using Null window search with some reduction.
            *  If somehow they fail high, then re-search without reduction.
            */
            const bool do_lmr = (depth >= LmrDepth and
                                 node->moves_searched >= LmrMoveCount and
                                 reduct_depth < depth - 1);
        
            if (do_lmr) {
                child_node->is_cut = true;

                node->score = -nmSearch<NON_PV_NODE, true>(pos, limits, results, game, child_node,
                                                           -alpha - 1, -alpha,
                                                           reduct_depth,
                                                           ply + 1);

                child_node->is_cut = !(node->score > alpha);

#if defined(LEAF_COLLECT_SEARCH_STATS)
                results.reduced_search_cnt++;
                results.reduced_search_fail_high += !(node->score > alpha);
                results.reduced_search_fail_low += node->score > alpha;
                results.move_reduced_cnt[node->move_index]++;
                results.move_reduced_fail_high_cnt[node->move_index] += !(node->score > alpha);
                results.move_reduction_sum[node->move_index] += reduction;
#endif // LEAF_COLLECT_SEARCH_STATS
            } 
        
            full_depth_search = !do_lmr or node->score > alpha;
        }

        const int extension = (move_extension + FixedPointMult / 2) / FixedPointMult;
        const int ext_depth = std::min(depth - 1 + extension, std::max(MaxDepth - ply, 0));

        if (full_depth_search and !full_window_search) {
            node->score = -nmSearch<NON_PV_NODE, true>(pos, limits, results, game, child_node,
                                                       -alpha - 1, -alpha,
                                                       ext_depth,
                                                       ply + 1);

            child_node->is_cut = !(node->score > alpha);
            full_window_search = IsPv and node->score > alpha;
        }

        if (full_window_search) {
            node->score = -nmSearch<NmNodeType, true>(pos, limits, results, game, child_node,
                                                      -beta, -alpha, 
                                                      ext_depth,
                                                      ply + 1);
        }

        node->moves_searched++;

        pos.unmake(node->move, node->state);

        if (limits.isTimeLeft() and 
            results.anyNodesLeft(limits) and
            results.anyQuiesceNodesLeft(limits) and
            node->move.isLegalAfterMove() and 
            node->score > node->best_score) 
        {
            node->best_move = node->move;
            node->best_score = node->score;

            if (node->score > alpha) {
                if (node->score >= beta) {
                    node->bound = tt::TTBound::LOWERBOUND;

                    if (node->move.isQuiet() and !node->move.isQueenPromotion())  {
                        node->move_picker.updateQuietsHistory(node->best_move, node->side2move, depth, ply, node);
                        node->move_picker.setKillerMove(node->move, parent_hash);
                    }

#if defined(LEAF_COLLECT_SEARCH_STATS)
                    if (node->move == tt_move)
                        results.ttmove_cut_cnt++;

                    results.beta_cut_cnt++;
                    results.move_cut_cnt[node->move_index]++;
#endif // LEAF_COLLECT_SEARCH_STATS

                    break;
                }

                node->bound = tt::TTBound::EXACT;
                alpha = node->score;
                
                /* Collect Pv from the child */
                if constexpr (IsPv) {
                    node->pv_line[0].best_move = packedMove(node->best_move);
                    node->pv_line[0].score = node->best_score;

                    mem::memCopy(node->pv_line.data() + 1, 
                                 child_node->pv_line.data(), 
                                 child_node->pv_line_len * sizeof(PvInfo));

                    node->pv_line_len = child_node->pv_line_len + 1;
                }
            }
        }
        else if (!limits.isTimeLeft() or
                 !results.anyNodesLeft(limits) or
                 !results.anyQuiesceNodesLeft(limits))
        break;
    }

    if (!limits.isTimeLeft() or
        !results.anyNodesLeft(limits) or
        !results.anyQuiesceNodesLeft(limits)) 
    {
        if constexpr (Root) {
            if (node->best_move.isNullMove()) {
                node->best_move = node->move;
                node->pv_line[0].best_move = packedMove(node->best_move);
                node->pv_line_len = 1;
            }
        }

        return -sc::Undef;
    }
    
    // detect checkmate or stealmate
    if (!node->can_move) {
        node->bound = tt::TTBound::EXACT;
        node->best_score = node->check ? -sc::Score::getMateScore(ply)
                                       : getDrawScore(node);
    }

    if (!node->best_score.isMateScore() or tt_entry.isEmpty()) {
        const Move16b bestmove16b = packedMove(node->best_move);

        _tt.write(hash, 
                  depth, ply, 
                  node->bound, 
                  node->best_score, bestmove16b, node->eval);
    }

    if constexpr (!Root) {
        child_node->move_picker.setKillerMove(NullMove, 0);
    }

    if constexpr (Root) {
        results.score_cp = node->best_score;
    }

    return node->best_score;
}

template <enumNode QNodeType, bool Root>
sc::Score Search::qSearch(Position& pos, 
                          const SearchLimitsWrapper& limits, 
                          SearchResultsWrapper& results, 
                          NodeInfo* node, 
                          sc::Score alpha, sc::Score beta, 
                          int depth, int ply) 
{
    assert(0 <= ply and ply <= MaxSelDepth);
    assert(alpha < beta);

    static constexpr mvo::enumOrderPolicy QuiescentOrderPolicy = mvo::QUIESCENT;
    static constexpr bool               IsPv = QNodeType & PV_NODE; 

    assert(IsPv or alpha == beta - 1);

    node->side2move = pos.getTurn();

    if (isInsufficientMaterial(pos)) {
        return getDrawScore(node);
    }
    else if ((results.nodes_cnt & CheckNodeCount) == 0 and !limits.isTimeLeft()) {
        return -sc::Undef;
    }
    
    NodeInfo* const preroot = _tree_stack->getPreRootNode();

    if (!results.anyNodesLeft(limits) or
        !results.anyQuiesceNodesLeft(limits) or
        ply >= MaxSelDepth) {
        return evaluate<QNodeType>(pos, _tree_stack.get(), 
                                   node, preroot, 
                                   node->side2move, results);
    }

#if defined(_TT_PROBE_QSEARCH)
    tt::TTEntry tt_entry;
    tt_entry.eval = sc::Undef;
    tt_entry.move = NullMove;
    tt_entry.score = sc::Undef;

#if defined(LEAF_COLLECT_SEARCH_STATS)
    results.tt_probe_cnt++;
    results.qtt_probe_cnt++;
#endif // LEAF_COLLECT_SEARCH_STATS

    const uint64_t hash = pos.getZobristKey();
    const uint8_t probe_depth = std::max<uint8_t>(0, depth);

    const bool tt_hit = _tt.probe(tt_entry, hash, alpha, beta, probe_depth);
    const bool exact_hit = (!IsPv and tt_hit) or
                           ( IsPv and tt_hit and tt_entry.bound == tt::TTBound::EXACT);

    if (exact_hit) {
#if defined(LEAF_COLLECT_SEARCH_STATS)
        results.tt_cut_cnt++;
        results.qtt_cut_cnt++;
#endif // LEAF_COLLECT_SEARCH_STATS
        return tt_entry.score;
    }
#endif // _TT_PROBE_QSEARCH

    results.nodes_cnt++;
    results.seldepth = std::max(results.seldepth, ply + 1);
    results.qnodes_cnt++;

#if defined(_TT_PROBE_QSEARCH)
    node->eval = !tt_entry.eval.isValid() _LIKELY ? evaluate<QNodeType>(pos, _tree_stack.get(), 
                                                                        node, preroot, 
                                                                        node->side2move, results)
                                                  : tt_entry.eval;
#else 
    node->eval = evaluate<QNodeType>(pos, _tree_stack.get(), node, preroot, node->side2move, results);
#endif // _TT_PROBE_QSEARCH

    node->check = pos.isInCheck(!node->side2move);

    /* Delta Pruning -
    *  when no move has any chance to raise alpha
    *  then prune all of the branches.
    */
    if (node->eval + QMaterialDelta < alpha and 
        pos.getNonPawnMaterial() > 0 and
        !node->check) 
    {
        const sc::Score delta_value = (static_cast<int32_t>(alpha)      * (128 - QDeltaPruningEvalWeight) + 
                                       static_cast<int32_t>(node->eval) * QDeltaPruningEvalWeight) / 128;
        return delta_value;
    }
    
    /* Standing Pat Cutoff -
    *  when we're already above the beta, we can make a cutoff.
    */
    if (node->eval > alpha) {
        if (node->eval >= beta) {
            const sc::Score beta_cutoff_value = (static_cast<int32_t>(beta)       * (128 - QBetaCutoffEvalWeight) + 
                                                 static_cast<int32_t>(node->eval) * QBetaCutoffEvalWeight) / 128;
            return beta_cutoff_value;
        }

        alpha = node->eval;
    }

    node->move_picker.clear();

    Move32b tt_move = NullMove;

#if defined(_TT_PROBE_QSEARCH)
    const Move16b ttm16b = tt_entry.move;

    if ((!IsPv or tt_entry.bound != tt::TTBound::LOWERBOUND) and 
        (ttm16b.isPackedCapture(pos) or ttm16b.isQueenPromotion()))
    {
#if defined(LEAF_COLLECT_SEARCH_STATS)
        results.qttmove_probe_cnt++;
#endif // LEAF_COLLECT_SEARCH_STATS

        const Move32b ttm32b = unpackedMove(pos, tt_entry.move);
        tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b 
                                            : NullMove;
        node->move_picker.setHashMove(tt_move);
    }
#endif // _TT_PROBE_QSEARCH

    NodeInfo* const child_node = node + 1;
    assert(child_node - preroot < MaxSelDepth);

    nn::AccumulatorCache* const accum_cache = &node->cluster.accum_cache;

    node->moves_searched = 0;
    node->state          = pos.getReversibleState();
    node->best_move      = NullMove;
    node->move           = NullMove;
    node->score          = sc::Undef;
    node->best_score     = -sc::Infinity;

    mvo::SMoveScore move_score = sc::Undef;

    for (node->move_index = 0;
         node->move_picker.nextMoveWithPolicy<QuiescentOrderPolicy, Root>(node, pos, node->move, move_score, ply);
         node->move_index++) 
    {
        
#if defined(_TT_PREFETCH_QSEARCH)    
        const uint64_t next_hash = pos.likelyZobristKeyAfterMove(node->move);
        _tt.prefetchBucket(next_hash);
#endif // _TT_PREFETCH_QSEARCH

        /* Static Exchange Evaluation Pruning -
        *  ignore losing captures, as they aren't likely to rise alpha anyway.
        */
        if constexpr (!IsPv) {
            if (node->move.isCapture() and
                !node->move.isPromotion() and
                !node->check and
                pos.badStaticExchangeEval(node->move, QSeePruningThreshold))
                continue;
        }

        if (!pos.make(node->move, accum_cache)) {
            pos.unmake(node->move, node->state);
            continue;
        }

        node->score = -qSearch<QNodeType>(pos, limits, results, child_node,
                                          -beta, -alpha,
                                          depth - 1,
                                          ply + 1);

        node->moves_searched++;

        pos.unmake(node->move, node->state);

        if (limits.isTimeLeft() and 
            results.anyNodesLeft(limits) and
            results.anyQuiesceNodesLeft(limits) and
            node->move.isLegalAfterMove() and 
            node->score > alpha) 
        {
            node->best_move = node->move;
            node->best_score = node->score;

            if (node->score >= beta) {
#if defined(LEAF_COLLECT_SEARCH_STATS)
                if (node->move == tt_move) {
                    results.ttmove_cut_cnt++;
                    results.qttmove_cut_cnt++;
                }

                results.beta_cut_cnt++;
                results.qbeta_cut_cnt++;

                results.move_cut_cnt[node->move_index]++;
#endif // LEAF_COLLECT_SEARCH_STATS
                return node->best_score;
            }
            
            alpha = node->score;
        }
        else if (!limits.isTimeLeft() or
                 !results.anyNodesLeft(limits) or
                 !results.anyQuiesceNodesLeft(limits))
        {
            return -sc::Undef;
        }
    }

    return node->best_score != -sc::Infinity ? node->best_score 
                                                : alpha;
}

_FORCEINLINE sc::Score Search::getDrawScore(const NodeInfo* node) const {
    return applyContempt(sc::Draw, node);
}

_FORCEINLINE sc::Score Search::getTablebaseScore(SyzygyTablebase::TbWdlInfo wdl, 
                                             const Position& pos, 
                                             const NodeInfo* node,
                                             int ply) const 
{
    static const auto get_win_tb_score = [](const Position& pos, int ply) -> sc::Score  _LAMBDA_FORCEINLINE {
        const int16_t pc_cnt_diff = std::abs(pos.getOwnPieces().popCount() - pos.getOppositePieces().popCount());
        const int16_t result = TablebaseWinScore - ply - TablebasePieceDiffMult * (15 - pc_cnt_diff);
        return static_cast<sc::Score>(result);
    };

    switch (wdl) {
    case SyzygyTablebase::WDL_WIN: {
        const sc::Score tb_score = get_win_tb_score(pos, ply);
        assert(tb_score > sc::Win);
        return tb_score;
    }
    case SyzygyTablebase::WDL_LOSS: {
        const sc::Score tb_score = -get_win_tb_score(pos, ply);
        assert(tb_score < -sc::Win);
        return -tb_score;
    }
    case SyzygyTablebase::WDL_DRAW: {
        return getDrawScore(node);
    }
    default: 
        break;
    }

    return sc::Undef;
}

_FORCEINLINE bool Search::isTablebaseScore(sc::Score score) const {
    static constexpr int16_t TablebaseLowestWinScore = TablebaseScoreScale 
                                                        * (TablebaseWinScore 
                                                          - MaxSelDepth 
                                                          - 15 * TablebasePieceDiffMult) 
                                                        / 16;
    return score.isValid() and abs<sc::Score::value_type>(
        static_cast<sc::Score::value_type>(score)) >= TablebaseLowestWinScore;
}

_FORCEINLINE sc::Score Search::applyContempt(sc::Score score, const NodeInfo* node) const {
    const NodeInfo* const root = _tree_stack->getRootNode();
    assert(_contempt.isValid());
    return root->side2move == node->side2move ? score - _contempt
                                              : score;
}

template <enumNode NodeType>
_INLINE sc::Score Search::evaluate(const Position& pos,
                                   TreeStack* _tree_stack,
                                   NodeInfo* node,
                                   NodeInfo* preroot,
                                   enumColor side2move, 
                                   _MAYBE_UNUSED SearchResultsWrapper& results)
{

#if defined(LEAF_COLLECT_SEARCH_STATS)
    if constexpr (NodeType & QUIESCE_NODE)
        results.qeval_cnt++;
        
    else
        results.nmeval_cnt++;
#endif

    sc::Score pawnless_eg_eval = sc::Undef;

    if (!SyzygyTablebase::get().isLoaded()) {
        pawnless_eg_eval = pos.getPawns().isEmpty() ? hce::StaticEval::evaluatePawnlessEndgame(pos)
                                                    : sc::Undef;

        if (pawnless_eg_eval == sc::Draw)
            return pawnless_eg_eval;
    }

    AccumulatorCluster* curr_accum_cluster = &node->cluster;
    const AccumulatorCluster* prev_accum_cluster = curr_accum_cluster->prev_cluster;

    if (prev_accum_cluster->accum_cache.isDirty()) {
        AccumulatorCluster* clean_accum_cluster = _tree_stack->getCleanAccumulatorCluster(curr_accum_cluster, preroot);
        _tree_stack->updateDirtyAccumulators(clean_accum_cluster, curr_accum_cluster);
    }

    assert(prev_accum_cluster->accum_cache.isClean());

    const nn::Accumulator& prev_accum = prev_accum_cluster->accum_cache.accum;

#if defined(_VERIFY_NN)
    ASSERT(nn::Accumulator::verify(prev_accum, pos), "Accumulator verification failed");
#endif

    const sc::Score eval = nn::NEval::evaluate(nn::GlobPackedNetwork, prev_accum, side2move);
    const int64_t unscaled_eval = NNEvalScale * 4096 * static_cast<int64_t>(eval);
    int16_t scaled_eval = (unscaled_eval + FixedPointMult / 2) / FixedPointMult;

    // Assert we won't overflow into special winning scores
    assert(abs<int16_t>(scaled_eval) < sc::Win.value());

    if (!SyzygyTablebase::get().isLoaded() and pawnless_eg_eval.isValid()) {
        switch (pawnless_eg_eval.value()) {
        case sc::Win.value():
        case -sc::Win.value():
            scaled_eval = pawnless_eg_eval.value() + 
                          std::clamp<sc::Score::value_type>(scaled_eval, sc::KnownWin.value() - sc::Win.value() - 1, 0);
            break;
        case sc::KnownWin.value():
        case -sc::KnownWin.value(): 
            scaled_eval = pawnless_eg_eval.value() + 
                          std::clamp<sc::Score::value_type>(scaled_eval, sc::MateBound.value() - sc::KnownWin.value() - 1, 0);
            break;
        default: 
            assert("Invalid endgame score");
            break;
        }
    }
 
    const uint8_t halfmoves_left = 100 - pos.getHalfmoveClock();
    const uint8_t halfmoves_left_limit = pos.getPiecesCount() < 6 ? EvalEgHalfMovesEvalLimit : EvalHalfMovesEvalLimit;
    const int32_t clock_mult = std::min<int32_t>(halfmoves_left, halfmoves_left_limit);
    sc::Score::value_type result = static_cast<sc::Score::value_type>(static_cast<int32_t>(scaled_eval) * clock_mult / halfmoves_left_limit);

    return result;
}

_FORCEINLINE sc::Score Search::correctedEvalScore(sc::Score eval, sc::Score score) {
    assert(eval.isValid());

    if (!score.isValid())
        return eval;

    const sc::Score tt_eval_diff = score - eval;
    return eval + tt_eval_diff / TTEvalCorrRate;
}

_FORCEINLINE int16_t Search::getRfpQuietHistPenalty(NodeInfo* parent_node) {
    const Move32b prev_move = parent_node->move;

    if (!prev_move.isNullMove() and prev_move.isQuiet() and !prev_move.isQueenPromotion()) {
        const int unorm_score = parent_node->move_score.value();
        return unorm_score * RfpQuietPenaltyMult / 8192;
    }

    return 0;
}

_FORCEINLINE int Search::getNullSearchDepth(sc::Score eval, sc::Score beta, int depth) {
    const float diff_reduction = std::min(1.31f, static_cast<float>(eval - beta) / NullDiffScale);
    const float diff_scale = 1.5f + 1.f / (diff_reduction - 2.f);
    // Do not return same depth, we could stuck in a loop
    assert(NullDepthMult * depth / 256 < depth);
    return std::max<int>(std::lroundf(NullDepthMult * diff_scale * depth / 256), 1);
}

_FORCEINLINE int Search::getNullVerifyDepth(int nm_depth) {
    return std::max(std::lroundf(static_cast<float>(NullVerifyDepthMult) * nm_depth / 64), 
                    1l);
}

void Search::refreshPVinTT(const Position& pos, 
                           const std::array<PvInfo, MaxSelDepth>& root_pv_line, 
                           uint16_t pv_len,
                           SearchResultsWrapper& results) 
{
    if (results.depth <= 1)
        return;

    Position cpy_pos = pos;

    for (uint16_t i = 0; 
         i < std::min(static_cast<uint16_t>(results.depth), pv_len); 
         i++) 
    {
        const Move16b pv_move = root_pv_line[i].best_move;
        const sc::Score score = root_pv_line[i].score;
        const uint64_t key = cpy_pos.getZobristKey();
        const int depth = results.depth - i;

        assert(depth > 0);
        assert(!pv_move.isNullMove());

        tt::TTEntry tt_entry;
        tt_entry.move = NullMove;

        const bool tt_hit = _tt.probe(tt_entry, 
                                      key,
                                      -sc::MateBound, +sc::MateBound, 
                                      depth);

        if (!tt_hit or pv_move != tt_entry.move) {
            _tt.write(key,
                      static_cast<uint8_t>(depth), static_cast<uint8_t>(i),
                      tt::TTBound::EXACT, 
                      score, pv_move, sc::Undef);
        }
        
        Move32b pv_unpack = unpackedMove(cpy_pos, pv_move);
        _declUnused(cpy_pos.make(pv_unpack));
    }

    if (results.best_move.isNullMove())
        return;

    // Assert we got PV-move at root - got it directly from previous best move

    const uint64_t key = pos.getZobristKey();
    const Move16b  root_best_move = packedMove(results.best_move);
    const int depth = results.depth;
    const sc::Score score = results.score_cp;

    if (!pv_len) {
        tt::TTEntry tt_entry;
        tt_entry.move = NullMove;

        const bool tt_hit = _tt.probe(tt_entry,
                                      key,
                                      -sc::MateBound, +sc::MateBound,
                                      depth);

        if (!tt_hit or root_best_move != tt_entry.move) {
            _tt.write(key,
                      depth, 0,
                      tt::TTBound::EXACT,
                      score, root_best_move, sc::Undef);
        }
    }

#if defined(DEBUG)

    // Check if PV-move for root node is actually there
    TTEntry tt_entry;
    tt_entry.move = NullMove;

    _tt.probe(tt_entry,
              key,
              -sc::MateBound, +sc::MateBound,
              depth);

    ASSERT_NOLOG(!tt_entry.move.isNullMove());

#endif
}

template <bool IsPv>
bool Search::isRepetitionCycle(const Position& pos,
                               const FullInfoRecord& game,
                               const NodeInfo* node,
                               int ply,
                               SearchResultsWrapper& results)
{
#if defined(LEAF_COLLECT_SEARCH_STATS)
    results.rep_call_cnt++;
#else
    _declUnused(results);
#endif

    const uint64_t curr_hashkey = pos.getZobristKey();
    const NodeInfo* prev_node = node;

    for (int p = ply - 1; 
         p >= 0 and p >= ply - pos.getHalfmoveClock(); 
         p -= 2) 
    {
        prev_node--;

        if (prev_node->move.isNullMove() or prev_node->move.isIrreversible())
            return false;

        prev_node--;
        
        if (prev_node->move.isNullMove() or prev_node->move.isIrreversible())
            return false;

        assert(prev_node->side2move == node->side2move);

        if (curr_hashkey == prev_node->state.hash_key)
            return true;
    }

    if (ply >= pos.getHalfmoveClock())
        return false;

    const int game_rep_depth = 50 - ply;
    const int curr_halfclock = static_cast<int>(game.getMoveCount());

    for (int halfclock = curr_halfclock - 1;
         halfclock >= 0 and curr_halfclock - halfclock <= game_rep_depth;
         halfclock--) 
    {
        const Move32b move = game.getPrevMove(halfclock);

        if (move.isIrreversible())
            return false;

        if (curr_hashkey == game.getPrevKey(halfclock))
            return true;
    }
    
    return false;
}

/*
* Upcoming repetition detection based on a paper:
* http://web.archive.org/web/20201107002606/https://marcelk.net/2013-04-06/paper/upcoming-rep-v2.pdf
*/

bool Search::canRepetitionDraw(const Position& pos,
                               const NodeInfo* node,
                               int ply)
{
    if (pos.getHalfmoveClock() < 3)
        return false;

    const uint64_t curr_hash = pos.getZobristKey();
    const NodeInfo* prev_node = node - 1;

    if (prev_node->move.isNullMove() or prev_node->move.isIrreversible())
        return false;

    size_t idx = static_cast<size_t>(-1);

    for (int p = ply - 1; 
         p >= 2 and p >= ply - pos.getHalfmoveClock() + 2; 
         p -= 2) 
    {
        prev_node--;
        
        if (prev_node->move.isNullMove() or prev_node->move.isIrreversible())
            break;
        
        prev_node--;

        if (prev_node->move.isNullMove() or prev_node->move.isIrreversible())
            break;

        assert(prev_node->side2move != node->side2move);
        assert(prev_node->state.hash_key);

        if (const uint32_t move_hash = static_cast<uint32_t>(prev_node->state.hash_key ^ curr_hash);
            (idx = CuckooTables::cuckooIndex1(move_hash), _cuckoo_tables.getMoveHash(idx) == move_hash) or
            (idx = CuckooTables::cuckooIndex2(move_hash), _cuckoo_tables.getMoveHash(idx) == move_hash)) {

            // simplified verification for obtained cuckoo move

            const Move16b move16b = _cuckoo_tables.getMove16b(idx);

            const Square org = move16b.getOrigin();
            const Square dst = move16b.getTarget();

            const BitBoard occupied = pos.getOccupied();
            
            if (!(occupied & (BitBoard(org) | BitBoard(dst))))
                continue;
                
            if (!(onlyBetween(org, dst) & occupied) or move16b.isKnight())
                return true;
        }
    }

    return false;
}

bool Search::isInsufficientMaterial(const Position& pos) {
    if (pos.getPawns() or pos.getQueens())
        return false;

    const int piece_cnt = pos.getPiecesCount();

    // K vs K
    if (piece_cnt == 2)
        return true;

    // K + B vs K
    if (piece_cnt == 3 and
        pos.getBishops() /* .popCount() == 1 */)
        return true;

    // K + N vs K
    if (piece_cnt == 3 and
        pos.getKnights() /* .popCount() == 1 */)
        return true;

    // K + BB... vs K + BB... with same color bishops

    const int white_bishop_cnt = pos.getBishopsBySide(WHITE).popCount();
    const int black_bishop_cnt = pos.getBishopsBySide(BLACK).popCount();

    if (const int total_bishop_cnt = white_bishop_cnt + black_bishop_cnt;
        piece_cnt - 2 == total_bishop_cnt and 
        white_bishop_cnt > 0 and 
        black_bishop_cnt > 0) 
    {
        const BitBoard bishops = pos.getBishops();
        const BitBoard white_square_bishops = bishops & BitBoard::WhiteSquares;

        return white_square_bishops == bishops or white_square_bishops.isEmpty();
    }

    return false;
}

template Move32b Search::findBestMove<SEARCH_FULL_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::findBestMove<SEARCH_SHORT_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::findBestMove<SEARCH_ONLY_BM_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::findBestMove<SEARCH_NO_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::findBestMove<SEARCH_FULL_INFO>(Position&, const FullInfoRecord&, SearchLimits, SearchResults&);
template Move32b Search::findBestMove<SEARCH_SHORT_INFO>(Position&, const FullInfoRecord&, SearchLimits, SearchResults&);
template Move32b Search::findBestMove<SEARCH_ONLY_BM_INFO>(Position&, const FullInfoRecord&, SearchLimits, SearchResults&);
template Move32b Search::findBestMove<SEARCH_NO_INFO>(Position&, const FullInfoRecord&, SearchLimits, SearchResults&);

} // namespace search
