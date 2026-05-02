#include "Tablebase.hpp"

#if defined(_USE_SYZYGY_TB)
#define TB_NO_THREADS
#include "vendor/tbprobe.h"
#endif

SyzygyTablebase& SyzygyTablebase::get() {
    static SyzygyTablebase SyzygyTable;
    return SyzygyTable;
}

bool SyzygyTablebase::loadSyzygyFile(const std::string& fp) {
#if defined(_USE_SYZYGY_TB)
    if (_initialized) {
        std::cout << "Syzygy tablebase already intialized" << std::endl;
        return false;
    }
    else if (!std::filesystem::exists(fp)) {
        std::cout << "Invalid Syzygy path - directory not found" << std::endl;
        return false;
    }

    const bool status = tb_init(fp.c_str());

    if (!status) {
        std::cout << "Failed to initialize Syzygy tablebase" << std::endl;
        return false;
    }

    _initialized = true;
    return true;
#else // !defined(_USE_SYZYGY_TB)
    _initialized = false;
    return false;
#endif // _USE_SYZYGY_TB
}

bool SyzygyTablebase::isReady() const {
    return _initialized;
}

bool SyzygyTablebase::probeWdl(const Position& pos, TbWdlInfo& wdl) {  
    if (!_initialized)
        return false;

    const uint pccnt = pos.getPiecesCount();

    if (pccnt > TB_LARGEST)
        return false;

    uint tb_castling = 0;
    if (pos.getCastlingByColor(WHITE).isShortPossible()) 
        tb_castling |= TB_CASTLING_K;
    if (pos.getCastlingByColor(WHITE).isLongPossible())
        tb_castling |= TB_CASTLING_Q;
    if (pos.getCastlingByColor(BLACK).isShortPossible())
        tb_castling |= TB_CASTLING_k;
    if (pos.getCastlingByColor(BLACK).isLongPossible())
        tb_castling |= TB_CASTLING_q;

    const uint tb_ep = pos.getEnPassantSq().isNull() ? 0 : static_cast<Square::uint_t>(pos.getEnPassantSq());

    // One of [TB_LOSS, TB_BLESSED_LOSS, TB_DRAW, TB_CURSED_WIN, TB_WIN, TB_RESULT_FAILED]
    const uint tb_wdl = tb_probe_wdl(pos.getWhites(),
                                     pos.getBlacks(),
                                     pos.getKings(),
                                     pos.getQueens(),
                                     pos.getRooks(),
                                     pos.getBishops(),
                                     pos.getKnights(),
                                     pos.getPawns(),
                                     pos.getHalfmoveClock(),
                                     tb_castling,
                                     tb_ep,
                                     pos.getTurn() == WHITE);

    assert(TB_LOSS == WDL_LOSS and
           TB_BLESSED_LOSS == WDL_MAYBE_LOSS and
           TB_DRAW == WDL_DRAW and
           TB_CURSED_WIN == WDL_MAYBE_WIN and
           TB_WIN == WDL_WIN);

    if (tb_wdl == TB_LOSS or 
        tb_wdl == TB_BLESSED_LOSS or
        tb_wdl == TB_DRAW or
        tb_wdl == TB_CURSED_WIN or
        tb_wdl == TB_WIN)
        wdl = static_cast<TbWdlInfo>(tb_wdl);
    else wdl = WDL_INVALID;

    return wdl != WDL_INVALID;
}

bool SyzygyTablebase::probeDtz(const Position& pos, 
                               TbWdlInfo& wdl, 
                               uint& dtz, 
                               Move16b& move) {
    if (!_initialized)
        return false;

    const uint pccnt = pos.getPiecesCount();

    if (pccnt > TB_LARGEST)
        return false;

    uint tb_castling = 0;
    if (pos.getCastlingByColor(WHITE).isShortPossible()) 
        tb_castling |= TB_CASTLING_K;
    if (pos.getCastlingByColor(WHITE).isLongPossible())
        tb_castling |= TB_CASTLING_Q;
    if (pos.getCastlingByColor(BLACK).isShortPossible())
        tb_castling |= TB_CASTLING_k;
    if (pos.getCastlingByColor(BLACK).isLongPossible())
        tb_castling |= TB_CASTLING_q;

    const uint tb_ep = pos.getEnPassantSq().isNull() ? 0 : static_cast<Square::uint_t>(pos.getEnPassantSq());

    const uint tb_res = tb_probe_root(pos.getWhites(),
                                      pos.getBlacks(),
                                      pos.getKings(),
                                      pos.getQueens(),
                                      pos.getRooks(),
                                      pos.getBishops(),
                                      pos.getKnights(),
                                      pos.getPawns(),
                                      pos.getHalfmoveClock(),
                                      tb_castling,
                                      tb_ep,
                                      pos.getTurn() == WHITE,
                                      nullptr);

    if (tb_res == TB_RESULT_FAILED) {
        wdl = WDL_INVALID;
        dtz = 0;
        move = Move32b::Null;
        return false;
    }

    const uint tb_from = TB_GET_FROM(tb_res);
    const uint tb_to = TB_GET_TO(tb_res);
    const uint tb_promo = TB_GET_PROMOTES(tb_res);

    const Square from = static_cast<Square>(tb_from);
    const Square to = static_cast<Square>(tb_to);

    if (tb_promo != TB_PROMOTES_NONE) {
        Piece::enumType promo_t = Piece::NONE;

        switch (tb_promo) {
        case TB_PROMOTES_QUEEN: 
            promo_t = Piece::QUEEN;
            break;
        case TB_PROMOTES_ROOK:
            promo_t = Piece::ROOK;
            break;
        case TB_PROMOTES_BISHOP:
            promo_t = Piece::BISHOP;
            break;
        case TB_PROMOTES_KNIGHT:
            promo_t = Piece::KNIGHT;
            break;
        default: break;
        }

        move = Move16b::makePackedPromo(from, to, promo_t);
    } 
    else {
        move = Move16b::makePackedSimple(from, to);
    }

    dtz = TB_GET_DTZ(tb_res);
    const uint tb_wdl = TB_GET_WDL(tb_res);

    assert(TB_LOSS == WDL_LOSS and
           TB_BLESSED_LOSS == WDL_MAYBE_LOSS and
           TB_DRAW == WDL_DRAW and
           TB_CURSED_WIN == WDL_MAYBE_WIN and
           TB_WIN == WDL_WIN);

    if (tb_wdl == TB_LOSS or 
        tb_wdl == TB_BLESSED_LOSS or 
        tb_wdl == TB_DRAW or 
        tb_wdl == TB_CURSED_WIN or
        tb_wdl == TB_WIN)
        wdl = static_cast<TbWdlInfo>(tb_wdl);
    else wdl = WDL_INVALID;

    return true;
}

SyzygyTablebase::~SyzygyTablebase() {
    if (_initialized) 
        tb_free();
}
