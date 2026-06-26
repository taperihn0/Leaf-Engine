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

#include "Move.hpp"
#include "MoveGen.hpp"
#include "Position.hpp"

/* TODO: move validity restricted checking */

Move32b createMove(const Position& pos, Square origin, Square target, Piece::enumType piece, 
                   bool capture, bool ep_capture, bool promotion, 
                   bool short_castle, bool long_castle, Piece::enumType promo_piece) 
{

    ASSERT(piece != Piece::NONE, "Invalid move");
    ASSERT(pos.getOwnPieces().isEmptySq(target), "Invalid move");

    Move32b res = Move32b::Null;

    if (promotion)
        res = Move32b::makePromotion(origin, target, capture, promo_piece);
    else if (ep_capture)
        res = Move32b::makeEnPassant(origin, target);
    else if (short_castle) {
        ASSERT(pos.getOwnCastling().isShortPossible(), "Invalid O-O move");
        res = Move32b::makeCastling<Move32b::Castle::SHORT>(origin, target);
    }
    else if (long_castle) {
        ASSERT(pos.getOwnCastling().isLongPossible(), "Invalid O-O-O move");
        res = Move32b::makeCastling<Move32b::Castle::LONG>(origin, target);
    }
    else res = Move32b::makeSimple(origin, target, capture, piece);

    return res;
}

template <>
template <>
Move32b Move32b::fromStr<Move32b::Notation::REGULAR>(const Position& pos, const std::string& str) {
    ASSERT(str.size() == 4 or str.size() == 5, "Invalid move");

    Square                  origin = Square::fromChar(str[0], str[1]),
                          target = Square::fromChar(str[2], str[3]);
    const Piece::enumType piece = pos.pieceOn(origin, pos.getTurn());
    const bool              capture = pos.getOppositePieces().isOccupiedSq(target),
                          ep_capture = piece == Piece::PAWN and target == pos.getEnPassantSq(),
                          promotion = str.size() == 5,
                          short_castle = piece == Piece::KING and origin - target == -2,
                          long_castle = piece == Piece::KING and origin - target == 2;
    const Piece::enumType promo_piece = promotion ? Piece::typeFromChar(str[4]) : Piece::NONE;

    return createMove(pos, origin, target, piece, 
                     capture, ep_capture, promotion, 
                     short_castle, long_castle, promo_piece);
}

template <>
bool Move16b::isPackedCapture(const Position& pos) const {
    const Square dst = getTarget();
    return pos.getOppositePieces().isOccupiedSq(dst);
}

