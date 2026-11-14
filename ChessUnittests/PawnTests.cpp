// PawnTests.cpp - unit tests of the moves of the Pawn: advancing, the first move and capturing

#include "pch.h"
#include "TestSupport.hpp"

using namespace ChessTests;

namespace ChessUnittests
{
	TEST_CLASS(PawnAdvanceTests)
	{
	public:
		TEST_METHOD(White_AdvancesOneOrTwoSquaresFromItsStartingRank)
		{
			Position position(L"8/8/8/8/8/8/4P3/8");
			Piece* pawn = position.At(L"E2");

			Assert::AreEqual(std::wstring(L"E3 E4"), Destinations(pawn, L"E2", position.Get()));
		}

		TEST_METHOD(Black_AdvancesOneOrTwoSquaresFromItsStartingRank)
		{
			Position position(L"8/4p3/8/8/8/8/8/8");
			Piece* pawn = position.At(L"E7");

			Assert::AreEqual(std::wstring(L"E5 E6"), Destinations(pawn, L"E7", position.Get()));
		}

		TEST_METHOD(White_CannotMoveBackwardSidewaysOrFartherThanTwoSquares)
		{
			Position position(L"8/8/8/8/8/8/4P3/8");
			Piece* pawn = position.At(L"E2");

			AssertMove(pawn, L"E2", L"E5", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E2", L"E8", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E2", L"E1", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E2", L"D2", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E2", L"F2", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E2", L"D1", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E2", L"C4", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E2", L"E2", position.Get(), ChessErrHandler::DEST_EQ_SOURCE, LINE_INFO());
		}

		TEST_METHOD(Black_CannotMoveBackwardSidewaysOrFartherThanTwoSquares)
		{
			Position position(L"8/4p3/8/8/8/8/8/8");
			Piece* pawn = position.At(L"E7");

			AssertMove(pawn, L"E7", L"E4", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E7", L"E1", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E7", L"E8", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E7", L"D7", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E7", L"F7", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E7", L"D8", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E7", L"E7", position.Get(), ChessErrHandler::DEST_EQ_SOURCE, LINE_INFO());
		}

		TEST_METHOD(CannotMoveDiagonallyToAnEmptySquare)
		{
			Position position(L"8/4p3/8/8/8/8/4P3/8");

			AssertMove(position.At(L"E2"), L"E2", L"D3", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(position.At(L"E2"), L"E2", L"F3", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(position.At(L"E7"), L"E7", L"D6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(position.At(L"E7"), L"E7", L"F6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
		}

		TEST_METHOD(AfterItsFirstMove_AdvancesOnlyOneSquare)
		{
			Position position(L"8/8/8/8/8/8/4P3/8");
			Piece* pawn = position.At(L"E2");
			pawn->confirmMove();

			Assert::AreEqual(std::wstring(L"E3"), Destinations(pawn, L"E2", position.Get()));
			AssertMove(pawn, L"E2", L"E4", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
		}

		TEST_METHOD(OutsideItsStartingRank_AdvancesOnlyOneSquare)
		{
			Position position(L"8/8/4p3/8/8/4P3/8/8");

			Assert::AreEqual(std::wstring(L"E4"), Destinations(position.At(L"E3"), L"E3", position.Get()));
			Assert::AreEqual(std::wstring(L"E5"), Destinations(position.At(L"E6"), L"E6", position.Get()));
		}
	};

	TEST_CLASS(PawnBlockedTests)
	{
	public:
		TEST_METHOD(AnEnemyPieceAheadBlocksItAndCannotBeCapturedStraight)
		{
			Position position(L"8/8/8/8/8/4p3/4P3/8");
			Piece* pawn = position.At(L"E2");

			AssertMove(pawn, L"E2", L"E3", position.Get(), ChessErrHandler::PAWN_ILLEGAL_CAPTURE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E2", L"E4", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			Assert::AreEqual(std::wstring(L""), Destinations(pawn, L"E2", position.Get()));
		}

		TEST_METHOD(AFriendlyPieceAheadBlocksIt)
		{
			Position position(L"8/8/8/8/8/4P3/4P3/8");
			Piece* pawn = position.At(L"E2");

			AssertMove(pawn, L"E2", L"E3", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(pawn, L"E2", L"E4", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			Assert::AreEqual(std::wstring(L""), Destinations(pawn, L"E2", position.Get()));
		}

		TEST_METHOD(Black_IsBlockedTheSameWay)
		{
			Position position(L"8/4p3/4P3/8/8/8/8/8");
			Piece* pawn = position.At(L"E7");

			AssertMove(pawn, L"E7", L"E6", position.Get(), ChessErrHandler::PAWN_ILLEGAL_CAPTURE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E7", L"E5", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			Assert::AreEqual(std::wstring(L""), Destinations(pawn, L"E7", position.Get()));
		}

		TEST_METHOD(AnEnemyPieceOnTheSecondSquareCannotBeCapturedStraight)
		{
			Position position(L"8/8/8/8/4p3/8/4P3/8");
			Piece* pawn = position.At(L"E2");

			AssertMove(pawn, L"E2", L"E3", position.Get(), VALID, LINE_INFO());
			AssertMove(pawn, L"E2", L"E4", position.Get(), ChessErrHandler::PAWN_ILLEGAL_CAPTURE_PATTERN, LINE_INFO());
		}

		TEST_METHOD(AFriendlyPieceOnTheSecondSquareBlocksTheDoubleStep)
		{
			Position position(L"8/8/8/8/4P3/8/4P3/8");
			Piece* pawn = position.At(L"E2");

			AssertMove(pawn, L"E2", L"E3", position.Get(), VALID, LINE_INFO());
			AssertMove(pawn, L"E2", L"E4", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
		}
	};

	TEST_CLASS(PawnCaptureTests)
	{
	public:
		TEST_METHOD(White_CapturesDiagonallyForward)
		{
			Position position(L"8/8/8/3p1p2/4P3/8/8/8");
			Piece* pawn = position.At(L"E4");

			Assert::AreEqual(std::wstring(L"D5 E5 F5"), Destinations(pawn, L"E4", position.Get()));
		}

		TEST_METHOD(Black_CapturesDiagonallyForward)
		{
			Position position(L"8/8/8/4p3/3P1P2/8/8/8");
			Piece* pawn = position.At(L"E5");

			Assert::AreEqual(std::wstring(L"D4 E4 F4"), Destinations(pawn, L"E5", position.Get()));
		}

		TEST_METHOD(CannotCaptureBackwardOrSideways)
		{
			Position position(L"8/8/8/8/3pPp2/3p1p2/8/8");
			Piece* pawn = position.At(L"E4");

			AssertMove(pawn, L"E4", L"D4", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E4", L"F4", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E4", L"D3", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E4", L"F3", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			Assert::AreEqual(std::wstring(L"E5"), Destinations(pawn, L"E4", position.Get()));
		}

		TEST_METHOD(CannotCaptureFriendlyPieces)
		{
			Position position(L"8/8/8/3P1P2/4P3/8/8/8");
			Piece* pawn = position.At(L"E4");

			AssertMove(pawn, L"E4", L"D5", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E4", L"F5", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			Assert::AreEqual(std::wstring(L"E5"), Destinations(pawn, L"E4", position.Get()));
		}

		TEST_METHOD(CannotCaptureFartherThanTheAdjacentFile)
		{
			Position position(L"8/8/6p1/2p5/4P3/8/8/8");
			Piece* pawn = position.At(L"E4");

			AssertMove(pawn, L"E4", L"C5", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(pawn, L"E4", L"G6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
		}

		TEST_METHOD(OnItsFirstMoveItCapturesLikeAnyOtherPawn)
		{
			Position position(L"8/8/8/8/8/3p1p2/4P3/8");
			Piece* pawn = position.At(L"E2");

			Assert::AreEqual(std::wstring(L"D3 E3 E4 F3"), Destinations(pawn, L"E2", position.Get()));
		}
	};
}
