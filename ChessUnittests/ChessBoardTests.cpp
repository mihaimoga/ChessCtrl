// ChessBoardTests.cpp - unit tests of the ChessBoard: a new game, the moves that are accepted and the moves that are rejected

#include "pch.h"
#include "TestSupport.hpp"

using namespace ChessTests;

namespace
{
	int PieceCount(ChessBoard& game)
	{
		return static_cast<int>(game._board->size());
	}
}

namespace ChessUnittests
{
	TEST_CLASS(ChessBoardSetupTests)
	{
	public:
		TEST_METHOD(NewGame_StartsFromTheInitialPosition)
		{
			ChessBoard game;

			AssertPlacement(INITIAL_POSITION, game, LINE_INFO());
			Assert::AreEqual(32, PieceCount(game));
		}

		TEST_METHOD(NewGame_WhiteMovesFirstAndTheGameIsNotOver)
		{
			ChessBoard game;

			Assert::IsTrue(game.IsWhiteTurn());
			Assert::IsFalse(game.HasGameEnded());
		}

		TEST_METHOD(NewGame_EveryPawnMayAdvanceTwoSquares)
		{
			ChessBoard game;

			for (wchar_t file = ChessInfo::MIN_FILE; file <= ChessInfo::MAX_FILE; ++file)
			{
				const std::wstring whiteFrom = Square(file, L'2');
				const std::wstring whiteTo = Square(file, L'4');
				AssertMove(game._board->at(whiteFrom), whiteFrom.c_str(), whiteTo.c_str(), game._board, VALID, LINE_INFO());

				const std::wstring blackFrom = Square(file, L'7');
				const std::wstring blackTo = Square(file, L'5');
				AssertMove(game._board->at(blackFrom), blackFrom.c_str(), blackTo.c_str(), game._board, VALID, LINE_INFO());
			}
		}

		TEST_METHOD(ResetBoard_RestoresTheInitialPositionAndTheFirstPlayer)
		{
			ChessBoard game;
			PlayMoves(game, L"E2E4 E7E5 G1F3");
			Assert::IsFalse(game.IsWhiteTurn());

			game.resetBoard();

			AssertPlacement(INITIAL_POSITION, game, LINE_INFO());
			Assert::AreEqual(32, PieceCount(game));
			Assert::IsTrue(game.IsWhiteTurn());
			Assert::IsFalse(game.HasGameEnded());
		}

		TEST_METHOD(ResetBoard_GivesEveryPawnItsFirstMoveBack)
		{
			ChessBoard game;
			PlayMoves(game, L"E2E3 A7A6 E3E4");

			game.resetBoard();

			Play(game, L"E2", L"E4");
			AssertPlacement(L"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR", game, LINE_INFO());
		}

		TEST_METHOD(ResetBoard_ReopensAFinishedGame)
		{
			ChessBoard game;
			PlayMoves(game, L"F2F3 E7E5 G2G4 D8H4");
			Assert::IsTrue(game.HasGameEnded());

			game.resetBoard();

			Assert::IsFalse(game.HasGameEnded());
			Assert::IsTrue(game.IsWhiteTurn());
			AssertPlacement(INITIAL_POSITION, game, LINE_INFO());
			Play(game, L"D2", L"D4");
			AssertPlacement(L"rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR", game, LINE_INFO());
		}
	};

