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

#ifndef CHESSSEARCH_H
#define CHESSSEARCH_H

#include <cstdint>
#include <vector>

/* The search for the best move of a player, which the computer player (ComputerPlayer.cpp) uses.

   The engine keeps a board as a map of file & rank to Piece, which is far too slow to be searched deeply. A Position is a
   compact copy of a board, with 64 squares numbered the way they are written in a FEN record (A8 is 0, H8 is 7, A1 is 56
   and H1 is 63), and the search runs on it without ever touching the board of the game.

   A Position moves its pieces by the rules of the engine, which are the rules of chess without castling, en passant and
   promotion: a pawn that reaches the last rank stays there and cannot move any more, and a pawn advances two squares only
   from its starting rank. A king is never captured, the player who attacks it has to be answered instead.
*/
namespace ChessSearch
{
	constexpr int SQUARE_COUNT = 64;

	// The most moves a position can have: the move generator never writes more than this
	constexpr int MAX_MOVES = 256;

	// The deepest line of the search, in plies from the position it starts from
	constexpr int MAX_PLY = 64;

	// The score of the player who gives checkmate on the next ply, a checkmate that is N plies away scores MATE_SCORE - N
	constexpr int MATE_SCORE = 30000;
	constexpr int MATE_BOUND = MATE_SCORE - MAX_PLY;

	// The kind of a piece, a piece of Black is stored as the negative of its kind
	enum PieceKind
	{
		NO_PIECE = 0,
		PAWN,
		KNIGHT,
		BISHOP,
		ROOK,
		QUEEN,
		KING
	};

	struct Move
	{
		int nFrom = -1;
		int nTo = -1;

		bool operator==(const Move& other) const
		{
			return (nFrom == other.nFrom) && (nTo == other.nTo);
		}

		bool operator!=(const Move& other) const
		{
			return !(*this == other);
		}
	};

	// What a move changes, which gives the position back as it was when the move is taken back
	struct Undo
	{
		int nCaptured = 0;
		int nKingSquare[2] = { -1, -1 };
		uint64_t nHash = 0;
	};

	class Position
	{
	public:
		Position();

		void Clear();

		/* Puts a piece on a square, in place of the piece that is already there. The first king put for a player is the one
		   that player has to keep safe: a player without a king is never in check.
		*/
		void Put(int nSquare, int nKind, bool bWhite);

		void SetWhiteToMove(bool bWhiteToMove);

		bool IsWhiteToMove() const
		{
			return m_bWhiteToMove;
		}

		// The piece on a square: 0 when it is empty, the kind of the piece for White and its negative for Black
		int PieceAt(const int nSquare) const
		{
			return m_board[nSquare];
		}

		// Identifies the pieces on the squares and the player to move, it is kept up to date by the moves
		uint64_t Hash() const
		{
			return m_nHash;
		}

		/* Writes the moves of the player to move in pMoves, which holds MAX_MOVES moves, and returns their number. They are
		   the moves of the pieces, which can leave the king of the player attacked (MakeMove() refuses them). With
		   bCapturesOnly only the captures are written.
		*/
		int GenerateMoves(Move* pMoves, bool bCapturesOnly) const;

		// The moves of the player to move that keep its king safe
		std::vector<Move> LegalMoves() const;

		// Plays a move of the player to move. It is taken back at once and false is returned if it leaves its king attacked.
		bool MakeMove(const Move& move, Undo& undo);

		void UnmakeMove(const Move& move, const Undo& undo);

		// Gives the turn to the opponent without moving
		void MakeNullMove(Undo& undo);

		void UnmakeNullMove(const Undo& undo);

		// True when a piece of the player can capture on the square
		bool IsAttacked(int nSquare, bool bByWhite) const;

		// True when the king of the player to move is attacked
		bool IsInCheck() const;

		// True when the player has a piece other than pawns and the king
		bool HasPieces(bool bWhite) const;

		// The value of the position for the player to move, in centipawns
		int Evaluate() const;

		// The number of lines of nDepth plies that start from the position
		uint64_t Perft(int nDepth) const;

	private:
		void Restore(const Move& move, const Undo& undo);
		uint64_t PerftOfLine(int nDepth);

		int m_board[SQUARE_COUNT];
		bool m_bWhiteToMove;
		int m_nKingSquare[2];
		uint64_t m_nHash;
	};

	struct Limits
	{
		// The deepest iteration of the search, in plies: the search deepens one ply at a time up to it
		int nMaxDepth = 1;

		/* The number of positions after which the search stops, 0 for no limit. The limit is checked every 1024 positions, so
		   the search may go slightly over it. The best move is then the one of the last iteration that completed, or a
		   better one that the iteration in progress has already found. As the search counts positions and not time, the
		   same limit always gives the same move.
		*/
		uint64_t nMaxNodes = 0;

		// The search stops and gives nothing as soon as the value that this points to is false
		const volatile bool* pbContinue = nullptr;
	};

	struct Result
	{
		// True when the search chose a move
		bool bFound = false;

		// True when the search was stopped (it did not choose a move)
		bool bStopped = false;

		Move move;

		// The value of the move for the player who plays it, in centipawns or a score of checkmate
		int nScore = 0;

		// The depth of the last iteration that completed, in plies
		int nDepth = 0;

		uint64_t nNodes = 0;
	};

	/* Finds the best move of the player to move. The search takes the moves to choose from, which have to be moves of the
	   player to move, or all its legal moves when the list is empty. It is a pure function of its arguments, so the same
	   position gives the same move every time, and it only gives a move that keeps the king of the player safe.
	*/
	Result FindBestMove(const Position& position, const std::vector<Move>& rootMoves, const Limits& limits);
}

#endif
