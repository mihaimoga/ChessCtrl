// KingSafetyTests.cpp - unit tests of the rules that involve the Kings: check, checkmate and stalemate

#include "pch.h"
#include "TestSupport.hpp"

using namespace ChessTests;

namespace ChessUnittests
{
	TEST_CLASS(KingSafetyTests)
	{
	public:
		TEST_METHOD(APinnedPieceCannotLeaveTheLineOfTheAttack)
		{
			// The rook on E2 shields its King from the rook on E8
			ChessBoard game;
			SetPosition(game, L"4r2k/8/8/8/8/8/4R3/4K3");

			AssertRejected(game, L"E2", L"D2", LINE_INFO());
			AssertRejected(game, L"E2", L"A2", LINE_INFO());
			AssertRejected(game, L"E2", L"H2", LINE_INFO());
		}

		TEST_METHOD(APinnedPieceMayMoveAlongTheLineOfTheAttack)
		{
			ChessBoard game;
			SetPosition(game, L"4r2k/8/8/8/8/8/4R3/4K3");

			Play(game, L"E2", L"E5");

			AssertPlacement(L"4r2k/8/8/4R3/8/8/8/4K3", game, LINE_INFO());
		}

		TEST_METHOD(APinnedPieceMayCaptureThePinningPiece)
		{
			ChessBoard game;
			SetPosition(game, L"4r2k/8/8/8/8/8/4R3/4K3");

			Play(game, L"E2", L"E8");

			AssertPlacement(L"4R2k/8/8/8/8/8/8/4K3", game, LINE_INFO());
			Assert::IsFalse(game.HasGameEnded());
		}

		TEST_METHOD(AKingCannotMoveOntoASquareAttackedByARook)
		{
			ChessBoard game;
			SetPosition(game, L"4k3/8/8/8/8/8/r7/4K3");

			AssertRejected(game, L"E1", L"D2", LINE_INFO());
			AssertRejected(game, L"E1", L"E2", LINE_INFO());
			AssertRejected(game, L"E1", L"F2", LINE_INFO());

			Play(game, L"E1", L"D1");
			AssertPlacement(L"4k3/8/8/8/8/8/r7/3K4", game, LINE_INFO());
		}

		TEST_METHOD(AKingCannotMoveOntoASquareAttackedByAKnight)
		{
			ChessBoard game;
			SetPosition(game, L"4k3/8/8/8/8/5n2/8/4K3");

			AssertRejected(game, L"E1", L"D2", LINE_INFO());

			Play(game, L"E1", L"E2");
			AssertPlacement(L"4k3/8/8/8/8/5n2/4K3/8", game, LINE_INFO());
		}

		TEST_METHOD(AKingCannotMoveOntoASquareAttackedByAPawn)
		{
			// The pawn on D6 attacks E5 and C5, not the square in front of it
			ChessBoard game;
			SetPosition(game, L"4k3/8/3p4/8/4K3/8/8/8");

			AssertRejected(game, L"E4", L"E5", LINE_INFO());

			Play(game, L"E4", L"D5");
			AssertPlacement(L"4k3/8/3p4/3K4/8/8/8/8", game, LINE_INFO());
		}

		TEST_METHOD(AKingCannotMoveNextToTheOpponentsKing)
		{
			ChessBoard game;
			SetPosition(game, L"8/8/8/8/8/2k5/8/K7");

			AssertRejected(game, L"A1", L"B2", LINE_INFO());

			Play(game, L"A1", L"A2");
			AssertPlacement(L"8/8/8/8/8/2k5/K7/8", game, LINE_INFO());
		}

		TEST_METHOD(AKingCanCaptureAnUnprotectedPieceThatChecksIt)
		{
			ChessBoard game;
			SetPosition(game, L"4k3/8/8/8/8/8/4r3/4K3");

			Play(game, L"E1", L"E2");

			AssertPlacement(L"4k3/8/8/8/8/8/4K3/8", game, LINE_INFO());
		}

		TEST_METHOD(AKingCannotCaptureAProtectedPieceThatChecksIt)
		{
			// The bishop on G4 protects the rook on E2
			ChessBoard game;
			SetPosition(game, L"4k3/8/8/8/6b1/8/4r3/4K3");

			AssertRejected(game, L"E1", L"E2", LINE_INFO());

			Play(game, L"E1", L"D1");
			AssertPlacement(L"4k3/8/8/8/6b1/8/4r3/3K4", game, LINE_INFO());
		}

		TEST_METHOD(ACheckMustBeAnswered)
		{
			ChessBoard game;
			SetPosition(game, L"4r2k/8/8/8/8/8/R7/4K3");

			AssertRejected(game, L"A2", L"A3", LINE_INFO());
			AssertRejected(game, L"A2", L"A8", LINE_INFO());
			AssertRejected(game, L"E1", L"E2", LINE_INFO());
		}

		TEST_METHOD(ACheckCanBeAnsweredByBlocking)
		{
			ChessBoard game;
			SetPosition(game, L"4r2k/8/8/8/8/8/R7/4K3");

			Play(game, L"A2", L"E2");

			AssertPlacement(L"4r2k/8/8/8/8/8/4R3/4K3", game, LINE_INFO());
		}

		TEST_METHOD(ACheckCanBeAnsweredByMovingTheKing)
		{
			ChessBoard game;
			SetPosition(game, L"4r2k/8/8/8/8/8/R7/4K3");

			Play(game, L"E1", L"D1");

			AssertPlacement(L"4r2k/8/8/8/8/8/R7/3K4", game, LINE_INFO());
		}

		TEST_METHOD(ACheckCanBeAnsweredByCapturingTheCheckingPiece)
		{
			ChessBoard game;
			SetPosition(game, L"4r2k/8/8/8/B7/8/8/4K3");

			AssertRejected(game, L"A4", L"B5", LINE_INFO());

			Play(game, L"A4", L"E8");
			AssertPlacement(L"4B2k/8/8/8/8/8/8/4K3", game, LINE_INFO());
		}

		TEST_METHOD(ACheckDoesNotEndTheGameWhenItCanBeAnswered)
		{
			// 1.e4 f5 2.Qh5+ : the pawn on G7 is the only piece that can block the check
			ChessBoard game;
			PlayMoves(game, L"E2E4 F7F5 D1H5");
			Assert::IsFalse(game.HasGameEnded());

			AssertRejected(game, L"A7", L"A6", LINE_INFO());
			AssertRejected(game, L"G8", L"F6", LINE_INFO());
			AssertRejected(game, L"E8", L"F7", LINE_INFO());

			Play(game, L"G7", L"G6");
			AssertPlacement(L"rnbqkbnr/ppppp2p/6p1/5p1Q/4P3/8/PPPP1PPP/RNB1KBNR", game, LINE_INFO());
			Assert::IsFalse(game.HasGameEnded());
		}
	};