template <>
template <>
Move32b Move32b::fromStr<Move32b::Notation::ALGEBRAIC>(const Position& pos, const std::string& str) {
    Square       origin = Square::None,
               target = Square::None;
    char       chpromo = '\0';
    const bool capture = str.find('x') != std::string::npos or str.find('X') != std::string::npos,
               promotion = str.find('=') != std::string::npos,
               short_castle = str == "O-O" or str == "0-0",
               long_castle = str == "O-O-O" or str == "0-0-0";

    const size_t last = str.back() == '+' ? str.size() - 2 : str.size() - 1;

    if (short_castle) {
        ASSERT(pos.getOwnCastling().isShortPossible(), "Invalid castling move");
        origin = pos.getTurn() == WHITE ? Square::SQ_E1 : Square::SQ_E8;
        target = pos.getTurn() == WHITE ? Square::SQ_G1 : Square::SQ_G8;
    }
    else if (long_castle) {
        ASSERT(pos.getOwnCastling().isLongPossible(), "Invalid castling move");
        origin = pos.getTurn() == WHITE ? Square::SQ_E1 : Square::SQ_E8;
        target = pos.getTurn() == WHITE ? Square::SQ_C1 : Square::SQ_C8;
    }
    else if (promotion) {
        ASSERT(last >= 3, "Invalid move format");
        target = Square::fromChar(str[last - 3], str[last - 2]);

        if (!capture)
            origin = pos.getTurn() == WHITE ? Square(static_cast<int>(target) - 8)
                                            : Square(static_cast<int>(target) + 8);
        else origin = Square::fromChar(str[0], pos.getTurn() == WHITE ? '7' : '1');

        chpromo = str[last];
    }
    else {
        target = Square::fromChar(str[last - 1], str[last]);

        if (str.size() == 2 and !capture) {
            if (pos.getTurn() == WHITE ? target.getRank() == 3 : target.getRank() == 4) {
                Piece::enumType ddp = pos.pieceOn(
                    pos.getTurn() == WHITE ?
                        Square(static_cast<int>(target) - 16) : 
                        Square(static_cast<int>(target) + 16), pos.getTurn());

                Piece::enumType dp = pos.pieceOn(
                    pos.getTurn() == WHITE ?
                        Square(static_cast<int>(target) - 8) : 
                        Square(static_cast<int>(target) + 8), pos.getTurn());

                if (ddp == Piece::PAWN and dp == Piece::NONE) {
                    origin = pos.getTurn() == WHITE ? Square(static_cast<int>(target) - 16)
                                                    : Square(static_cast<int>(target) + 16);
                }
            }

            if (origin.isNull())
                origin = pos.getTurn() == WHITE ? Square(static_cast<int>(target) - 8)
                                                : Square(static_cast<int>(target) + 8);
        }
        else if (std::islower(str[0]) and capture)
            origin = Square::fromChar(str[0], pos.getTurn() == WHITE ? str[3] - 1 : str[3] + 1);
        else {
            Piece::enumType piece = Piece::typeFromChar(str[0]);
            BitBoard bb = attacks(piece, target, pos.getOccupied()) & pos.get(piece, pos.getTurn());
            ASSERT(bb != BitBoard::Empty, "Invalid capture");

            if (bb.popCount() > 1) {
                char id = str[1];
                size_t idn = 0;

                if ((idn = static_cast<int>(std::string_view("abcdefgh").find(id))) != std::string::npos) {
                    BitBoard file = BitBoard::file(static_cast<int>(idn));
                    bb &= file;
                    
                    if (bb.popCount() > 1) {
                        id = str[2];
                        BitBoard rank = BitBoard::rank(static_cast<int>(id - '1'));
                        bb &= rank;
                        origin = Square(bb.bitScanForward());
                    }
                    else origin = Square(bb.bitScanForward());
                }
                else {
                    BitBoard rank = BitBoard::rank(static_cast<int>(id - '1'));
                    bb &= rank;
                    origin = Square(bb.bitScanForward());
                }
            }
            else origin = Square(bb.bitScanForward());
        }
    }

    std::string puremove = origin.toStr() + target.toStr();
    if (promotion) puremove.append(1, chpromo);

    return fromStr<Move32b::Notation::REGULAR>(pos, puremove);
}

template <typename T>
enumColor MoveData<T>::getPieceColor(const Position& pos) const {
    const Square org = getOrigin();
    return pos.getBySide(WHITE) & BitBoard(org) ? WHITE : BLACK;
}

template <typename T>
Piece::enumType MoveData<T>::getCaptured(const Position& pos) const {
    const Square dst = getTarget();
    return pos.pieceOn(dst, pos.getOppositeTurn());
}

template <>
bool Move32b::isPseudoLegal(const Position& pos) const {
    if (*this == Move32b::Null) 
        return false;

    const Square org = getOrigin(), 
                 dst = getTarget();

    const Piece::enumType p = getPiece(),
                          d = pos.pieceOn(dst, pos.getOppositeTurn());

    if (p == Piece::KING) {
        if (kingAttacks(pos.getKingSquareBySide(pos.getOppositeTurn())).isOccupiedSq(dst))
            return false;

        if (isShortCastle()) {
            const CastlingRights own_castling_state = pos.getCastlingByColor(pos.getTurn());

            return own_castling_state.isShortPossible() and
                  (own_castling_state.notThroughPieces_Short(pos.getOccupied(), pos.getTurn())) and
                  !pos.isInCheck(pos.getTurn()) and
                  (own_castling_state.notThroughCheck_Short(pos, pos.getTurn()));
        }
        else if (isLongCastle()) {
            const CastlingRights own_castling_state = pos.getCastlingByColor(pos.getTurn());

            return own_castling_state.isLongPossible() and
                  (own_castling_state.notThroughPieces_Long(pos.getOccupied(), pos.getTurn())) and
                  !pos.isInCheck(pos.getTurn()) and
                  (own_castling_state.notThroughCheck_Long(pos, pos.getTurn()));
        }
    }
    else if (isEnPassant()) {
        return pos.pieceOn(org, pos.getTurn()) == Piece::PAWN and
               pos.getEnPassantSq() == dst;
    }

    return p == pos.pieceOn(org, pos.getTurn()) and
          (!isCapture() or d != Piece::NONE) and
          (!isQuiet() or (d == Piece::NONE and pos.pieceOn(dst, pos.getTurn()) == Piece::NONE)) and
          (p == Piece::KNIGHT or !(onlyBetween(org, dst) & pos.getOccupied()));
}