	TEST_CLASS(ChessBoardMoveTests)
	{
	public:
		TEST_METHOD(PawnAdvancesTwoSquaresFromItsStartingRank)
		{
			ChessBoard game;

			Play(game, L"E2", L"E4");

			AssertPlacement(L"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR", game, LINE_INFO());
		}

		TEST_METHOD(PawnAdvancesOneSquare)
		{
			ChessBoard game;

			Play(game, L"E2", L"E3");

			AssertPlacement(L"rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR", game, LINE_INFO());
		}

		TEST_METHOD(PawnKeepsAdvancingAfterItsFirstMove)
		{
			ChessBoard game;

			PlayMoves(game, L"E2E4 E7E6 E4E5");

			AssertPlacement(L"rnbqkbnr/pppp1ppp/4p3/4P3/8/8/PPPP1PPP/RNBQKBNR", game, LINE_INFO());
		}

		TEST_METHOD(KnightJumpsOverThePieces)
		{
			ChessBoard game;

			Play(game, L"B1", L"C3");

			AssertPlacement(L"rnbqkbnr/pppppppp/8/8/8/2N5/PPPPPPPP/R1BQKBNR", game, LINE_INFO());
		}

		TEST_METHOD(BlackMovesAfterWhite)
		{
			ChessBoard game;

			PlayMoves(game, L"E2E4 G8F6");

			AssertPlacement(L"rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR", game, LINE_INFO());
		}

		TEST_METHOD(PlayersMoveInTurn)
		{
			ChessBoard game;
			Assert::IsTrue(game.IsWhiteTurn());

			Play(game, L"E2", L"E4");
			Assert::IsFalse(game.IsWhiteTurn());

			Play(game, L"E7", L"E5");
			Assert::IsTrue(game.IsWhiteTurn());

			Play(game, L"G1", L"F3");
			Assert::IsFalse(game.IsWhiteTurn());
			Assert::IsFalse(game.HasGameEnded());
		}

		TEST_METHOD(BishopMovesOnceItsPathIsOpen)
		{
			ChessBoard game;

			PlayMoves(game, L"E2E4 E7E5 F1C4");

			AssertPlacement(L"rnbqkbnr/pppp1ppp/8/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR", game, LINE_INFO());
		}

		TEST_METHOD(QueenMovesOnceItsPathIsOpen)
		{
			ChessBoard game;

			PlayMoves(game, L"E2E4 E7E5 D1H5");

			AssertPlacement(L"rnbqkbnr/pppp1ppp/8/4p2Q/4P3/8/PPPP1PPP/RNB1KBNR", game, LINE_INFO());
		}

		TEST_METHOD(KingMovesToASafeSquare)
		{
			ChessBoard game;

			PlayMoves(game, L"E2E4 E7E5 E1E2");

			AssertPlacement(L"rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPPKPPP/RNBQ1BNR", game, LINE_INFO());
		}

		TEST_METHOD(PawnCapturesDiagonally)
		{
			ChessBoard game;
			PlayMoves(game, L"E2E4 D7D5");
			Assert::AreEqual(32, PieceCount(game));

			Play(game, L"E4", L"D5");

			AssertPlacement(L"rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR", game, LINE_INFO());
			Assert::AreEqual(31, PieceCount(game));
		}

		TEST_METHOD(QueenCapturesAPieceAlongAFile)
		{
			ChessBoard game;
			PlayMoves(game, L"E2E4 D7D5 E4D5");

			Play(game, L"D8", L"D5");

			AssertPlacement(L"rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR", game, LINE_INFO());
			Assert::AreEqual(30, PieceCount(game));
			Assert::IsFalse(game.HasGameEnded());
		}
	};

	TEST_CLASS(ChessBoardRejectedMoveTests)
	{
	public:
		TEST_METHOD(AMoveFromAnEmptySquare)
		{
			ChessBoard game;

			AssertRejected(game, L"E3", L"E4", LINE_INFO());
			AssertRejected(game, L"D5", L"D6", LINE_INFO());
		}

		TEST_METHOD(AMoveOfTheOpponentsPiece)
		{
			ChessBoard game;

			AssertRejected(game, L"E7", L"E5", LINE_INFO());
			AssertRejected(game, L"G8", L"F6", LINE_INFO());

			Play(game, L"E2", L"E4");
			AssertRejected(game, L"D2", L"D4", LINE_INFO());
			AssertRejected(game, L"G1", L"F3", LINE_INFO());
		}

		TEST_METHOD(AMoveToTheSameSquare)
		{
			ChessBoard game;

			AssertRejected(game, L"E2", L"E2", LINE_INFO());
			AssertRejected(game, L"B1", L"B1", LINE_INFO());
		}

		TEST_METHOD(AMoveFromOrToASquareOutsideTheBoard)
		{
			ChessBoard game;

			AssertRejected(game, L"E9", L"E4", LINE_INFO());
			AssertRejected(game, L"E0", L"E1", LINE_INFO());
			AssertRejected(game, L"J2", L"J3", LINE_INFO());
			AssertRejected(game, L"@2", L"@3", LINE_INFO());
			AssertRejected(game, L"E2", L"E9", LINE_INFO());
			AssertRejected(game, L"E2", L"J3", LINE_INFO());
			AssertRejected(game, L"B1", L"I3", LINE_INFO());
		}

		TEST_METHOD(AMoveWrittenWithTheWrongNumberOfCharacters)
		{
			ChessBoard game;

			AssertRejected(game, L"", L"E4", LINE_INFO());
			AssertRejected(game, L"E2", L"", LINE_INFO());
			AssertRejected(game, L"E", L"E4", LINE_INFO());
			AssertRejected(game, L"E2", L"E", LINE_INFO());
			AssertRejected(game, L"E22", L"E4", LINE_INFO());
			AssertRejected(game, L"E2", L"E44", LINE_INFO());
		}

		TEST_METHOD(AMoveOntoAFriendlyPiece)
		{
			ChessBoard game;

			AssertRejected(game, L"A1", L"A2", LINE_INFO());
			AssertRejected(game, L"B1", L"D2", LINE_INFO());
			AssertRejected(game, L"D1", L"E1", LINE_INFO());
			AssertRejected(game, L"E1", L"E2", LINE_INFO());
		}

		TEST_METHOD(AMoveThroughOtherPieces)
		{
			ChessBoard game;

			AssertRejected(game, L"A1", L"A3", LINE_INFO());
			AssertRejected(game, L"C1", L"E3", LINE_INFO());
			AssertRejected(game, L"D1", L"D3", LINE_INFO());
			AssertRejected(game, L"F1", L"H3", LINE_INFO());
		}

		TEST_METHOD(AMoveThatDoesNotFollowTheRulesOfThePiece)
		{
			ChessBoard game;

			AssertRejected(game, L"B1", L"B3", LINE_INFO());
			AssertRejected(game, L"A1", L"B2", LINE_INFO());
			AssertRejected(game, L"E2", L"E5", LINE_INFO());
			AssertRejected(game, L"E2", L"D3", LINE_INFO());
			AssertRejected(game, L"E1", L"E3", LINE_INFO());
		}

		TEST_METHOD(APawnCannotCaptureStraightAhead)
		{
			ChessBoard game;
			PlayMoves(game, L"E2E4 E7E5");

			AssertRejected(game, L"E4", L"E5", LINE_INFO());
		}

		TEST_METHOD(APawnMayAdvanceTwoSquaresOnlyOnItsFirstMove)
		{
			ChessBoard game;
			PlayMoves(game, L"E2E3 A7A6");

			AssertRejected(game, L"E3", L"E5", LINE_INFO());
			Play(game, L"E3", L"E4");
		}

		TEST_METHOD(ARejectedMoveKeepsTheTurn)
		{
			ChessBoard game;

			AssertRejected(game, L"E2", L"E5", LINE_INFO());
			Assert::IsTrue(game.IsWhiteTurn());

			Play(game, L"E2", L"E4");
			AssertPlacement(L"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR", game, LINE_INFO());
		}

		TEST_METHOD(ARejectedMoveOfBlackKeepsTheTurnOfBlack)
		{
			ChessBoard game;
			Play(game, L"E2", L"E4");

			AssertRejected(game, L"E7", L"E4", LINE_INFO());
			Assert::IsFalse(game.IsWhiteTurn());

			Play(game, L"E7", L"E5");
			AssertPlacement(L"rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR", game, LINE_INFO());
		}
	};

