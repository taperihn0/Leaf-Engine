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

#include "StaticEval.hpp"
#include "Search.hpp"
#include "Position.hpp"

sc::Score StaticEval::evaluatePawnlessEndgame(const Position& pos) {

    // Pawnless endgames:
    // https://en.wikipedia.org/wiki/Pawnless_chess_endgame

    const int piece_cnt = pos.getPiecesCount();
    const enumColor s2m = pos.getTurn();

    if (piece_cnt == 3) {

        // K + R vs K
        if (pos.getRooks().isSingleBit())
            return pos.getRooksBySide(s2m) ? sc::KnownWin
                                           : -sc::KnownWin;

        // K + Q vs K
        if (pos.getQueens().isSingleBit())
            return pos.getQueensBySide(s2m) ? sc::KnownWin
                                            : -sc::KnownWin;

    }
    else if (piece_cnt == 4) {

        const int white_bishops = pos.getBishopsBySide(WHITE).popCount();
        const int black_bishops = pos.getBishopsBySide(BLACK).popCount();

        // K + BB vs K
        if (white_bishops == 2 or black_bishops == 2)
            return pos.getBishopsBySide(s2m) ? sc::KnownWin
                                             : -sc::KnownWin;

        const int white_queens  = pos.getQueensBySide(WHITE).popCount();
        const int black_queens  = pos.getQueensBySide(BLACK).popCount();

        // K + Q vs K + B
        if ((white_queens == 1 and black_bishops == 1) or
            (black_queens == 1 and white_bishops == 1))
            return pos.getQueensBySide(s2m) ? sc::KnownWin
                                            : -sc::KnownWin;

        const int white_knights = pos.getKnightsBySide(WHITE).popCount();
        const int black_knights = pos.getKnightsBySide(BLACK).popCount();

        // K + NN vs K
        if (white_knights == 2 or black_knights == 2)
            return sc::Draw;

        // K + Q vs K + N
        if ((white_queens == 1 and black_knights == 1) or
            (black_queens == 1 and white_knights == 1))
            return pos.getQueensBySide(s2m) ? sc::KnownWin
                                            : -sc::KnownWin;

        // K + BN vs K
        if (white_bishops == 1 and white_knights == 1)
            return pos.getBishopsBySide(s2m) ? sc::KnownWin
                                             : -sc::KnownWin;

        if (black_bishops == 1 and black_knights == 1)
            return pos.getBishopsBySide(s2m) ? sc::KnownWin
                                             : -sc::KnownWin;

        const int white_rooks = pos.getRooksBySide(WHITE).popCount();
        const int black_rooks = pos.getRooksBySide(BLACK).popCount();

        // K + Q vs K + R
        if ((white_queens == 1 and black_rooks == 1) or
            (black_queens == 1 and white_rooks == 1))
            return pos.getQueensBySide(s2m) ? sc::Win
                                            : -sc::Win;

        // K + R vs K + R                                    
        if (piece_cnt == 4 and
            white_rooks == 1 and
            black_rooks == 1)
            return sc::Draw;

    }
    else if (piece_cnt == 5) {

        const int s2m_rooks  = pos.getRooksBySide(s2m).popCount();
        const int ns2m_rooks = pos.getRooksBySide(!s2m).popCount();

        // K + RR vs K + R
        if (s2m_rooks == 2 and ns2m_rooks == 1)
            return sc::KnownWin;

        if (ns2m_rooks == 2 and s2m_rooks == 1)
            return -sc::KnownWin;

        // K + NR vs K + R
        if (s2m_rooks == 1 and
            ns2m_rooks == 1 and
            pos.getKnights().isSingleBit())
            return sc::Draw;

    }
    else if (piece_cnt == 6) {

        const int white_knights = pos.getKnightsBySide(WHITE).popCount();
        const int black_knights = pos.getKnightsBySide(BLACK).popCount();

        const int s2m_rooks  = pos.getRooksBySide(s2m).popCount();
        const int ns2m_rooks = pos.getRooksBySide(!s2m).popCount();

        // K + RR vs K + BB, K + NN, K + NB
        if (s2m_rooks == 2 and
            (s2m ? white_knights : black_knights) == 2)
            return sc::Win;

        if (ns2m_rooks == 2 and
            (s2m ? black_knights : white_knights) == 2)
            return -sc::Win;

        const int white_rooks = pos.getRooksBySide(WHITE).popCount();
        const int black_rooks = pos.getRooksBySide(BLACK).popCount();

        const int white_bishops = pos.getBishopsBySide(WHITE).popCount();
        const int black_bishops = pos.getBishopsBySide(BLACK).popCount();

        // K + RB vs K + NN
        if ((white_rooks == 1 and
             white_bishops == 1 and
             black_knights == 2) or
            (black_rooks == 1 and
             black_bishops == 1 and
             white_knights == 2))
            return pos.getRooksBySide(s2m) ? sc::Win
                                           : -sc::Win;

    }

    return sc::Undef;
}

