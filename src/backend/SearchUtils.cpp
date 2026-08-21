#include "SearchUtils.hpp"

namespace engine
{
    
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

SearchStack::SearchStack()
    : _stack(mem::makeAlignedUnique<NodeInfo>(_Count, CachelineSize))
{
    ASSERT(_stack != nullptr, "Failed to allocate memory");
}

void SearchStack::clear(mem::AlignedSharedPtr<mvo::HistoryTablesCluster> history_buffer) {
    ASSERT_NO_LOG(history_buffer);

    mvo::MoveOrder::setHistoryBuffer(history_buffer);

    for (int i = 0; i < static_cast<int>(_Count); i++) {
        NodeInfo& node = _stack.get()[i];
        node.clear();
        node.cluster.prev_cluster = i - 1 >= 0 ? &_stack.get()[i - 1].cluster : nullptr;
        node.cluster.next_cluster = i + 1 < static_cast<int>(_Count) ? &_stack.get()[i + 1].cluster : nullptr;
    }
}

AccumulatorCluster* SearchStack::getCleanAccumulatorCluster(AccumulatorCluster* const accum_cluster,
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

void SearchStack::updateDirtyAccumulators(AccumulatorCluster* const clean_accum_cluster,
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

} // namespace engine
