// ComputerPlayerTests.cpp - unit tests of the search of the computer player and of the thread procedure that plays its move

#include "pch.h"
#include "TestSupport.hpp"
#include "ChessSearch.hpp"

#include <map>
#include <set>

// The search of the computer player is defined in ComputerPlayer.cpp and is not declared in a header
extern bool g_bThreadRunning;
bool WhitePlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, std::wstring& strMoveFrom, std::wstring& strMoveTo, Board* pBoard);
bool BlackPlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, std::wstring& strMoveFrom, std::wstring& strMoveTo, Board* pBoard);
DWORD WINAPI ComputerThreadProc(LPVOID lpParam);
bool BoardToPosition(Board* pBoard, const bool bWhiteToMove, ChessSearch::Position& position);

using namespace ChessTests;

namespace
{
	// The search only runs while g_bThreadRunning is set, which ComputerThreadProc() clears when it is done:
	// this sets the flag for the lifetime of the object and gives the previous value back
	class SearchFlag
	{
	public:
		explicit SearchFlag(bool running = true)
			: m_bPrevious(g_bThreadRunning)
		{
			g_bThreadRunning = running;
		}

		~SearchFlag()
		{
			g_bThreadRunning = m_bPrevious;
		}

		SearchFlag(const SearchFlag&) = delete;
		SearchFlag& operator=(const SearchFlag&) = delete;

	private:
		bool m_bPrevious;
	};

	typedef std::map<std::wstring, wchar_t> Letters;

	// The letter of the piece standing on every occupied square
	Letters LettersOf(const Board& board)
	{
		Letters letters;
		for (Board::const_iterator it = board.begin(); it != board.end(); ++it)
		{
			letters[it->first] = LetterOf(it->second);
		}
		return letters;
	}

	Letters WhiteLetters(const Letters& letters)
	{
		Letters white;
		for (const Letters::value_type& entry : letters)
		{
			if (std::iswupper(entry.second) != 0)
			{
				white.insert(entry);
			}
		}
		return white;
	}

	// The number of squares that hold another piece (or none) in the second position, a quiet move changes two squares
	int CountDifferences(const Letters& before, const Letters& after)
	{
		std::set<std::wstring> squares;
		for (const Letters::value_type& entry : before)
		{
			squares.insert(entry.first);
		}
		for (const Letters::value_type& entry : after)
		{
			squares.insert(entry.first);
		}

		int differences = 0;
		for (const std::wstring& square : squares)
		{
			const Letters::const_iterator was = before.find(square);
			const Letters::const_iterator is = after.find(square);
			const wchar_t wasLetter = (was == before.end()) ? L' ' : was->second;
			const wchar_t isLetter = (is == after.end()) ? L' ' : is->second;
			if (wasLetter != isLetter)
			{
				++differences;
			}
		}
		return differences;
	}

	// A position with a few pieces of every kind, where both players have captures to choose from
	const wchar_t* const MIDDLE_GAME = L"r3k2r/pppq1ppp/2n2n2/3pp3/3PP3/2N2N2/PPPQ1PPP/R3K2R";

	// The letter of the piece standing on a square, a space when there is none
	wchar_t LetterOn(const Letters& letters, const wchar_t* square)
	{
		const Letters::const_iterator found = letters.find(square);
		return (found == letters.end()) ? L' ' : found->second;
	}

	/* True when the engine accepts the move of the player to move in the position: the move is legal, which is more than
	   being a valid move of the piece because it must not leave the king of the player in check.
	   A new game starts with White to move, a first move of White (which the position replaces) gives the turn to Black.
	*/
	bool EngineAccepts(const wchar_t* placement, bool whiteToMove, const std::wstring& from, const std::wstring& to)
	{
		ChessBoard game;
		if (!whiteToMove)
		{
			Play(game, L"G1", L"F3");
		}
		SetPosition(game, placement);
		Assert::AreEqual(whiteToMove, game.IsWhiteTurn());
		return Submit(game, from.c_str(), to.c_str());
	}

	// The moves of a set separated by spaces, like L"E2E3 E2E4"
	std::wstring Join(const std::set<std::wstring>& moves)
	{
		std::wstring list;
		for (const std::wstring& move : moves)
		{
			if (!list.empty())
			{
				list += L' ';
			}
			list += move;
		}
		return list;
	}