/*
*  PeSTO evaluation tables provided by Chess Programming Wiki:
*  https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function 
*/

MultiArray<int16_t, 64> StaticEval::_mg_pawn_tables = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

MultiArray<int16_t, 64> StaticEval::_mg_knight_tables = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

MultiArray<int16_t, 64> StaticEval::_mg_bishop_tables = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

MultiArray<int16_t, 64> StaticEval::_mg_rook_tables = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

MultiArray<int16_t, 64> StaticEval::_mg_queen_tables = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};

MultiArray<int16_t, 64> StaticEval::_mg_king_tables = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};

sc::Score StaticEval::matEval(const Position& pos) {
    const enumColor turn = pos.getTurn();
    return pos.getOnBoardMaterial(turn) - pos.getOnBoardMaterial(!turn);
}

_INLINE sc::Score StaticEval::pawnsStaticEval(const Position& pos, enumColor side) {
    BitBoard pawns = pos.getPawnsBySide(side);
    int16_t res = 0;

    while (pawns) {
        const Square sq = pawns.dropForward();
        res += _mg_pawn_tables[sqBlackPerspectiveFlip(sq, side)];
    }

    return sc::Score(res);
}

_INLINE sc::Score StaticEval::knightsStaticEval(const Position& pos, enumColor side) {
    BitBoard knights = pos.getKnightsBySide(side);
    int16_t res = 0;

    while (knights) {
        const Square sq = knights.dropForward();
        res += _mg_knight_tables[sqBlackPerspectiveFlip(sq, side)];
    }

    return sc::Score(res);
}

_INLINE sc::Score StaticEval::bishopsStaticEval(const Position& pos, enumColor side) {
    BitBoard bishops = pos.getBishopsBySide(side);
    int16_t res = 0;

    while (bishops) {
        const Square sq = bishops.dropForward();
        res += _mg_bishop_tables[sqBlackPerspectiveFlip(sq, side)];
    }

    return sc::Score(res);
}

_INLINE sc::Score StaticEval::rooksStaticEval(const Position& pos, enumColor side) {
    BitBoard rooks = pos.getRooksBySide(side);
    int16_t res = 0;

    while (rooks) {
        const Square sq = rooks.dropForward();
        res += _mg_rook_tables[sqBlackPerspectiveFlip(sq, side)];
    }

    return sc::Score(res);
}

_INLINE sc::Score StaticEval::queensStaticEval(const Position& pos, enumColor side) {
    BitBoard queens = pos.getQueensBySide(side);
    int16_t res = 0;

    while (queens) {
        const Square sq = queens.dropForward();
        res += _mg_queen_tables[sqBlackPerspectiveFlip(sq, side)];
    }

    return res;
}

_INLINE sc::Score StaticEval::kingsStaticEval(const Position& pos, enumColor side) {
    const Square ksq = pos.getKingSquareBySide(side);
    return sc::Score(_mg_king_tables[sqBlackPerspectiveFlip(ksq, side)]);
}

sc::Score StaticEval::staticEval(const Position& pos) {
    const enumColor side = pos.getTurn();

    return matEval(pos)
        + pawnsStaticEval(pos, side) - pawnsStaticEval(pos, !side)
        + knightsStaticEval(pos, side) - knightsStaticEval(pos, !side)
        + bishopsStaticEval(pos, side) - bishopsStaticEval(pos, !side)
        + rooksStaticEval(pos, side) - rooksStaticEval(pos, !side)
        + queensStaticEval(pos, side) - queensStaticEval(pos, !side)
        + kingsStaticEval(pos, side) - kingsStaticEval(pos, !side);
}
