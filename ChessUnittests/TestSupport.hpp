// TestSupport.hpp - helpers shared by the ChessUnittests test classes
//
// A position is written as the piece placement field of a FEN record: ranks 8 down to 1 separated by '/',
// uppercase letters for White and lowercase letters for Black (P N B R Q K) and digits for runs of empty squares.
// Example: L"4k3/8/8/8/8/8/8/R3K3" is a White rook and a White king against a Black king.
// The board keeps empty squares out of its map, so an empty square is simply a square without a piece.
// Every piece made from a letter is counted while it is alive (see LivePieces), and so is every clone the engine makes of it.

#pragma once

#include "ChessBoard.hpp"
#include "ChessSearch.hpp"
#include "CppUnitTest.h"

#include <cwctype>
#include <memory>
#include <set>
#include <sstream>
#include <string>

// CppUnitTest.h does not do it: every test file uses Assert and the test macros unqualified
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChessTests
{
	// Piece placement of the position a new game starts from
	inline constexpr const wchar_t* INITIAL_POSITION = L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";

	// What Piece::isValidMove() returns for a valid move
	inline constexpr int VALID = ChessErrHandler::CHESS_NO_ERROR;

	inline constexpr int FILES_PER_RANK = ChessInfo::MAX_FILE - ChessInfo::MIN_FILE + 1;

	typedef std::unique_ptr<Piece> PiecePtr;

	inline std::wstring Square(wchar_t file, wchar_t rank)
	{
		return std::wstring{ file, rank };
	}

	// Frees a board together with the pieces on it
	inline void FreeBoard(Board* board)
	{
		for (Board::iterator it = board->begin(); it != board->end(); ++it)
		{
			delete it->second;
		}
		delete board;
	}

	// The number of counted pieces that are alive: a test that makes the engine try a move can compare it before and after
	// to see that the engine freed everything it cloned
	inline int& LivePieces()
	{
		static int count = 0;
		return count;
	}

	template<class PieceType>
	class Counted : public PieceType
	{
	public:
		explicit Counted(bool white)
			: PieceType(white)
		{
			++LivePieces();
		}

		~Counted() override
		{
			--LivePieces();
		}

		// A clone of a counted piece is counted too, it is made the way the clone of the piece itself is
		PieceType* clone() override
		{
			return new Counted(this->isWhitePlayer());
		}
	};

	inline Piece* NewPiece(wchar_t letter)
	{
		const bool isWhite = std::iswupper(letter) != 0;
		switch (std::towlower(letter))
		{
			case L'p': return new Counted<Pawn>(isWhite);
			case L'n': return new Counted<Knight>(isWhite);
			case L'b': return new Counted<Bishop>(isWhite);
			case L'r': return new Counted<Rook>(isWhite);
			case L'q': return new Counted<Queen>(isWhite);
			case L'k': return new Counted<King>(isWhite);
			default: return nullptr;
		}
	}

	// The letter a piece is written with in a position, uppercase for White
	inline wchar_t LetterOf(Piece* piece)
	{
		wchar_t letter = L'?';
		if (dynamic_cast<Pawn*>(piece) != nullptr)
		{
			letter = L'p';
		}
		else if (dynamic_cast<Knight*>(piece) != nullptr)
		{
			letter = L'n';
		}
		else if (dynamic_cast<Bishop*>(piece) != nullptr)
		{
			letter = L'b';
		}
		else if (dynamic_cast<Rook*>(piece) != nullptr)
		{
			letter = L'r';
		}
		else if (dynamic_cast<Queen*>(piece) != nullptr)
		{
			letter = L'q';
		}
		else if (dynamic_cast<King*>(piece) != nullptr)
		{
			letter = L'k';
		}
		return piece->isWhitePlayer() ? static_cast<wchar_t>(std::towupper(letter)) : letter;
	}

	/* Builds a board from a position. A pawn knows whether it has already moved, so the pawns that are not on their
	   starting rank are marked as moved, as they would be after a real game: only the others may advance two squares.
	*/
	inline Board* ParseBoard(const wchar_t* placement)
	{
		const std::wstring description = std::wstring(L"Invalid position: ") + placement;
		std::unique_ptr<Board, void (*)(Board*)> board(new Board(), &FreeBoard);

		wchar_t rank = ChessInfo::MAX_RANK;
		int file = 0;
		for (const wchar_t* next = placement; *next != L'\0'; ++next)
		{
			if (*next == L'/')
			{
				--rank;
				file = 0;
			}
			else if ((*next >= L'1') && (*next <= L'8'))
			{
				file += *next - L'0';
			}
			else
			{
				Assert::IsTrue((file < FILES_PER_RANK) && (rank >= ChessInfo::MIN_RANK), description.c_str());
				Piece* piece = NewPiece(*next);
				Assert::IsNotNull(piece, description.c_str());

				if (dynamic_cast<Pawn*>(piece) != nullptr)
				{
					wchar_t initialRank = ChessInfo::BLACK_INIT_PAWN_RANK;
					if (piece->isWhitePlayer())
					{
						initialRank = ChessInfo::WHITE_INIT_PAWN_RANK;
					}
					if (rank != initialRank)
					{
						piece->confirmMove();
					}
				}

				board->insert({ Square(static_cast<wchar_t>(ChessInfo::MIN_FILE + file), rank), piece });
				++file;
			}
		}
		return board.release();
	}

	// The position of a board, written the way ParseBoard() reads it
	inline std::wstring Placement(const Board& board)
	{
		std::wstring placement;
		for (wchar_t rank = ChessInfo::MAX_RANK; rank >= ChessInfo::MIN_RANK; --rank)
		{
			int emptySquares = 0;
			for (wchar_t file = ChessInfo::MIN_FILE; file <= ChessInfo::MAX_FILE; ++file)
			{
				const Board::const_iterator found = board.find(Square(file, rank));
				if (found == board.end())
				{
					++emptySquares;
					continue;
				}
				if (emptySquares > 0)
				{
					placement += static_cast<wchar_t>(L'0' + emptySquares);
					emptySquares = 0;
				}
				placement += LetterOf(found->second);
			}
			if (emptySquares > 0)
			{
				placement += static_cast<wchar_t>(L'0' + emptySquares);
			}
			if (rank > ChessInfo::MIN_RANK)
			{
				placement += L'/';
			}
		}
		return placement;
	}

	// A board built from a position that is freed together with its pieces
	class Position
	{
	public:
		explicit Position(const wchar_t* placement)
			: m_pBoard(ParseBoard(placement))
		{
		}

		~Position()
		{
			FreeBoard(m_pBoard);
		}

		Position(const Position&) = delete;
		Position& operator=(const Position&) = delete;

		Board* Get()
		{
			return m_pBoard;
		}

		// The piece standing on a square, which the test expects to be there
		Piece* At(const wchar_t* square)
		{
			const Board::iterator found = m_pBoard->find(square);
			Assert::IsTrue(found != m_pBoard->end(), (std::wstring(L"There is no piece on ") + square).c_str());
			return found->second;
		}

	private:
		Board* m_pBoard;
	};

	// Squares the piece standing on `from` may move to on the board, sorted and separated by spaces: L"B3 C2"
	inline std::wstring Destinations(Piece* piece, const wchar_t* from, Board* board)
	{
		std::set<std::wstring> squares;
		for (wchar_t file = ChessInfo::MIN_FILE; file <= ChessInfo::MAX_FILE; ++file)
		{
			for (wchar_t rank = ChessInfo::MIN_RANK; rank <= ChessInfo::MAX_RANK; ++rank)
			{
				const std::wstring to = Square(file, rank);
				if (piece->isValidMove(from, to, board) == ChessErrHandler::CHESS_NO_ERROR)
				{
					squares.insert(to);
				}
			}
		}

		std::wstring list;
		for (const std::wstring& square : squares)
		{
			if (!list.empty())
			{
				list += L' ';
			}
			list += square;
		}
		return list;
	}

	inline int CountDestinations(Piece* piece, const wchar_t* from, Board* board)
	{
		const std::wstring list = Destinations(piece, from, board);
		return list.empty() ? 0 : static_cast<int>((list.length() + 1) / 3);
	}

	inline const wchar_t* ErrorName(int code)
	{
		switch (code)
		{
			case ChessErrHandler::CHESS_NO_ERROR: return L"CHESS_NO_ERROR";
			case ChessErrHandler::DEST_EQ_SOURCE: return L"DEST_EQ_SOURCE";
			case ChessErrHandler::SOURCE_OUTOF_BOUND: return L"SOURCE_OUTOF_BOUND";
			case ChessErrHandler::DEST_OUTOF_BOUND: return L"DEST_OUTOF_BOUND";
			case ChessErrHandler::INVALID_FILE_RANK: return L"INVALID_FILE_RANK";
			case ChessErrHandler::MOVED_EMPTY_PIECE: return L"MOVED_EMPTY_PIECE";
			case ChessErrHandler::NOT_OWNER_TURN: return L"NOT_OWNER_TURN";
			case ChessErrHandler::ILLEGAL_MOVE_PATTERN: return L"ILLEGAL_MOVE_PATTERN";
			case ChessErrHandler::OBSTRUCTION_EN_ROUTE: return L"OBSTRUCTION_EN_ROUTE";
			case ChessErrHandler::FRIENDLY_AT_DEST: return L"FRIENDLY_AT_DEST";
			case ChessErrHandler::PAWN_ILLEGAL_CAPTURE_PATTERN: return L"PAWN_ILLEGAL_CAPTURE_PATTERN";
			case ChessErrHandler::ALLOW_KING_IN_CHECK: return L"ALLOW_KING_IN_CHECK";
			case ChessErrHandler::GAME_HAS_ENDED: return L"GAME_HAS_ENDED";
			default: return L"(unknown error code)";
		}
	}

	// The piece standing on `from` must answer `expected` (VALID or an error code) for a move to `to`
	inline void AssertMove(Piece* piece, const wchar_t* from, const wchar_t* to, Board* board, int expected,
		const __LineInfo* pLineInfo = nullptr)
	{
		const int actual = piece->isValidMove(from, to, board);
		if (actual != expected)
		{
			std::wstringstream message;
			message << L"Move " << from << L"-" << to << L" should give " << ErrorName(expected) << L" (" << expected
				<< L") but gives " << ErrorName(actual) << L" (" << actual << L")";
			Assert::Fail(message.str().c_str(), pLineInfo);
		}
	}

	// Replaces the pieces of a game by a custom position, the player to move does not change
	inline void SetPosition(ChessBoard& game, const wchar_t* placement)
	{
		Board* board = ParseBoard(placement);
		FreeBoard(game._board);
		game._board = board;
	}

	inline void AssertPlacement(const wchar_t* expected, ChessBoard& game, const __LineInfo* pLineInfo = nullptr)
	{
		Assert::AreEqual(std::wstring(expected), Placement(*game._board), L"Position of the game", pLineInfo);
	}

	// Submits a move, true when the engine accepts it: an accepted move always passes the turn to the opponent
	inline bool Submit(ChessBoard& game, const wchar_t* from, const wchar_t* to)
	{
		const bool wasWhiteTurn = game.IsWhiteTurn();
		game.submitMove(from, to);
		return game.IsWhiteTurn() != wasWhiteTurn;
	}

	// Submits a move that has to be accepted
	inline void Play(ChessBoard& game, const wchar_t* from, const wchar_t* to)
	{
		if (!Submit(game, from, to))
		{
			const std::wstring message = std::wstring(L"Move ") + from + L"-" + to + L" was rejected in position " +
				Placement(*game._board);
			Assert::Fail(message.c_str());
		}
	}

	// Submits the moves of a list like L"E2E4 E7E5 G1F3", every one of them has to be accepted
	inline void PlayMoves(ChessBoard& game, const wchar_t* moves)
	{
		std::wistringstream list(moves);
		std::wstring move;
		while (list >> move)
		{
			Assert::IsTrue(move.length() == 4, (std::wstring(L"A move is written as 4 characters: ") + move).c_str());
			Play(game, move.substr(0, 2).c_str(), move.substr(2, 2).c_str());
		}
	}

	// Submits a move that has to be rejected: the position, the player to move and the end of the game stay as they were
	inline void AssertRejected(ChessBoard& game, const wchar_t* from, const wchar_t* to, const __LineInfo* pLineInfo = nullptr)
	{
		const std::wstring before = Placement(*game._board);
		const bool wasWhiteTurn = game.IsWhiteTurn();
		const bool hadEnded = game.HasGameEnded();

		game.submitMove(from, to);

		const std::wstring move = std::wstring(L"Move ") + from + L"-" + to;
		Assert::IsTrue(game.IsWhiteTurn() == wasWhiteTurn, (move + L" was accepted, it passed the turn").c_str(), pLineInfo);
		Assert::AreEqual(before, Placement(*game._board), (move + L" changed the position").c_str(), pLineInfo);
		Assert::IsTrue(game.HasGameEnded() == hadEnded, (move + L" changed the end of the game").c_str(), pLineInfo);
	}

	// The name of a square of the search, like L"E2": the search numbers the squares from A8 (0) to H1 (63)
	inline std::wstring SearchSquareName(int square)
	{
		return Square(static_cast<wchar_t>(ChessInfo::MIN_FILE + (square % FILES_PER_RANK)),
			static_cast<wchar_t>(ChessInfo::MAX_RANK - (square / FILES_PER_RANK)));
	}

	inline int SearchSquareOf(const wchar_t* name)
	{
		return (ChessInfo::MAX_RANK - name[1]) * FILES_PER_RANK + (name[0] - ChessInfo::MIN_FILE);
	}

	// A move of the search written as the square it starts from and the one it ends on, like L"E2E4"
	inline std::wstring SearchMoveName(const ChessSearch::Move& move)
	{
		return SearchSquareName(move.nFrom) + SearchSquareName(move.nTo);
	}

	inline ChessSearch::Move SearchMoveOf(const wchar_t* name)
	{
		ChessSearch::Move move;
		move.nFrom = SearchSquareOf(name);
		move.nTo = SearchSquareOf(name + 2);
		return move;
	}

	// The position the search works on for a placement: the pieces that ParseBoard() puts on a board, and the player to move
	inline ChessSearch::Position ParseSearchPosition(const wchar_t* placement, bool whiteToMove)
	{
		const std::wstring description = std::wstring(L"Invalid position: ") + placement;
		ChessSearch::Position position;
		int square = 0;
		for (const wchar_t* pLetter = placement; *pLetter != L'\0'; ++pLetter)
		{
			const wchar_t letter = *pLetter;
			if (letter == L'/')
			{
				continue;
			}
			if ((letter >= L'1') && (letter <= L'8'))
			{
				square += letter - L'0';
				continue;
			}

			int kind = ChessSearch::NO_PIECE;
			switch (std::towlower(letter))
			{
				case L'p': kind = ChessSearch::PAWN; break;
				case L'n': kind = ChessSearch::KNIGHT; break;
				case L'b': kind = ChessSearch::BISHOP; break;
				case L'r': kind = ChessSearch::ROOK; break;
				case L'q': kind = ChessSearch::QUEEN; break;
				case L'k': kind = ChessSearch::KING; break;
				default: break;
			}
			Assert::IsTrue((kind != ChessSearch::NO_PIECE) && (square < ChessSearch::SQUARE_COUNT), description.c_str());
			position.Put(square, kind, std::iswupper(letter) != 0);
			++square;
		}
		Assert::AreEqual(ChessSearch::SQUARE_COUNT, square, description.c_str());
		position.SetWhiteToMove(whiteToMove);
		return position;
	}

	// True when two positions have the same piece on every square, the same player to move and so the same hash
	inline bool SameSearchPosition(const ChessSearch::Position& first, const ChessSearch::Position& second)
	{
		if ((first.Hash() != second.Hash()) || (first.IsWhiteToMove() != second.IsWhiteToMove()))
		{
			return false;
		}
		for (int square = 0; square < ChessSearch::SQUARE_COUNT; ++square)
		{
			if (first.PieceAt(square) != second.PieceAt(square))
			{
				return false;
			}
		}
		return true;
	}
}
