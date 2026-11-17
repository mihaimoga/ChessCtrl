/* Copyright (C) 2024-2026 Stefan-Mihai MOGA
This file is part of ChessCtrl application developed by Stefan-Mihai MOGA.
Fully featured Chess Control written in C++ with the help of the MFC library.

ChessCtrl is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

ChessCtrl is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ChessCtrl. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

#include "pch.h"
#include "ChessSearch.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace ChessSearch
{
	namespace
	{
		constexpr int SIDE_WHITE = 0;
		constexpr int SIDE_BLACK = 1;

		// Centipawns by PieceKind, a king is never traded so it only has a value for the order of the captures
		constexpr int PIECE_VALUE[KING + 1] = { 0, 100, 320, 330, 500, 900, 20000 };

		constexpr int FULL_PHASE = 24;
		constexpr int BISHOP_PAIR_BONUS = 25;
		constexpr int DOUBLED_PAWN_PENALTY = 12;
		constexpr int ISOLATED_PAWN_PENALTY = 10;
		constexpr int ROOK_OPEN_FILE_BONUS = 14;
		constexpr int ROOK_HALF_OPEN_FILE_BONUS = 7;
		constexpr int SHIELD_NEAR_BONUS = 10;
		constexpr int SHIELD_FAR_BONUS = 5;
		constexpr int MOP_UP_EDGE_BONUS = 10;
		constexpr int MOP_UP_CLOSE_BONUS = 4;

		// Larger than any score, so that it can be the bounds of a window
		constexpr int INFINITE_SCORE = MATE_SCORE + 1000;

		constexpr int DELTA_MARGIN = 200;
		constexpr int HISTORY_LIMIT = 8000;

		// Everything that does not change with the position, built once when the program starts
		struct Tables
		{
			// The squares on a line from a square, in each of the 8 directions (the first 4 are straight, the others diagonal), -1 ends a line
			signed char ray[SQUARE_COUNT][8][8] = {};
			signed char knight[SQUARE_COUNT][9] = {};
			signed char king[SQUARE_COUNT][9] = {};

			uint64_t zobrist[2][KING + 1][SQUARE_COUNT] = {};
			uint64_t zobristSide = 0;

			// The value of a piece on a square, by side and kind (a king has its own tables)
			int pst[2][KING + 1][SQUARE_COUNT] = {};
			int kingMiddle[2][SQUARE_COUNT] = {};
			int kingEnd[2][SQUARE_COUNT] = {};

			Tables();
		};

		Tables::Tables()
		{
			static const int FILE_STEP[8] = { 0, 1, 0, -1, 1, 1, -1, -1 };
			static const int ROW_STEP[8] = { -1, 0, 1, 0, -1, 1, 1, -1 };
			static const int KNIGHT_FILE[8] = { 1, 2, 2, 1, -1, -2, -2, -1 };
			static const int KNIGHT_ROW[8] = { -2, -1, 1, 2, 2, 1, -1, -2 };

			// How far a pawn has advanced is worth more in the center than on the wings, and nothing on the last rank (no promotion)
			static const int PAWN_ADVANCE[8] = { 0, 0, 3, 7, 12, 14, 8, -20 };
			static const int PAWN_WEIGHT[8] = { 2, 3, 4, 4, 4, 4, 3, 2 };
			static const int PAWN_CENTER[8] = { -3, 0, 3, 6, 6, 3, 0, -3 };

			for (int nSquare = 0; nSquare < SQUARE_COUNT; ++nSquare)
			{
				const int nFile = nSquare & 7;
				const int nRow = nSquare >> 3;

				int nKingMoves = 0;
				for (int nDirection = 0; nDirection < 8; ++nDirection)
				{
					int nCount = 0;
					int nStepFile = nFile + FILE_STEP[nDirection];
					int nStepRow = nRow + ROW_STEP[nDirection];
					while ((nStepFile >= 0) && (nStepFile < 8) && (nStepRow >= 0) && (nStepRow < 8))
					{
						ray[nSquare][nDirection][nCount++] = static_cast<signed char>(nStepRow * 8 + nStepFile);
						nStepFile += FILE_STEP[nDirection];
						nStepRow += ROW_STEP[nDirection];
					}
					ray[nSquare][nDirection][nCount] = -1;
					if (nCount > 0)
					{
						king[nSquare][nKingMoves++] = ray[nSquare][nDirection][0];
					}
				}
				king[nSquare][nKingMoves] = -1;

				int nKnightMoves = 0;
				for (int nJump = 0; nJump < 8; ++nJump)
				{
					const int nJumpFile = nFile + KNIGHT_FILE[nJump];
					const int nJumpRow = nRow + KNIGHT_ROW[nJump];
					if ((nJumpFile >= 0) && (nJumpFile < 8) && (nJumpRow >= 0) && (nJumpRow < 8))
					{
						knight[nSquare][nKnightMoves++] = static_cast<signed char>(nJumpRow * 8 + nJumpFile);
					}
				}
				knight[nSquare][nKnightMoves] = -1;

				// The tables are written for White: the rank 1 is at the bottom (row 7), Black uses the mirrored square
				const int nAdvance = 7 - nRow;
				const int nCenter = (2 * nFile - 7) * (2 * nFile - 7) + (2 * nRow - 7) * (2 * nRow - 7); // 2 in the center, 98 in a corner
				const int nWing = std::abs(2 * nFile - 7);

				int nPawn = PAWN_ADVANCE[nAdvance] * PAWN_WEIGHT[nFile] / 4;
				if ((nAdvance >= 1) && (nAdvance <= 5))
				{
					nPawn += PAWN_CENTER[nFile];
				}
				const int nKnight = 24 - (nCenter * 3) / 4;
				const int nBishop = 12 - nCenter / 8;
				const int nRook = ((nAdvance == 6) ? 10 : 0) + (((nFile == 3) || (nFile == 4)) ? 3 : 0);
				const int nQueen = 6 - nCenter / 16;
				const int nKingMiddle = -10 * nAdvance + (nWing - 1) * 2;
				const int nKingEnd = 20 - nCenter / 5;

				const int nMirror = nSquare ^ 56;
				pst[SIDE_WHITE][PAWN][nSquare] = nPawn;
				pst[SIDE_BLACK][PAWN][nMirror] = nPawn;
				pst[SIDE_WHITE][KNIGHT][nSquare] = nKnight;
				pst[SIDE_BLACK][KNIGHT][nMirror] = nKnight;
				pst[SIDE_WHITE][BISHOP][nSquare] = nBishop;
				pst[SIDE_BLACK][BISHOP][nMirror] = nBishop;
				pst[SIDE_WHITE][ROOK][nSquare] = nRook;
				pst[SIDE_BLACK][ROOK][nMirror] = nRook;
				pst[SIDE_WHITE][QUEEN][nSquare] = nQueen;
				pst[SIDE_BLACK][QUEEN][nMirror] = nQueen;
				kingMiddle[SIDE_WHITE][nSquare] = nKingMiddle;
				kingMiddle[SIDE_BLACK][nMirror] = nKingMiddle;
				kingEnd[SIDE_WHITE][nSquare] = nKingEnd;
				kingEnd[SIDE_BLACK][nMirror] = nKingEnd;
			}

			// A fixed seed, so that the search of a position never changes
			uint64_t nState = 0x9E3779B97F4A7C15ull;
			const auto next = [&nState]()
			{
				nState += 0x9E3779B97F4A7C15ull;
				uint64_t nValue = nState;
				nValue = (nValue ^ (nValue >> 30)) * 0xBF58476D1CE4E5B9ull;
				nValue = (nValue ^ (nValue >> 27)) * 0x94D049BB133111EBull;
				return nValue ^ (nValue >> 31);
			};
			for (int nSide = 0; nSide < 2; ++nSide)
			{
				for (int nKind = PAWN; nKind <= KING; ++nKind)
				{
					for (int nSquare = 0; nSquare < SQUARE_COUNT; ++nSquare)
					{
						zobrist[nSide][nKind][nSquare] = next();
					}
				}
			}
			zobristSide = next();
		}

		const Tables g_tables;

		inline uint64_t PieceKey(const int nPiece, const int nSquare)
		{
			return (nPiece > 0) ? g_tables.zobrist[SIDE_WHITE][nPiece][nSquare] : g_tables.zobrist[SIDE_BLACK][-nPiece][nSquare];
		}

		// The pawns of a king that stand in front of it, which cover it in the middle game
		int PawnShield(const int* pBoard, const int nKing, const int nSide)
		{
			const int nFile = nKing & 7;
			const int nRow = nKing >> 3;
			const int nStep = (nSide == SIDE_WHITE) ? -1 : 1;
			const int nOwnPawn = (nSide == SIDE_WHITE) ? PAWN : -PAWN;

			int nShield = 0;
			for (int nColumn = nFile - 1; nColumn <= nFile + 1; ++nColumn)
			{
				const int nNearRow = nRow + nStep;
				if ((nColumn < 0) || (nColumn > 7) || (nNearRow < 0) || (nNearRow > 7))
				{
					continue;
				}
				if (pBoard[nNearRow * 8 + nColumn] == nOwnPawn)
				{
					nShield += SHIELD_NEAR_BONUS;
					continue;
				}
				const int nFarRow = nNearRow + nStep;
				if ((nFarRow >= 0) && (nFarRow <= 7) && (pBoard[nFarRow * 8 + nColumn] == nOwnPawn))
				{
					nShield += SHIELD_FAR_BONUS;
				}
			}
			return nShield;
		}
	}

	Position::Position()
	{
		Clear();
	}

	void Position::Clear()
	{
		for (int& nPiece : m_board)
		{
			nPiece = 0;
		}
		m_bWhiteToMove = true;
		m_nKingSquare[SIDE_WHITE] = -1;
		m_nKingSquare[SIDE_BLACK] = -1;
		m_nHash = 0;
	}

	void Position::Put(const int nSquare, const int nKind, const bool bWhite)
	{
		if ((nSquare < 0) || (nSquare >= SQUARE_COUNT))
		{
			return;
		}

		const int nOld = m_board[nSquare];
		if (nOld != 0)
		{
			m_nHash ^= PieceKey(nOld, nSquare);
			const int nOldSide = (nOld > 0) ? SIDE_WHITE : SIDE_BLACK;
			if (m_nKingSquare[nOldSide] == nSquare)
			{
				m_nKingSquare[nOldSide] = -1;
			}
		}

		if ((nKind < PAWN) || (nKind > KING))
		{
			m_board[nSquare] = 0;
			return;
		}

		const int nSide = bWhite ? SIDE_WHITE : SIDE_BLACK;
		const int nPiece = bWhite ? nKind : -nKind;
		m_board[nSquare] = nPiece;
		m_nHash ^= PieceKey(nPiece, nSquare);
		if ((nKind == KING) && (m_nKingSquare[nSide] < 0))
		{
			m_nKingSquare[nSide] = nSquare;
		}
	}

	void Position::SetWhiteToMove(const bool bWhiteToMove)
	{
		if (m_bWhiteToMove != bWhiteToMove)
		{
			m_bWhiteToMove = bWhiteToMove;
			m_nHash ^= g_tables.zobristSide;
		}
	}

	int Position::GenerateMoves(Move* pMoves, const bool bCapturesOnly) const
	{
		const Tables& tables = g_tables;
		const bool bWhite = m_bWhiteToMove;
		int nMoves = 0;

		const auto add = [pMoves, &nMoves](const int nFrom, const int nTo)
		{
			if (nMoves < MAX_MOVES)
			{
				pMoves[nMoves].nFrom = nFrom;
				pMoves[nMoves].nTo = nTo;
				++nMoves;
			}
		};

		for (int nFrom = 0; nFrom < SQUARE_COUNT; ++nFrom)
		{
			const int nPiece = m_board[nFrom];
			if ((nPiece == 0) || ((nPiece > 0) != bWhite))
			{
				continue;
			}

			const int nKind = (nPiece > 0) ? nPiece : -nPiece;
			if (nKind == PAWN)
			{
				const int nRow = nFrom >> 3;
				const int nFile = nFrom & 7;
				if (nRow == (bWhite ? 0 : 7))
				{
					continue; // there is no promotion, a pawn on the last rank stays there
				}

				const int nStep = bWhite ? -8 : 8;
				const int nAhead = nFrom + nStep;
				if (!bCapturesOnly && (m_board[nAhead] == 0))
				{
					add(nFrom, nAhead);
					if ((nRow == (bWhite ? 6 : 1)) && (m_board[nAhead + nStep] == 0))
					{
						add(nFrom, nAhead + nStep);
					}
				}
				if (nFile > 0)
				{
					const int nTarget = m_board[nAhead - 1];
					if ((nTarget != 0) && ((nTarget > 0) != bWhite))
					{
						add(nFrom, nAhead - 1);
					}
				}
				if (nFile < 7)
				{
					const int nTarget = m_board[nAhead + 1];
					if ((nTarget != 0) && ((nTarget > 0) != bWhite))
					{
						add(nFrom, nAhead + 1);
					}
				}
			}
			else if ((nKind == KNIGHT) || (nKind == KING))
			{
				const signed char* pTargets = (nKind == KNIGHT) ? tables.knight[nFrom] : tables.king[nFrom];
				for (; *pTargets >= 0; ++pTargets)
				{
					const int nTarget = m_board[*pTargets];
					if (nTarget == 0)
					{
						if (!bCapturesOnly)
						{
							add(nFrom, *pTargets);
						}
					}
					else if ((nTarget > 0) != bWhite)
					{
						add(nFrom, *pTargets);
					}
				}
			}
			else
			{
				const int nFirstDirection = (nKind == BISHOP) ? 4 : 0;
				const int nLastDirection = (nKind == ROOK) ? 4 : 8;
				for (int nDirection = nFirstDirection; nDirection < nLastDirection; ++nDirection)
				{
					for (const signed char* pTarget = tables.ray[nFrom][nDirection]; *pTarget >= 0; ++pTarget)
					{
						const int nTarget = m_board[*pTarget];
						if (nTarget == 0)
						{
							if (!bCapturesOnly)
							{
								add(nFrom, *pTarget);
							}
							continue;
						}
						if ((nTarget > 0) != bWhite)
						{
							add(nFrom, *pTarget);
						}
						break;
					}
				}
			}
		}
		return nMoves;
	}

	std::vector<Move> Position::LegalMoves() const
	{
		Position position(*this);
		Move moves[MAX_MOVES];
		const int nMoves = position.GenerateMoves(moves, false);

		std::vector<Move> legalMoves;
		for (int nMove = 0; nMove < nMoves; ++nMove)
		{
			Undo undo;
			if (position.MakeMove(moves[nMove], undo))
			{
				legalMoves.push_back(moves[nMove]);
				position.UnmakeMove(moves[nMove], undo);
			}
		}
		return legalMoves;
	}

	bool Position::MakeMove(const Move& move, Undo& undo)
	{
		const int nPiece = m_board[move.nFrom];
		const int nCaptured = m_board[move.nTo];
		const int nSide = (nPiece > 0) ? SIDE_WHITE : SIDE_BLACK;

		undo.nCaptured = nCaptured;
		undo.nKingSquare[SIDE_WHITE] = m_nKingSquare[SIDE_WHITE];
		undo.nKingSquare[SIDE_BLACK] = m_nKingSquare[SIDE_BLACK];
		undo.nHash = m_nHash;

		m_board[move.nTo] = nPiece;
		m_board[move.nFrom] = 0;
		m_nHash ^= PieceKey(nPiece, move.nFrom) ^ PieceKey(nPiece, move.nTo);
		if (nCaptured != 0)
		{
			m_nHash ^= PieceKey(nCaptured, move.nTo);
			if ((nCaptured == KING) || (nCaptured == -KING))
			{
				m_nKingSquare[1 - nSide] = -1;
			}
		}
		if ((nPiece == KING) || (nPiece == -KING))
		{
			m_nKingSquare[nSide] = move.nTo;
		}

		const int nKing = m_nKingSquare[nSide];
		if ((nKing >= 0) && IsAttacked(nKing, nSide == SIDE_BLACK))
		{
			Restore(move, undo);
			return false;
		}

		m_bWhiteToMove = !m_bWhiteToMove;
		m_nHash ^= g_tables.zobristSide;
		return true;
	}

	void Position::UnmakeMove(const Move& move, const Undo& undo)
	{
		Restore(move, undo);
		m_bWhiteToMove = !m_bWhiteToMove;
	}

	void Position::Restore(const Move& move, const Undo& undo)
	{
		m_board[move.nFrom] = m_board[move.nTo];
		m_board[move.nTo] = undo.nCaptured;
		m_nKingSquare[SIDE_WHITE] = undo.nKingSquare[SIDE_WHITE];
		m_nKingSquare[SIDE_BLACK] = undo.nKingSquare[SIDE_BLACK];
		m_nHash = undo.nHash;
	}

	void Position::MakeNullMove(Undo& undo)
	{
		undo.nHash = m_nHash;
		m_bWhiteToMove = !m_bWhiteToMove;
		m_nHash ^= g_tables.zobristSide;
	}

	void Position::UnmakeNullMove(const Undo& undo)
	{
		m_bWhiteToMove = !m_bWhiteToMove;
		m_nHash = undo.nHash;
	}

	bool Position::IsAttacked(const int nSquare, const bool bByWhite) const
	{
		const Tables& tables = g_tables;
		const int nSign = bByWhite ? 1 : -1;
		const int nFile = nSquare & 7;
		const int nRow = nSquare >> 3;

		// A pawn captures towards the other side of the board, so it stands on the row behind the square it attacks
		if (bByWhite)
		{
			if (nRow < 7)
			{
				if ((nFile > 0) && (m_board[nSquare + 7] == PAWN))
				{
					return true;
				}
				if ((nFile < 7) && (m_board[nSquare + 9] == PAWN))
				{
					return true;
				}
			}
		}
		else if (nRow > 0)
		{
			if ((nFile > 0) && (m_board[nSquare - 9] == -PAWN))
			{
				return true;
			}
			if ((nFile < 7) && (m_board[nSquare - 7] == -PAWN))
			{
				return true;
			}
		}

		for (const signed char* pTarget = tables.knight[nSquare]; *pTarget >= 0; ++pTarget)
		{
			if (m_board[*pTarget] == nSign * KNIGHT)
			{
				return true;
			}
		}
		for (const signed char* pTarget = tables.king[nSquare]; *pTarget >= 0; ++pTarget)
		{
			if (m_board[*pTarget] == nSign * KING)
			{
				return true;
			}
		}

		for (int nDirection = 0; nDirection < 8; ++nDirection)
		{
			const int nSlider = (nDirection < 4) ? ROOK : BISHOP;
			for (const signed char* pTarget = tables.ray[nSquare][nDirection]; *pTarget >= 0; ++pTarget)
			{
				const int nPiece = m_board[*pTarget];
				if (nPiece == 0)
				{
					continue;
				}
				if ((nPiece == nSign * nSlider) || (nPiece == nSign * QUEEN))
				{
					return true;
				}
				break;
			}
		}
		return false;
	}

	bool Position::IsInCheck() const
	{
		const int nKing = m_nKingSquare[m_bWhiteToMove ? SIDE_WHITE : SIDE_BLACK];
		return (nKing >= 0) && IsAttacked(nKing, !m_bWhiteToMove);
	}

	bool Position::HasPieces(const bool bWhite) const
	{
		for (int nSquare = 0; nSquare < SQUARE_COUNT; ++nSquare)
		{
			const int nPiece = bWhite ? m_board[nSquare] : -m_board[nSquare];
			if ((nPiece > PAWN) && (nPiece < KING))
			{
				return true;
			}
		}
		return false;
	}

	int Position::Evaluate() const
	{
		const Tables& tables = g_tables;

		int nScore[2] = { 0, 0 };
		int nPieceValue[2] = { 0, 0 }; // the pieces that are not pawns
		int nPawns[2] = { 0, 0 };
		int nBishops[2] = { 0, 0 };
		int nBishopSquare[2] = { 0, 0 };
		int nKnights[2] = { 0, 0 };
		int nPawnsOnFile[2][8] = {};
		int nRooks[2] = { 0, 0 };
		int nRookSquare[2][8] = {};
		int nPhase = 0;

		for (int nSquare = 0; nSquare < SQUARE_COUNT; ++nSquare)
		{
			const int nPiece = m_board[nSquare];
			if (nPiece == 0)
			{
				continue;
			}

			const int nSide = (nPiece > 0) ? SIDE_WHITE : SIDE_BLACK;
			const int nKind = (nPiece > 0) ? nPiece : -nPiece;
			if (nKind == KING)
			{
				continue;
			}

			nScore[nSide] += PIECE_VALUE[nKind] + tables.pst[nSide][nKind][nSquare];
			if (nKind == PAWN)
			{
				++nPawns[nSide];
				++nPawnsOnFile[nSide][nSquare & 7];
				continue;
			}

			nPieceValue[nSide] += PIECE_VALUE[nKind];
			switch (nKind)
			{
			case BISHOP:
				++nBishops[nSide];
				nBishopSquare[nSide] = nSquare;
				nPhase += 1;
				break;
			case ROOK:
				if (nRooks[nSide] < 8)
				{
					nRookSquare[nSide][nRooks[nSide]++] = nSquare;
				}
				nPhase += 2;
				break;
			case QUEEN:
				nPhase += 4;
				break;
			default:
				++nKnights[nSide];
				nPhase += 1;
				break;
			}
		}

		// Neither player can give checkmate with a king and a single piece against a king and a single piece
		if ((nPawns[SIDE_WHITE] + nPawns[SIDE_BLACK] == 0) && (nPieceValue[SIDE_WHITE] <= PIECE_VALUE[BISHOP]) && (nPieceValue[SIDE_BLACK] <= PIECE_VALUE[BISHOP]))
		{
			return 0;
		}

		nPhase = (std::min)(nPhase, FULL_PHASE);
		for (int nSide = 0; nSide < 2; ++nSide)
		{
			const int nOther = 1 - nSide;

			if (nBishops[nSide] >= 2)
			{
				nScore[nSide] += BISHOP_PAIR_BONUS;
			}

			for (int nFile = 0; nFile < 8; ++nFile)
			{
				const int nCount = nPawnsOnFile[nSide][nFile];
				if (nCount == 0)
				{
					continue;
				}
				nScore[nSide] -= DOUBLED_PAWN_PENALTY * (nCount - 1);
				const bool bLeft = (nFile > 0) && (nPawnsOnFile[nSide][nFile - 1] > 0);
				const bool bRight = (nFile < 7) && (nPawnsOnFile[nSide][nFile + 1] > 0);
				if (!bLeft && !bRight)
				{
					nScore[nSide] -= ISOLATED_PAWN_PENALTY * nCount;
				}
			}

			for (int nRook = 0; nRook < nRooks[nSide]; ++nRook)
			{
				const int nFile = nRookSquare[nSide][nRook] & 7;
				if (nPawnsOnFile[nSide][nFile] == 0)
				{
					nScore[nSide] += (nPawnsOnFile[nOther][nFile] == 0) ? ROOK_OPEN_FILE_BONUS : ROOK_HALF_OPEN_FILE_BONUS;
				}
			}

			// The king hides in the middle game and comes to the center in the end game
			const int nKing = m_nKingSquare[nSide];
			if (nKing >= 0)
			{
				nScore[nSide] += (tables.kingMiddle[nSide][nKing] * nPhase + tables.kingEnd[nSide][nKing] * (FULL_PHASE - nPhase)) / FULL_PHASE;
				nScore[nSide] += PawnShield(m_board, nKing, nSide) * nPhase / FULL_PHASE;
			}
		}

		// A player with a rook or more against a bare king drives it to the edge of the board and brings its own king close
		if ((m_nKingSquare[SIDE_WHITE] >= 0) && (m_nKingSquare[SIDE_BLACK] >= 0))
		{
			for (int nWinner = 0; nWinner < 2; ++nWinner)
			{
				const int nLoser = 1 - nWinner;
				if ((nPawns[nLoser] == 0) && (nPieceValue[nLoser] <= PIECE_VALUE[BISHOP]) && (nPieceValue[nWinner] >= PIECE_VALUE[ROOK]))
				{
					const int nLoserKing = m_nKingSquare[nLoser];
					const int nWinnerKing = m_nKingSquare[nWinner];
					int nEdge = std::abs(2 * (nLoserKing & 7) - 7) + std::abs(2 * (nLoserKing >> 3) - 7); // 2 in the center, 14 in a corner
					if ((nPieceValue[nLoser] == 0) && (nBishops[nWinner] == 1) && (nKnights[nWinner] == 1) &&
						(nPieceValue[nWinner] == PIECE_VALUE[BISHOP] + PIECE_VALUE[KNIGHT]))
					{
						// A bishop and a knight give checkmate only in a corner of the color of the bishop: 14 in those two, 7 in the other two
						const int nFile = nLoserKing & 7;
						const int nRow = nLoserKing >> 3;
						const bool bSameColorAsA8 = (((nBishopSquare[nWinner] & 7) + (nBishopSquare[nWinner] >> 3)) & 1) == 0;
						const int nCorner = bSameColorAsA8 ? (std::min)(nFile + nRow, 14 - nFile - nRow) : (std::min)(7 - nFile + nRow, nFile + 7 - nRow);
						nEdge = 14 - nCorner;
					}
					const int nDistance = std::abs((nLoserKing & 7) - (nWinnerKing & 7)) + std::abs((nLoserKing >> 3) - (nWinnerKing >> 3));
					nScore[nWinner] += MOP_UP_EDGE_BONUS * nEdge + MOP_UP_CLOSE_BONUS * (14 - nDistance);
				}
			}
		}

		const int nMe = m_bWhiteToMove ? SIDE_WHITE : SIDE_BLACK;
		return nScore[nMe] - nScore[1 - nMe];
	}

	uint64_t Position::Perft(const int nDepth) const
	{
		Position position(*this);
		return position.PerftOfLine(nDepth);
	}

	uint64_t Position::PerftOfLine(const int nDepth)
	{
		if (nDepth <= 0)
		{
			return 1;
		}

		Move moves[MAX_MOVES];
		const int nMoves = GenerateMoves(moves, false);
		uint64_t nLines = 0;
		for (int nMove = 0; nMove < nMoves; ++nMove)
		{
			Undo undo;
			if (MakeMove(moves[nMove], undo))
			{
				nLines += PerftOfLine(nDepth - 1);
				UnmakeMove(moves[nMove], undo);
			}
		}
		return nLines;
	}

	namespace
	{
		constexpr uint8_t BOUND_NONE = 0;
		constexpr uint8_t BOUND_EXACT = 1;
		constexpr uint8_t BOUND_LOWER = 2; // the score is at least this one
		constexpr uint8_t BOUND_UPPER = 3; // the score is at most this one

		struct TableEntry
		{
			uint64_t nKey = 0;
			int16_t nScore = 0;
			uint16_t nMove = 0; // 0 for no move
			int8_t nDepth = 0;
			uint8_t nBound = BOUND_NONE;
		};

		uint16_t EncodeMove(const Move& move)
		{
			return (move.nFrom < 0) ? static_cast<uint16_t>(0) : static_cast<uint16_t>(1 + move.nFrom * SQUARE_COUNT + move.nTo);
		}

		Move DecodeMove(const uint16_t nCode)
		{
			Move move;
			if (nCode != 0)
			{
				move.nFrom = (nCode - 1) / SQUARE_COUNT;
				move.nTo = (nCode - 1) % SQUARE_COUNT;
			}
			return move;
		}

		// A score of checkmate depends on the distance from the position where the search started, which a table entry does not know
		int ScoreToTable(const int nScore, const int nPly)
		{
			if (nScore >= MATE_BOUND)
			{
				return nScore + nPly;
			}
			return (nScore <= -MATE_BOUND) ? (nScore - nPly) : nScore;
		}

		int ScoreFromTable(const int nScore, const int nPly)
		{
			if (nScore >= MATE_BOUND)
			{
				return nScore - nPly;
			}
			return (nScore <= -MATE_BOUND) ? (nScore + nPly) : nScore;
		}

		// Brings the move that has the highest score of those from nIndex on to nIndex
		void PickMove(Move* pMoves, int* pScores, const int nIndex, const int nCount)
		{
			int nBest = nIndex;
			for (int nMove = nIndex + 1; nMove < nCount; ++nMove)
			{
				if (pScores[nMove] > pScores[nBest])
				{
					nBest = nMove;
				}
			}
			if (nBest != nIndex)
			{
				std::swap(pMoves[nIndex], pMoves[nBest]);
				std::swap(pScores[nIndex], pScores[nBest]);
			}
		}

		/* Negamax search with alpha-beta pruning, deepened one ply at a time. It looks at the captures after the last ply (the
		   quiescence search), answers every check, and orders the moves by the table of positions, the captures (the most
		   valuable victim by the least valuable attacker), the killer moves and the history of the moves that were good.
		   A player that has a piece other than pawns can pass the turn to see if its position is that good, and the moves
		   that are late in the order and quiet are searched less deep unless they turn out to be good.
		*/
		class Searcher
		{
		public:
			Searcher(const Position& position, const Limits& limits);

			Result Run(const std::vector<Move>& rootMoves);

		private:
			struct RootMove
			{
				Move move;
				int nOrder = 0;
			};

			int Search(int nDepth, int nAlpha, int nBeta, int nPly, bool bAllowNull, bool bInCheck);
			int Quiesce(int nAlpha, int nBeta, int nPly, bool bInCheck);
			int ScoreMove(const Move& move, const Move& tableMove, int nPly) const;
			void RecordCutoff(const Move& move, int nDepth, int nPly);
			void EnterLine(const Move& move, const Undo& undo, int nPly);
			void PollStop();

			Position m_position;
			Limits m_limits;
			std::vector<TableEntry> m_table;
			uint64_t m_nTableMask;
			Move m_killers[MAX_PLY][2];
			int m_history[2][SQUARE_COUNT][SQUARE_COUNT];
			uint64_t m_path[MAX_PLY]; // the position at each ply of the line being searched, the first is the one where the search started
			int m_nRepeatFrom[MAX_PLY]; // the first ply of the line from which a position can come again
			uint64_t m_nNodes;
			int m_nMaxDepth;
			int m_nExtensionLimit;
			bool m_bStopped;
			bool m_bUserStop;
		};

		Searcher::Searcher(const Position& position, const Limits& limits)
			: m_position(position)
			, m_limits(limits)
			, m_nTableMask(0)
			, m_nNodes(0)
			, m_nMaxDepth(1)
			, m_nExtensionLimit(0)
			, m_bStopped(false)
			, m_bUserStop(false)
		{
			m_nMaxDepth = (std::max)(1, (std::min)(limits.nMaxDepth, MAX_PLY / 2 - 4));

			// The deeper the search, the more positions the table has to hold
			const int nBits = (std::max)(12, (std::min)(8 + 2 * m_nMaxDepth, 20));
			m_table.assign(static_cast<size_t>(1) << nBits, TableEntry());
			m_nTableMask = (static_cast<uint64_t>(1) << nBits) - 1;

			std::memset(m_history, 0, sizeof(m_history));
			std::memset(m_path, 0, sizeof(m_path));
			std::memset(m_nRepeatFrom, 0, sizeof(m_nRepeatFrom));
		}

		// Called once a move of the line has been made: a capture or a pawn move cannot be taken back, so no position before it can come again
		void Searcher::EnterLine(const Move& move, const Undo& undo, const int nPly)
		{
			const bool bReversible = (undo.nCaptured == 0) && (std::abs(m_position.PieceAt(move.nTo)) != PAWN);
			m_nRepeatFrom[nPly + 1] = bReversible ? m_nRepeatFrom[nPly] : nPly + 1;
		}

		void Searcher::PollStop()
		{
			if ((m_limits.nMaxNodes != 0) && (m_nNodes >= m_limits.nMaxNodes))
			{
				m_bStopped = true;
			}
			if ((m_limits.pbContinue != nullptr) && !*m_limits.pbContinue)
			{
				m_bStopped = true;
				m_bUserStop = true;
			}
		}

		int Searcher::ScoreMove(const Move& move, const Move& tableMove, const int nPly) const
		{
			if (move == tableMove)
			{
				return 1000000;
			}

			const int nVictim = std::abs(m_position.PieceAt(move.nTo));
			if (nVictim != 0)
			{
				return 100000 + nVictim * 16 - std::abs(m_position.PieceAt(move.nFrom));
			}
			if (move == m_killers[nPly][0])
			{
				return 90000;
			}
			if (move == m_killers[nPly][1])
			{
				return 89999;
			}
			return m_history[m_position.IsWhiteToMove() ? SIDE_WHITE : SIDE_BLACK][move.nFrom][move.nTo];
		}

		// Called once the move has been taken back, so the player to move is the one that played it
		void Searcher::RecordCutoff(const Move& move, const int nDepth, const int nPly)
		{
			if (move != m_killers[nPly][0])
			{
				m_killers[nPly][1] = m_killers[nPly][0];
				m_killers[nPly][0] = move;
			}
			int& nHistory = m_history[m_position.IsWhiteToMove() ? SIDE_WHITE : SIDE_BLACK][move.nFrom][move.nTo];
			nHistory = (std::min)(nHistory + nDepth * nDepth, HISTORY_LIMIT);
		}

		int Searcher::Quiesce(int nAlpha, const int nBeta, const int nPly, const bool bInCheck)
		{
			++m_nNodes;
			if ((m_nNodes & 1023) == 0)
			{
				PollStop();
			}
			if (m_bStopped)
			{
				return 0;
			}
			if (nPly >= MAX_PLY - 1)
			{
				return m_position.Evaluate();
			}

			// A player in check cannot stand pat, it has to get out of it: every move is searched
			int nStandPat = -INFINITE_SCORE;
			int nBestScore = -INFINITE_SCORE;
			if (!bInCheck)
			{
				nStandPat = m_position.Evaluate();
				if (nStandPat >= nBeta)
				{
					return nStandPat;
				}
				nAlpha = (std::max)(nAlpha, nStandPat);
				nBestScore = nStandPat;
			}

			Move moves[MAX_MOVES];
			int nScores[MAX_MOVES];
			const int nMoves = m_position.GenerateMoves(moves, !bInCheck);
			const Move noMove;
			for (int nMove = 0; nMove < nMoves; ++nMove)
			{
				nScores[nMove] = ScoreMove(moves[nMove], noMove, nPly);
			}

			int nLegalMoves = 0;
			for (int nMove = 0; nMove < nMoves; ++nMove)
			{
				PickMove(moves, nScores, nMove, nMoves);
				const Move move = moves[nMove];

				// A capture that cannot bring the score up to alpha, even with something to spare, is not worth a look
				if (!bInCheck && (nStandPat + PIECE_VALUE[std::abs(m_position.PieceAt(move.nTo))] + DELTA_MARGIN < nAlpha))
				{
					continue;
				}

				Undo undo;
				if (!m_position.MakeMove(move, undo))
				{
					continue;
				}
				++nLegalMoves;
				const int nScore = -Quiesce(-nBeta, -nAlpha, nPly + 1, m_position.IsInCheck());
				m_position.UnmakeMove(move, undo);
				if (m_bStopped)
				{
					return 0;
				}

				if (nScore > nBestScore)
				{
					nBestScore = nScore;
					if (nScore > nAlpha)
					{
						nAlpha = nScore;
						if (nAlpha >= nBeta)
						{
							break;
						}
					}
				}
			}

			if (bInCheck && (nLegalMoves == 0))
			{
				return -MATE_SCORE + nPly;
			}
			return nBestScore;
		}

		int Searcher::Search(int nDepth, int nAlpha, int nBeta, const int nPly, const bool bAllowNull, const bool bInCheck)
		{
			// A check is answered before the search stops, which finds the checkmates at the end of a line (a bounded number of plies are added)
			if (bInCheck && (nPly < m_nExtensionLimit))
			{
				++nDepth;
			}
			if ((nDepth <= 0) || (nPly >= MAX_PLY - 1))
			{
				return Quiesce(nAlpha, nBeta, nPly, bInCheck);
			}

			++m_nNodes;
			if ((m_nNodes & 1023) == 0)
			{
				PollStop();
			}
			if (m_bStopped)
			{
				return 0;
			}

			// A position that comes again in the line is a draw: the player that is better does not go round in circles, and the other one may
			const uint64_t nKey = m_position.Hash();
			m_path[nPly] = nKey;
			for (int nBack = nPly - 2; nBack >= m_nRepeatFrom[nPly]; nBack -= 2)
			{
				if (m_path[nBack] == nKey)
				{
					return 0;
				}
			}

			const bool bPrincipal = (nBeta - nAlpha) > 1;

			// No line can be better than a checkmate given right now, or worse than being checkmated right now
			nAlpha = (std::max)(nAlpha, -MATE_SCORE + nPly);
			nBeta = (std::min)(nBeta, MATE_SCORE - nPly - 1);
			if (nAlpha >= nBeta)
			{
				return nAlpha;
			}

			TableEntry& entry = m_table[static_cast<size_t>(nKey & m_nTableMask)];
			Move tableMove;
			if ((entry.nBound != BOUND_NONE) && (entry.nKey == nKey))
			{
				tableMove = DecodeMove(entry.nMove);
				if (!bPrincipal && (entry.nDepth >= nDepth))
				{
					const int nTableScore = ScoreFromTable(entry.nScore, nPly);
					if ((entry.nBound == BOUND_EXACT) ||
						((entry.nBound == BOUND_LOWER) && (nTableScore >= nBeta)) ||
						((entry.nBound == BOUND_UPPER) && (nTableScore <= nAlpha)))
					{
						return nTableScore;
					}
				}
			}

			const bool bWhite = m_position.IsWhiteToMove();
			if (bAllowNull && !bPrincipal && !bInCheck && (nDepth >= 3) && m_position.HasPieces(bWhite) && (m_position.Evaluate() >= nBeta))
			{
				Undo undo;
				m_position.MakeNullMove(undo);
				m_nRepeatFrom[nPly + 1] = nPly + 1;
				const bool bPassAllowed = !m_position.IsInCheck();
				int nNullScore = -INFINITE_SCORE;
				if (bPassAllowed)
				{
					nNullScore = -Search(nDepth - 3 - (nDepth >= 6 ? 1 : 0), -nBeta, -nBeta + 1, nPly + 1, false, false);
				}
				m_position.UnmakeNullMove(undo);
				if (m_bStopped)
				{
					return 0;
				}
				if (bPassAllowed && (nNullScore >= nBeta))
				{
					return (nNullScore >= MATE_BOUND) ? nBeta : nNullScore;
				}
			}

			Move moves[MAX_MOVES];
			int nScores[MAX_MOVES];
			const int nMoves = m_position.GenerateMoves(moves, false);
			for (int nMove = 0; nMove < nMoves; ++nMove)
			{
				nScores[nMove] = ScoreMove(moves[nMove], tableMove, nPly);
			}

			const int nOriginalAlpha = nAlpha;
			int nBestScore = -INFINITE_SCORE;
			Move bestMove;
			int nLegalMoves = 0;
			for (int nMove = 0; nMove < nMoves; ++nMove)
			{
				PickMove(moves, nScores, nMove, nMoves);
				const Move move = moves[nMove];

				Undo undo;
				if (!m_position.MakeMove(move, undo))
				{
					continue;
				}
				++nLegalMoves;
				EnterLine(move, undo, nPly);

				const bool bCapture = (undo.nCaptured != 0);
				const bool bGivesCheck = m_position.IsInCheck();
				int nScore;
				if (nLegalMoves == 1)
				{
					nScore = -Search(nDepth - 1, -nBeta, -nAlpha, nPly + 1, true, bGivesCheck);
				}
				else
				{
					int nReduction = 0;
					if ((nDepth >= 3) && (nLegalMoves > 3) && !bCapture && !bInCheck && !bGivesCheck &&
						(move != m_killers[nPly][0]) && (move != m_killers[nPly][1]))
					{
						nReduction = ((nDepth >= 6) && (nLegalMoves > 8)) ? 2 : 1;
					}

					nScore = -Search(nDepth - 1 - nReduction, -nAlpha - 1, -nAlpha, nPly + 1, true, bGivesCheck);
					if ((nScore > nAlpha) && (nReduction > 0))
					{
						nScore = -Search(nDepth - 1, -nAlpha - 1, -nAlpha, nPly + 1, true, bGivesCheck);
					}
					if ((nScore > nAlpha) && (nScore < nBeta))
					{
						nScore = -Search(nDepth - 1, -nBeta, -nAlpha, nPly + 1, true, bGivesCheck);
					}
				}
				m_position.UnmakeMove(move, undo);
				if (m_bStopped)
				{
					return 0;
				}

				if (nScore > nBestScore)
				{
					nBestScore = nScore;
					bestMove = move;
					if (nScore > nAlpha)
					{
						nAlpha = nScore;
						if (nAlpha >= nBeta)
						{
							if (!bCapture)
							{
								RecordCutoff(move, nDepth, nPly);
							}
							break;
						}
					}
				}
			}

			if (nLegalMoves == 0)
			{
				return bInCheck ? (-MATE_SCORE + nPly) : 0;
			}

			if ((entry.nKey != nKey) || (entry.nBound == BOUND_NONE) || (nDepth >= entry.nDepth))
			{
				const Move keptMove = (bestMove.nFrom >= 0) ? bestMove : (entry.nKey == nKey ? DecodeMove(entry.nMove) : bestMove);
				entry.nKey = nKey;
				entry.nScore = static_cast<int16_t>(ScoreToTable(nBestScore, nPly));
				entry.nMove = EncodeMove(keptMove);
				entry.nDepth = static_cast<int8_t>(nDepth);
				if (nBestScore >= nBeta)
				{
					entry.nBound = BOUND_LOWER;
				}
				else
				{
					entry.nBound = (nBestScore > nOriginalAlpha) ? BOUND_EXACT : BOUND_UPPER;
				}
			}
			return nBestScore;
		}

		Result Searcher::Run(const std::vector<Move>& rootMoves)
		{
			Result result;
			if ((m_limits.pbContinue != nullptr) && !*m_limits.pbContinue)
			{
				result.bStopped = true;
				return result;
			}

			// The moves to choose from are the legal moves of the position, or the ones of rootMoves among them
			std::vector<RootMove> root;
			{
				Move moves[MAX_MOVES];
				const int nMoves = m_position.GenerateMoves(moves, false);
				const Move noMove;
				for (int nMove = 0; nMove < nMoves; ++nMove)
				{
					if (!rootMoves.empty() && (std::find(rootMoves.begin(), rootMoves.end(), moves[nMove]) == rootMoves.end()))
					{
						continue;
					}

					Undo undo;
					if (m_position.MakeMove(moves[nMove], undo))
					{
						m_position.UnmakeMove(moves[nMove], undo);
						RootMove rootMove;
						rootMove.move = moves[nMove];
						rootMove.nOrder = ScoreMove(moves[nMove], noMove, 0);
						root.push_back(rootMove);
					}
				}
			}
			if (root.empty())
			{
				return result;
			}

			// The best move of an iteration is the first one of the next, then the others in the order of their scores
			const auto byOrder = [](const RootMove& first, const RootMove& second)
			{
				return first.nOrder > second.nOrder;
			};
			std::stable_sort(root.begin(), root.end(), byOrder);

			m_path[0] = m_position.Hash();
			m_nRepeatFrom[0] = 0;

			for (int nDepth = 1; nDepth <= m_nMaxDepth; ++nDepth)
			{
				m_nExtensionLimit = 2 * nDepth;
				int nAlpha = -INFINITE_SCORE;
				int nBestScore = -INFINITE_SCORE;
				int nBestIndex = -1;

				for (int nIndex = 0; nIndex < static_cast<int>(root.size()); ++nIndex)
				{
					const Move move = root[nIndex].move;
					Undo undo;
					if (!m_position.MakeMove(move, undo))
					{
						continue;
					}

					EnterLine(move, undo, 0);
					const bool bGivesCheck = m_position.IsInCheck();
					int nScore;
					if (nBestIndex < 0)
					{
						nScore = -Search(nDepth - 1, -INFINITE_SCORE, -nAlpha, 1, true, bGivesCheck);
					}
					else
					{
						nScore = -Search(nDepth - 1, -nAlpha - 1, -nAlpha, 1, true, bGivesCheck);
						if (!m_bStopped && (nScore > nAlpha))
						{
							nScore = -Search(nDepth - 1, -INFINITE_SCORE, -nAlpha, 1, true, bGivesCheck);
						}
					}
					m_position.UnmakeMove(move, undo);
					if (m_bStopped)
					{
						break;
					}

					root[nIndex].nOrder = nScore;
					if (nScore > nBestScore)
					{
						nBestScore = nScore;
						nBestIndex = nIndex;
						nAlpha = nScore;
					}
				}

				if (m_bStopped)
				{
					result.nNodes = m_nNodes;
					if (m_bUserStop)
					{
						result.bFound = false;
						result.bStopped = true;
						return result;
					}
					if (nBestIndex >= 0)
					{
						// The first move of the iteration is the best of the previous one: a move that has replaced it is better
						result.bFound = true;
						result.move = root[nBestIndex].move;
						result.nScore = nBestScore;
					}
					break;
				}

				if (nBestIndex < 0)
				{
					break;
				}
				result.bFound = true;
				result.move = root[nBestIndex].move;
				result.nScore = nBestScore;
				result.nDepth = nDepth;

				std::stable_sort(root.begin(), root.end(), byOrder);
				if ((root.size() == 1) || (std::abs(nBestScore) >= MATE_SCORE - nDepth))
				{
					break;
				}
			}

			if (!result.bFound)
			{
				// The limit of positions was reached before the first move was searched: any legal move will do
				result.bFound = true;
				result.move = root[0].move;
			}
			result.nNodes = m_nNodes;
			return result;
		}
	}

	Result FindBestMove(const Position& position, const std::vector<Move>& rootMoves, const Limits& limits)
	{
		const std::unique_ptr<Searcher> pSearcher(new Searcher(position, limits));
		return pSearcher->Run(rootMoves);
	}
}