	TEST_CLASS(GameEndTests)
	{
	public:
		TEST_METHOD(FoolsMate)
		{
			ChessBoard game;
			PlayMoves(game, L"F2F3 E7E5 G2G4");
			Assert::IsFalse(game.HasGameEnded());

			Play(game, L"D8", L"H4");

			Assert::IsTrue(game.HasGameEnded());
			AssertPlacement(L"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR", game, LINE_INFO());
		}

		TEST_METHOD(ScholarsMate)
		{
			ChessBoard game;
			PlayMoves(game, L"E2E4 E7E5 F1C4 B8C6 D1H5 G8F6");
			Assert::IsFalse(game.HasGameEnded());

			Play(game, L"H5", L"F7");

			Assert::IsTrue(game.HasGameEnded());
			AssertPlacement(L"r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR", game, LINE_INFO());
		}

		TEST_METHOD(LegalsMate)
		{
			// The King of Black has no square left: Bxf7 and Nxe5 guard them and the knight on D5 gives the check
			ChessBoard game;
			PlayMoves(game, L"E2E4 E7E5 G1F3 D7D6 F1C4 C8G4 B1C3 G7G6 F3E5 G4D1 C4F7 E8E7");
			Assert::IsFalse(game.HasGameEnded());

			Play(game, L"C3", L"D5");

			Assert::IsTrue(game.HasGameEnded());
			AssertPlacement(L"rn1q1bnr/ppp1kB1p/3p2p1/3NN3/4P3/8/PPPP1PPP/R1BbK2R", game, LINE_INFO());
		}

		TEST_METHOD(BackRankMate)
		{
			ChessBoard game;
			SetPosition(game, L"6k1/5ppp/8/8/8/8/8/R3K3");

			Play(game, L"A1", L"A8");

			Assert::IsTrue(game.HasGameEnded());
		}

		TEST_METHOD(ACheckThatCanBeAnsweredByCapturingIsNotAMate)
		{
			ChessBoard game;
			SetPosition(game, L"6k1/5ppp/8/8/4b3/8/8/R3K3");

			Play(game, L"A1", L"A8");
			Assert::IsFalse(game.HasGameEnded());

			Play(game, L"E4", L"A8");
			AssertPlacement(L"b5k1/5ppp/8/8/8/8/8/4K3", game, LINE_INFO());
			Assert::IsFalse(game.HasGameEnded());
		}

		TEST_METHOD(Stalemate)
		{
			// The King of Black is not in check but every square it could go to is attacked
			ChessBoard game;
			SetPosition(game, L"k7/2K5/3Q4/8/8/8/8/8");

			Play(game, L"D6", L"B6");

			Assert::IsTrue(game.HasGameEnded());
		}

		TEST_METHOD(AKingThatCanStillMoveIsNotStalemated)
		{
			ChessBoard game;
			SetPosition(game, L"k7/2K5/3Q4/8/8/8/8/8");

			Play(game, L"C7", L"C6");

			Assert::IsFalse(game.HasGameEnded());
		}

		TEST_METHOD(AfterTheGameEnded_NoMoveIsAccepted)
		{
			ChessBoard game;
			PlayMoves(game, L"F2F3 E7E5 G2G4 D8H4");
			Assert::IsTrue(game.HasGameEnded());

			AssertRejected(game, L"E2", L"E4", LINE_INFO());
			AssertRejected(game, L"A2", L"A3", LINE_INFO());
			AssertRejected(game, L"H4", L"H3", LINE_INFO());
			AssertRejected(game, L"E5", L"E4", LINE_INFO());
			Assert::IsTrue(game.HasGameEnded());
		}
	};
}