	/* The moves of the player to move that the game accepts, like L"E2E4". The game accepts a move that its piece finds
	   valid and that keeps the king of the player safe, and it only tells by accepting it: every valid move is tried.
	*/
	std::set<std::wstring> AcceptedMoves(Board* board, bool whiteToMove)
	{
		const std::wstring placement = Placement(*board);
		std::set<std::wstring> accepted;
		for (Board::const_iterator it = board->begin(); it != board->end(); ++it)
		{
			if (it->second->isWhitePlayer() != whiteToMove)
			{
				continue;
			}
			for (wchar_t file = ChessInfo::MIN_FILE; file <= ChessInfo::MAX_FILE; ++file)
			{
				for (wchar_t rank = ChessInfo::MIN_RANK; rank <= ChessInfo::MAX_RANK; ++rank)
				{
					const std::wstring to = Square(file, rank);
					if ((to != it->first) && (it->second->isValidMove(it->first, to, board) == VALID) &&
						EngineAccepts(placement.c_str(), whiteToMove, it->first, to))
					{
						accepted.insert(it->first + to);
					}
				}
			}
		}
		return accepted;
	}

	// True when the player who is not to move has its king attacked: a game never reaches such a position
	bool OpponentIsInCheck(const ChessSearch::Position& position)
	{
		ChessSearch::Position passed(position);
		ChessSearch::Undo undo;
		passed.MakeNullMove(undo);
		return passed.IsInCheck();
	}

	/* Compares the moves that the search generates in the position of a board with the moves that the game accepts. They
	   have to be the same: a move that the game refuses would freeze it, and a move that the search does not have would
	   never be played. Gives the moves.
	*/
	std::set<std::wstring> AssertSameMoves(Board* board, bool whiteToMove, const std::wstring& where)
	{
		ChessSearch::Position position;
		Assert::IsTrue(BoardToPosition(board, whiteToMove, position), (where + L": the board cannot be searched").c_str());

		std::set<std::wstring> generated;
		for (const ChessSearch::Move& move : position.LegalMoves())
		{
			generated.insert(SearchMoveName(move));
		}

		const std::set<std::wstring> accepted = AcceptedMoves(board, whiteToMove);
		if (generated != accepted)
		{
			Assert::Fail((where + L": the game accepts " + Join(accepted) + L" but the search generates " + Join(generated)).c_str());
		}
		return accepted;
	}
}

namespace ChessUnittests
{
	TEST_CLASS(ComputerSearchTests)
	{
	public:
		TEST_METHOD(Black_ChoosesAValidMoveOfItsOwnPiece)
		{
			SearchFlag searching;
			Position position(INITIAL_POSITION);
			int score = 0;
			std::wstring from, to;

			Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));