template <>
template <bool onlyQuiets>
bool Move32b::isPseudoLegal_fromList(const Position& pos) const {
    MoveList mlist;
    MoveGen::generatePseudoLegalMoves<onlyQuiets ? MoveGen::QUIETS : MoveGen::ALL>(pos, mlist);
    return mlist.contains(*this);
}

template <>
bool Move32b::isLegal(Position& pos) {
    const Position::IrreversibleState state = pos.getIrreversibleState();
    bool legal = pos.make(*this);
    pos.unmake(*this, state);
    return legal;
}

template <typename T>
void MoveData<T>::print(std::ostream& os) const {
#if defined(_PURE_NOTATION_DISPLAY)
    if (_rmove == Null) {
        std::cout << "0000";
    }
    else {
        getOrigin().print(os); 
        getTarget().print(os);

        if (isPromotion()) 
            Piece(BLACK, getPromoPiece()).print(os);
    }
#else
    ASSERT(false, "Printing moves in algebraic notation not supported");
#endif
}

Move32b unpackedMove(const Position& pos, Move16b move) {
    if (move.isNull()) 
        return Move32b::Null;

    Square                  origin = move.getOrigin(),
                          target = move.getTarget();
    const Piece::enumType piece = pos.pieceOn(origin, pos.getTurn());
    const bool              capture = pos.getOppositePieces().isOccupiedSq(target),
                          ep_capture = piece == Piece::PAWN and target == pos.getEnPassantSq(),
                          short_castle = piece == Piece::KING and origin - target == -2,
                          long_castle = piece == Piece::KING and origin - target == 2;
    const Piece::enumType promo_piece = move.getPromoPiece();
    const bool              promotion = move.isPromotion();
    const int              dir = pos.getTurn() == WHITE ? 8 : -8;
    const int              pawn_start_rank = pos.getTurn() == WHITE ? 1 : 6;

    if (piece == Piece::PAWN) {
        const bool double_push = target - origin == 2 * dir;

        if (double_push and origin.getRank() != pawn_start_rank)
            return Move32b::Null;

        const BitBoard pawn_capt = pawnAttacks(origin, pos.getTurn()) & BitBoard(target);

        if (pawn_capt and pawn_capt & pos.getEmpties() and pos.getEnPassantSq() != target)
            return Move32b::Null;
        else if (!pawn_capt and !double_push and target - origin != dir)
            return Move32b::Null;
    }
    else if (short_castle and
        !pos.getOwnCastling().isShortPossible()) {
        return Move32b::Null;
    }
    else if (long_castle and
        !pos.getOwnCastling().isLongPossible()) {
        return Move32b::Null;
    }
    
    if (piece == Piece::NONE or
        pos.getOwnPieces().isOccupiedSq(target) or
        pos.pieceOn(target, pos.getOppositeTurn()) == Piece::KING)
        return Move32b::Null;
    else if 
        (isSlider(piece) and
       !(attacks(piece, origin, pos.getOccupied()) & BitBoard(target)))
        return Move32b::Null;
    else if 
        (isSlider(piece) and
        (onlyBetween(origin, target) & pos.getOccupied()))
        return Move32b::Null;

    return createMove(pos, origin, target, piece, 
                      capture, ep_capture, promotion, 
                      short_castle, long_castle, promo_piece);
}

template enumColor Move16b::getPieceColor(const Position&) const;
template enumColor Move32b::getPieceColor(const Position&) const;

template Piece::enumType Move16b::getCaptured(const Position&) const;
template Piece::enumType Move32b::getCaptured(const Position&) const;

template bool Move32b::isPseudoLegal_fromList<true>(const Position&) const;
template bool Move32b::isPseudoLegal_fromList<false>(const Position&) const;

template void Move16b::print(std::ostream& os) const;
template void Move32b::print(std::ostream& os) const;