	/* A move is tried on a copy of the board (all of its pieces are cloned) before it is played, and the copy has to be
	   freed whether the move is accepted or not: a test compares the number of live pieces before and after the move.
	*/
	TEST_CLASS(ChessBoardResourceTests)
	{
	public:
		TEST_METHOD(ARejectedCaptureFreesTheCopyOfTheBoard)
		{
			// The rook on E2 is pinned by the rook on E8: its capture of the pawn on A2 is tried, then rejected
			ChessBoard game;
			SetPosition(game, L"4r1k1/8/8/8/8/8/p3R3/4K3");
			const int livePieces = LivePieces();

			AssertRejected(game, L"E2", L"A2", LINE_INFO());

			Assert::AreEqual(livePieces, LivePieces());
			Assert::AreEqual(5, PieceCount(game));
		}

		TEST_METHOD(ARejectedMoveWithoutACaptureFreesTheCopyOfTheBoard)
		{
			// The pinned rook on E2 leaves the file of its king
			ChessBoard game;
			SetPosition(game, L"4r1k1/8/8/8/8/8/p3R3/4K3");
			const int livePieces = LivePieces();

			AssertRejected(game, L"E2", L"D2", LINE_INFO());

			Assert::AreEqual(livePieces, LivePieces());
			Assert::AreEqual(5, PieceCount(game));
		}

		TEST_METHOD(ARejectedMoveOfBlackFreesTheCopyOfTheBoard)
		{
			// The rook on E7 is pinned by the rook on E1
			ChessBoard game;
			Play(game, L"G1", L"F3");
			SetPosition(game, L"4k3/P3r3/8/8/8/8/8/4R2K");
			const int livePieces = LivePieces();

			AssertRejected(game, L"E7", L"D7", LINE_INFO());
			AssertRejected(game, L"E7", L"A7", LINE_INFO());

			Assert::AreEqual(livePieces, LivePieces());
		}

		TEST_METHOD(AnAcceptedMoveFreesTheCopyOfTheBoard)
		{
			// The rook on E2 moves along the file it is pinned on
			ChessBoard game;
			SetPosition(game, L"4r1k1/8/8/8/8/8/p3R3/4K3");
			const int livePieces = LivePieces();

			Play(game, L"E2", L"E5");

			Assert::AreEqual(livePieces, LivePieces());
			Assert::AreEqual(5, PieceCount(game));
		}

		TEST_METHOD(AnAcceptedCaptureFreesTheCopyOfTheBoardAndThePieceItTook)
		{
			ChessBoard game;
			SetPosition(game, L"6k1/8/8/8/8/8/p3R3/4K3");
			const int livePieces = LivePieces();

			Play(game, L"E2", L"A2");

			Assert::AreEqual(livePieces - 1, LivePieces());
			Assert::AreEqual(3, PieceCount(game));
		}
	};
}