			Piece* piece = position.At(from.c_str());
			Assert::IsFalse(piece->isWhitePlayer());
			AssertMove(piece, from.c_str(), to.c_str(), position.Get(), VALID, LINE_INFO());
		}

		TEST_METHOD(White_ChoosesAValidMoveOfItsOwnPiece)
		{
			SearchFlag searching;
			Position position(INITIAL_POSITION);
			int score = 0;
			std::wstring from, to;

			Assert::IsTrue(WhitePlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));

			Piece* piece = position.At(from.c_str());
			Assert::IsTrue(piece->isWhitePlayer());
			AssertMove(piece, from.c_str(), to.c_str(), position.Get(), VALID, LINE_INFO());
		}

		TEST_METHOD(TheSearchLeavesTheBoardAsItWas)
		{
			SearchFlag searching;
			Position position(MIDDLE_GAME);
			const std::wstring before = Placement(*position.Get());
			int score = 0;
			std::wstring from, to;

			Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));
			Assert::AreEqual(before, Placement(*position.Get()));

			Assert::IsTrue(WhitePlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));
			Assert::AreEqual(before, Placement(*position.Get()));
		}

		TEST_METHOD(TheSameBoardGivesTheSameMove)
		{
			SearchFlag searching;
			Position position(MIDDLE_GAME);
			int score = 0;
			std::wstring firstFrom, firstTo, secondFrom, secondTo;

			Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(2, score, firstFrom, firstTo, position.Get()));
			Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(2, score, secondFrom, secondTo, position.Get()));

			Assert::AreEqual(firstFrom, secondFrom);
			Assert::AreEqual(firstTo, secondTo);
		}

		TEST_METHOD(TheSearchFreesEveryPieceItCopies)
		{
			// The search tries the moves on copies of the board: they are counted pieces, which have to be freed
			SearchFlag searching;
			Position position(MIDDLE_GAME);
			const int livePieces = LivePieces();
			int score = 0;
			std::wstring from, to;

			Assert::IsTrue(WhitePlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));
			Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));

			Assert::AreEqual(livePieces, LivePieces());
		}

		TEST_METHOD(TheScoreIsInFavourOfThePlayerWhoIsAhead)
		{
			// One of the players has a queen more, and the other has nothing but its king
			SearchFlag searching;
			Position whiteAhead(L"4k3/8/8/8/8/8/8/3QK3");
			Position blackAhead(L"3qk3/8/8/8/8/8/8/4K3");
			int score = 0;
			std::wstring from, to;

			Assert::IsTrue(WhitePlayer_BacktrackingAlgorithm(2, score, from, to, whiteAhead.Get()));
			Assert::IsTrue(score > 500, L"White is ahead and has the move");
			Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(2, score, from, to, whiteAhead.Get()));
			Assert::IsTrue(score < -500, L"White is ahead and Black has the move");

			Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(2, score, from, to, blackAhead.Get()));
			Assert::IsTrue(score > 500, L"Black is ahead and has the move");
			Assert::IsTrue(WhitePlayer_BacktrackingAlgorithm(2, score, from, to, blackAhead.Get()));
			Assert::IsTrue(score < -500, L"Black is ahead and White has the move");
		}

		TEST_METHOD(Black_TakesAPieceWhenItCan)
		{
			// The rook on A3 takes the rook on A1, the only capture of Black
			SearchFlag searching;
			Position position(L"4k3/8/8/8/8/r7/8/R3K3");

			for (int level = 1; level <= 2; ++level)
			{
				const std::wstring message = L"Search level " + std::to_wstring(level);
				int score = 0;
				std::wstring from, to;

				Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(level, score, from, to, position.Get()), message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(L"A3"), from, message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(L"A1"), to, message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(Black_TakesTheMostValuablePieceOfThoseInReachOfAPiece)
		{
			// The rook on D4 can take the pawn on H4 or the queen on D1
			SearchFlag searching;
			Position position(L"4k3/8/8/8/3r3P/8/8/K2Q4");

			for (int level = 1; level <= 2; ++level)
			{
				const std::wstring message = L"Search level " + std::to_wstring(level);
				int score = 0;
				std::wstring from, to;

				Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(level, score, from, to, position.Get()), message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(L"D4"), from, message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(L"D1"), to, message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(Black_HasNoMoveWithoutPieces)
		{
			SearchFlag searching;
			Position position(L"8/8/8/8/8/8/8/4K3");
			int score = 0;
			std::wstring from = L"stale", to = L"stale";

			Assert::IsFalse(BlackPlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));

			Assert::AreEqual(std::wstring(), from);
			Assert::AreEqual(std::wstring(), to);
		}

		TEST_METHOD(White_HasNoMoveWithoutPieces)
		{
			SearchFlag searching;
			Position position(L"4k3/8/8/8/8/8/8/8");
			int score = 0;
			std::wstring from = L"stale", to = L"stale";

			Assert::IsFalse(WhitePlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));

			Assert::AreEqual(std::wstring(), from);
			Assert::AreEqual(std::wstring(), to);
		}

		TEST_METHOD(APlayerWhoseOnlyPiecesAreBlockedHasNoMove)
		{
			// Neither of the two pawns can advance or capture
			SearchFlag searching;
			Position position(L"8/8/8/8/8/p7/P7/8");
			int score = 0;
			std::wstring from, to;

			Assert::IsFalse(BlackPlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));
			Assert::IsFalse(WhitePlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));
		}

		TEST_METHOD(TheSearchGivesNothingWhenItIsStopped)
		{
			SearchFlag stopped(false);
			Position position(INITIAL_POSITION);
			int score = 0;
			std::wstring from = L"stale", to = L"stale";

			Assert::IsFalse(BlackPlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));
			Assert::AreEqual(std::wstring(), from);
			Assert::AreEqual(std::wstring(), to);

			from = L"stale";
			to = L"stale";
			Assert::IsFalse(WhitePlayer_BacktrackingAlgorithm(2, score, from, to, position.Get()));
			Assert::AreEqual(std::wstring(), from);
			Assert::AreEqual(std::wstring(), to);
		}

		TEST_METHOD(Black_AnswersACheckWithAMoveOfItsKing)
		{
			// The rook on E1 checks the king on E8. The rook on A8 comes first and none of its moves answers the check
			SearchFlag searching;
			const wchar_t* const placement = L"r3k3/8/8/8/8/8/8/4R2K";
			Position position(placement);

			for (int level = 1; level <= 2; ++level)
			{
				const std::wstring message = L"Search level " + std::to_wstring(level);
				int score = 0;
				std::wstring from, to;

				Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(level, score, from, to, position.Get()), message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(L"E8"), from, message.c_str(), LINE_INFO());
				Assert::IsTrue(EngineAccepts(placement, false, from, to), message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(White_AnswersACheckWithAMoveOfItsKing)
		{
			// The rook on E8 checks the king on E1. The rook on A1 comes first and none of its moves answers the check
			SearchFlag searching;
			const wchar_t* const placement = L"4r1k1/8/8/8/8/8/8/R3K3";
			Position position(placement);

			for (int level = 1; level <= 2; ++level)
			{
				const std::wstring message = L"Search level " + std::to_wstring(level);
				int score = 0;
				std::wstring from, to;

				Assert::IsTrue(WhitePlayer_BacktrackingAlgorithm(level, score, from, to, position.Get()), message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(L"E1"), from, message.c_str(), LINE_INFO());
				Assert::IsTrue(EngineAccepts(placement, true, from, to), message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(Black_DoesNotCaptureWithAPinnedPiece)
		{
			// The bishop on E7 could take the queen on H4, but it shields its king from the rook on E1: it cannot move at all
			SearchFlag searching;
			const wchar_t* const placement = L"4k3/4b3/8/8/7Q/8/8/4R2K";
			Position position(placement);

			for (int level = 1; level <= 2; ++level)
			{
				const std::wstring message = L"Search level " + std::to_wstring(level);
				int score = 0;
				std::wstring from, to;

				Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(level, score, from, to, position.Get()), message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(L"E8"), from, message.c_str(), LINE_INFO());
				Assert::IsTrue(EngineAccepts(placement, false, from, to), message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(Black_TakesALegalPieceWhenTheCaptureOfAPinnedPieceIsNotLegal)
		{
			// The pinned bishop on E7 could take the queen on H4, the rook on A8 takes the rook on A1 instead
			// (the knight on G8 protects the bishop, without it the queen takes the bishop with a checkmate)
			SearchFlag searching;
			const wchar_t* const placement = L"r3k1n1/4b3/8/8/7Q/8/8/R3R2K";
			Position position(placement);

			for (int level = 1; level <= 2; ++level)
			{
				const std::wstring message = L"Search level " + std::to_wstring(level);
				int score = 0;
				std::wstring from, to;

				Assert::IsTrue(BlackPlayer_BacktrackingAlgorithm(level, score, from, to, position.Get()), message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(L"A8"), from, message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(L"A1"), to, message.c_str(), LINE_INFO());
				Assert::IsTrue(EngineAccepts(placement, false, from, to), message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(Black_HasNoMoveWhenCheckmated)
		{
			// The king on H8 can only go to squares the queen on G7 attacks, or take the queen that the king on F6 protects
			SearchFlag searching;
			Position position(L"7k/6Q1/5K2/8/8/8/8/8");

			for (int level = 1; level <= 2; ++level)
			{
				const std::wstring message = L"Search level " + std::to_wstring(level);
				int score = 0;
				std::wstring from = L"stale", to = L"stale";

				Assert::IsFalse(BlackPlayer_BacktrackingAlgorithm(level, score, from, to, position.Get()), message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(), from, message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(), to, message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(White_HasNoMoveWhenCheckmated)
		{
			// The king on A1 can only go to squares the queen on B2 attacks, or take the queen that the king on B3 protects
			SearchFlag searching;
			Position position(L"8/8/8/8/8/1k6/1q6/K7");

			for (int level = 1; level <= 2; ++level)
			{
				const std::wstring message = L"Search level " + std::to_wstring(level);
				int score = 0;
				std::wstring from = L"stale", to = L"stale";

				Assert::IsFalse(WhitePlayer_BacktrackingAlgorithm(level, score, from, to, position.Get()), message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(), from, message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(), to, message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(Black_HasNoMoveWhenStalemated)
		{
			// The king on H8 is not in check, and every square it can go to is attacked by the queen on F7 or the king on G6
			SearchFlag searching;
			Position position(L"7k/5Q2/6K1/8/8/8/8/8");

			for (int level = 1; level <= 2; ++level)
			{
				const std::wstring message = L"Search level " + std::to_wstring(level);
				int score = 0;
				std::wstring from = L"stale", to = L"stale";

				Assert::IsFalse(BlackPlayer_BacktrackingAlgorithm(level, score, from, to, position.Get()), message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(), from, message.c_str(), LINE_INFO());
				Assert::AreEqual(std::wstring(), to, message.c_str(), LINE_INFO());
			}
		}
	};

	TEST_CLASS(ComputerThreadProcTests)
	{
	public:
		TEST_METHOD(WithoutAGame_ReturnsZeroAndClearsTheFlag)
		{
			SearchFlag searching;

			const DWORD result = ComputerThreadProc(nullptr);

			Assert::AreEqual<DWORD>(0, result);
			Assert::IsFalse(g_bThreadRunning);
		}

		TEST_METHOD(PlaysAQuietMoveOfBlackAndPassesTheTurnToWhite)
		{
			SearchFlag searching;
			ChessBoard game;
			Play(game, L"E2", L"E4");
			Assert::IsFalse(game.IsWhiteTurn());
			const Letters before = LettersOf(*game._board);

			const DWORD result = ComputerThreadProc(&game);

			Assert::AreEqual<DWORD>(0, result);
			Assert::IsFalse(g_bThreadRunning);
			Assert::IsTrue(game.IsWhiteTurn());
			Assert::IsFalse(game.HasGameEnded());

			const Letters after = LettersOf(*game._board);
			Assert::AreEqual(2, CountDifferences(before, after));
			Assert::IsTrue(WhiteLetters(before) == WhiteLetters(after), L"The pieces of White must not change");
			Assert::AreEqual(32, static_cast<int>(game._board->size()));
		}

		TEST_METHOD(TakesAPieceWhenItCan)
		{
			SearchFlag searching;
			ChessBoard game;
			SetPosition(game, L"4k3/8/8/8/8/r7/8/R3K3");
			Play(game, L"E1", L"E2");

			ComputerThreadProc(&game);

			AssertPlacement(L"4k3/8/8/8/8/8/4K3/r7", game, LINE_INFO());
			Assert::IsTrue(game.IsWhiteTurn());
		}

		TEST_METHOD(DoesNotMoveWhenItIsTheTurnOfWhite)
		{
			SearchFlag searching;
			ChessBoard game;

			ComputerThreadProc(&game);

			AssertPlacement(INITIAL_POSITION, game, LINE_INFO());
			Assert::IsTrue(game.IsWhiteTurn());
			Assert::IsFalse(g_bThreadRunning);
		}

		TEST_METHOD(DoesNotMoveWhenTheGameEnded)
		{
			SearchFlag searching;
			ChessBoard game;
			PlayMoves(game, L"F2F3 E7E5 G2G4 D8H4");
			Assert::IsTrue(game.HasGameEnded());
			const std::wstring before = Placement(*game._board);

			ComputerThreadProc(&game);

			Assert::AreEqual(before, Placement(*game._board));
			Assert::IsTrue(game.HasGameEnded());
			Assert::IsFalse(g_bThreadRunning);
		}

		TEST_METHOD(AnswersACheckWithAMoveOfItsKing)
		{
			// The rook on E1 checks the king on E8: the king goes to D8, F8, D7 or F7 (E7 is on the file of the rook)
			SearchFlag searching;
			ChessBoard game;
			SetPosition(game, L"r3k3/8/8/8/8/8/8/R6K");
			Play(game, L"A1", L"E1");
			Assert::IsFalse(game.IsWhiteTurn());

			ComputerThreadProc(&game);

			Assert::IsTrue(game.IsWhiteTurn(), L"Black did not move");
			const std::wstring after = Placement(*game._board);
			const bool movedTheKing = (after == L"r2k4/8/8/8/8/8/8/4R2K") || (after == L"r4k2/8/8/8/8/8/8/4R2K") ||
				(after == L"r7/3k4/8/8/8/8/8/4R2K") || (after == L"r7/5k2/8/8/8/8/8/4R2K");
			Assert::IsTrue(movedTheKing, (L"Black did not answer the check, the position is " + after).c_str());
		}

		TEST_METHOD(DoesNotMoveAPinnedPieceToCapture)
		{
			// The bishop on E7 could take the queen on H4, but it shields its king from the rook on E1
			SearchFlag searching;
			ChessBoard game;
			SetPosition(game, L"4k3/4b3/8/8/7Q/8/8/4R2K");
			Play(game, L"H1", L"G1");
			Assert::IsFalse(game.IsWhiteTurn());

			ComputerThreadProc(&game);

			Assert::IsTrue(game.IsWhiteTurn(), L"Black did not move");
			const Letters after = LettersOf(*game._board);
			Assert::AreEqual(L'b', LetterOn(after, L"E7"));
			Assert::AreEqual(L'Q', LetterOn(after, L"H4"));
			Assert::AreEqual(5, static_cast<int>(game._board->size()));
		}
	};

	TEST_CLASS(ComputerBoardTests)
	{
	public:
		TEST_METHOD(ABoardIsCopiedSquareBySquare)
		{
			const wchar_t* const placements[] =
			{
				INITIAL_POSITION,
				MIDDLE_GAME,
				L"r3k1n1/4b3/8/8/7Q/8/8/R3R2K",
				L"4k3/P5P1/8/8/8/8/p5p1/4K3",
				L"8/8/8/8/8/8/8/4K3",
			};
			for (const wchar_t* placement : placements)
			{
				for (int side = 0; side < 2; ++side)
				{
					const bool whiteToMove = (side == 0);
					const std::wstring message = std::wstring(L"Position ") + placement + (whiteToMove ? L" with White to move" : L" with Black to move");
					Position board(placement);
					ChessSearch::Position copy;

					Assert::IsTrue(BoardToPosition(board.Get(), whiteToMove, copy), message.c_str(), LINE_INFO());
					Assert::IsTrue(SameSearchPosition(ParseSearchPosition(placement, whiteToMove), copy), message.c_str(), LINE_INFO());
				}
			}
		}

		TEST_METHOD(ACopyReplacesWhatThePositionHeld)
		{
			// The same object is used again and again: nothing of the previous board may stay in it
			Position first(INITIAL_POSITION);
			Position second(L"4k3/8/8/8/8/8/8/4K3");
			ChessSearch::Position copy;

			Assert::IsTrue(BoardToPosition(first.Get(), true, copy));
			Assert::IsTrue(BoardToPosition(second.Get(), false, copy));

			Assert::IsTrue(SameSearchPosition(ParseSearchPosition(L"4k3/8/8/8/8/8/8/4K3", false), copy));
		}

		TEST_METHOD(ABoardWithSomethingThatIsNotAChessPieceIsRefused)
		{
			Position board(L"4k3/8/8/8/8/8/8/4K3");
			board.Get()->insert({ L"D4", new EmptyPiece(true) });
			ChessSearch::Position copy;

			Assert::IsFalse(BoardToPosition(board.Get(), true, copy));
		}

		TEST_METHOD(ABoardWithAPieceOffTheBoardIsRefused)
		{
			Position board(L"4k3/8/8/8/8/8/8/4K3");
			board.Get()->insert({ L"D9", NewPiece(L'P') });
			ChessSearch::Position copy;

			Assert::IsFalse(BoardToPosition(board.Get(), true, copy));
		}
	};

	TEST_CLASS(ComputerMovesTests)
	{
	public:
		TEST_METHOD(TheMovesOfSpecialPositionsAreTheMovesOfTheGame)
		{
			const wchar_t* const placements[] =
			{
				INITIAL_POSITION,
				MIDDLE_GAME,

				// Pawns about to reach the last rank, and pawns that have reached it
				L"4k3/P5P1/8/8/8/8/p5p1/4K3",
				L"P3k3/8/8/8/8/8/8/p3K3",

				// The king and the rooks where they could castle, and pawns where a capture en passant could be
				L"r3k2r/8/8/8/8/8/8/R3K2R",
				L"4k3/8/8/3pP3/8/8/8/4K3",

				// Pinned pieces, a rook next to the king and a check by a knight
				L"4k3/4r3/8/8/8/8/4N3/4K3",
				L"4k3/8/8/8/1b6/8/3N4/4K3",
				L"r3k1n1/4b3/8/8/7Q/8/8/R3R2K",
				L"4k3/8/8/8/8/8/4r3/R3K3",
				L"4k3/8/8/8/8/3n4/8/R3K3",
				L"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8",

				// A checkmate and a stalemate
				L"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR",
				L"k7/2Q5/1K6/8/8/8/8/8",
			};

			int movesCompared = 0;
			for (const wchar_t* placement : placements)
			{
				for (int side = 0; side < 2; ++side)
				{
					const bool whiteToMove = (side == 0);
					Position board(placement);
					ChessSearch::Position position;
					Assert::IsTrue(BoardToPosition(board.Get(), whiteToMove, position));
					if (OpponentIsInCheck(position))
					{
						continue;
					}

					const std::wstring where = std::wstring(L"Position ") + placement + (whiteToMove ? L" with White to move" : L" with Black to move");
					movesCompared += static_cast<int>(AssertSameMoves(board.Get(), whiteToMove, where).size());
				}
			}
			Assert::IsTrue(movesCompared > 200, L"The positions have moves to compare");
		}

		TEST_METHOD(TheMovesOfRandomGamesAreTheMovesOfTheGame)
		{
			// The games are played at random with a generator that gives the same games every time
			uint32_t state = 2026;
			int movesCompared = 0;
			for (int number = 1; number <= 3; ++number)
			{
				ChessBoard game;
				for (int ply = 1; ply <= 40; ++ply)
				{
					const std::wstring where = L"Game " + std::to_wstring(number) + L" before ply " + std::to_wstring(ply) + L" in " + Placement(*game._board);
					const std::set<std::wstring> moves = AssertSameMoves(game._board, game.IsWhiteTurn(), where);
					if (moves.empty())
					{
						Assert::IsTrue(game.HasGameEnded(), (where + L" has no move and the game goes on").c_str());
						break;
					}
					movesCompared += static_cast<int>(moves.size());

					state = state * 1664525u + 1013904223u;
					std::set<std::wstring>::const_iterator chosen = moves.begin();
					std::advance(chosen, static_cast<std::ptrdiff_t>((state >> 16) % moves.size()));
					Play(game, chosen->substr(0, 2).c_str(), chosen->substr(2, 2).c_str());
				}
			}
			Assert::IsTrue(movesCompared > 1000, L"The games have moves to compare");
		}
	};

	TEST_CLASS(ComputerGameTests)
	{
	public:
		TEST_METHOD(TwoComputerPlayersPlayAGameOfMovesTheGameAccepts)
		{
			SearchFlag searching;
			ChessBoard game;
			for (int ply = 1; (ply <= 40) && !game.HasGameEnded(); ++ply)
			{
				const bool whiteToMove = game.IsWhiteTurn();
				const std::wstring where = L"Ply " + std::to_wstring(ply) + (whiteToMove ? L" of White in " : L" of Black in ") + Placement(*game._board);
				int score = 0;
				std::wstring from, to;

				const bool found = whiteToMove ? WhitePlayer_BacktrackingAlgorithm(3, score, from, to, game._board) :
					BlackPlayer_BacktrackingAlgorithm(3, score, from, to, game._board);

				// A game that has not ended has a move to play, and Play() fails when the game refuses it
				Assert::IsTrue(found, (where + L" has no move").c_str());
				Play(game, from.c_str(), to.c_str());
			}
		}
	};
}
