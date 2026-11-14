// PieceTests.cpp - unit tests of the pieces: their description and the moves of Rook, Bishop, Queen, Knight and King

#include "pch.h"
#include "TestSupport.hpp"

using namespace ChessTests;

namespace
{
	struct PieceFacts
	{
		wchar_t letter;
		const wchar_t* name;
		const wchar_t* graphics;
		int score;
	};

	const PieceFacts PIECES[] =
	{
		{ L'P', L"White's Pawn", L"\x2659", 1 },
		{ L'p', L"Black's Pawn", L"\x265F", 1 },
		{ L'N', L"White's Knight", L"\x2658", 10 },
		{ L'n', L"Black's Knight", L"\x265E", 10 },
		{ L'B', L"White's Bishop", L"\x2657", 10 },
		{ L'b', L"Black's Bishop", L"\x265D", 10 },
		{ L'R', L"White's Rook", L"\x2656", 10 },
		{ L'r', L"Black's Rook", L"\x265C", 10 },
		{ L'Q', L"White's Queen", L"\x2655", 100 },
		{ L'q', L"Black's Queen", L"\x265B", 100 },
		{ L'K', L"White's King", L"\x2654", 1000 },
		{ L'k', L"Black's King", L"\x265A", 1000 },
	};
}

namespace ChessUnittests
{
	TEST_CLASS(PieceInfoTests)
	{
	public:
		TEST_METHOD(ToString_NamesThePlayerAndThePiece)
		{
			for (const PieceFacts& facts : PIECES)
			{
				PiecePtr piece(NewPiece(facts.letter));
				Assert::AreEqual(std::wstring(facts.name), piece->toString(), facts.name, LINE_INFO());
			}
		}

		TEST_METHOD(ToGraphics_ReturnsTheChessSymbol)
		{
			for (const PieceFacts& facts : PIECES)
			{
				PiecePtr piece(NewPiece(facts.letter));
				Assert::AreEqual(std::wstring(facts.graphics), piece->toGraphics(), facts.name, LINE_INFO());
			}
		}

		TEST_METHOD(Score_ValuesThePiecesForTheComputerPlayer)
		{
			for (const PieceFacts& facts : PIECES)
			{
				PiecePtr piece(NewPiece(facts.letter));
				Assert::AreEqual(facts.score, piece->Score(), facts.name, LINE_INFO());
			}
		}

		TEST_METHOD(IsWhitePlayer_TellsWhichPlayerOwnsThePiece)
		{
			for (const PieceFacts& facts : PIECES)
			{
				const bool isWhite = std::iswupper(facts.letter) != 0;
				PiecePtr piece(NewPiece(facts.letter));
				Assert::AreEqual(isWhite, piece->isWhitePlayer(), facts.name, LINE_INFO());
				Assert::AreEqual(std::wstring(isWhite ? L"White's" : L"Black's"), piece->playerToString(), facts.name, LINE_INFO());
			}
		}

		TEST_METHOD(IsKing_IsTrueOnlyForTheKing)
		{
			for (const PieceFacts& facts : PIECES)
			{
				const bool isKing = std::towlower(facts.letter) == L'k';
				PiecePtr piece(NewPiece(facts.letter));
				Assert::AreEqual(isKing, piece->isKing(), facts.name, LINE_INFO());
			}
		}

		TEST_METHOD(Clone_IsANewPieceOfTheSameKindAndPlayer)
		{
			for (const PieceFacts& facts : PIECES)
			{
				PiecePtr piece(NewPiece(facts.letter));
				PiecePtr copy(piece->clone());
				Assert::IsTrue(copy.get() != piece.get(), facts.name, LINE_INFO());
				Assert::AreEqual(facts.letter, LetterOf(copy.get()), facts.name, LINE_INFO());
			}
		}

		TEST_METHOD(EmptyPiece_CannotMoveAndIsWorthNothing)
		{
			Position position(L"8/8/8/8/8/8/8/8");
			EmptyPiece empty(true);

			AssertMove(&empty, L"A1", L"A2", position.Get(), ChessErrHandler::MOVED_EMPTY_PIECE, LINE_INFO());
			AssertMove(&empty, L"D4", L"D4", position.Get(), ChessErrHandler::MOVED_EMPTY_PIECE, LINE_INFO());
			Assert::AreEqual(0, empty.Score());
			Assert::AreEqual(std::wstring(L""), empty.toString());
			Assert::AreEqual(std::wstring(L" "), empty.toGraphics());
		}

		TEST_METHOD(EmptyPiece_CloneIsAnotherEmptyPiece)
		{
			EmptyPiece empty(true);
			PiecePtr copy(empty.clone());
			Assert::IsTrue(copy.get() != &empty);
			Assert::IsNotNull(dynamic_cast<EmptyPiece*>(copy.get()));
		}
	};

	TEST_CLASS(RookTests)
	{
	public:
		TEST_METHOD(MovesAlongItsFileAndRank)
		{
			Position position(L"8/8/8/8/3R4/8/8/8");
			Piece* rook = position.At(L"D4");

			Assert::AreEqual(std::wstring(L"A4 B4 C4 D1 D2 D3 D5 D6 D7 D8 E4 F4 G4 H4"), Destinations(rook, L"D4", position.Get()));
		}

		TEST_METHOD(CannotMoveOffItsLines)
		{
			Position position(L"8/8/8/8/3R4/8/8/8");
			Piece* rook = position.At(L"D4");

			AssertMove(rook, L"D4", L"E5", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(rook, L"D4", L"E6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(rook, L"D4", L"D4", position.Get(), ChessErrHandler::DEST_EQ_SOURCE, LINE_INFO());
		}

		TEST_METHOD(CapturesEnemyPiecesButCannotJumpOverThem)
		{
			Position position(L"8/8/8/3p4/2pRp3/3p4/8/8");
			Piece* rook = position.At(L"D4");

			Assert::AreEqual(std::wstring(L"C4 D3 D5 E4"), Destinations(rook, L"D4", position.Get()));
			AssertMove(rook, L"D4", L"D8", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(rook, L"D4", L"D1", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(rook, L"D4", L"A4", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(rook, L"D4", L"H4", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
		}

		TEST_METHOD(CannotCaptureOrJumpOverFriendlyPieces)
		{
			Position position(L"8/8/8/3P4/2PRP3/3P4/8/8");
			Piece* rook = position.At(L"D4");

			Assert::AreEqual(std::wstring(L""), Destinations(rook, L"D4", position.Get()));
			AssertMove(rook, L"D4", L"D5", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(rook, L"D4", L"D3", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(rook, L"D4", L"C4", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(rook, L"D4", L"E4", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(rook, L"D4", L"D8", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(rook, L"D4", L"A4", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
		}

		TEST_METHOD(IsOnlyStoppedByPiecesBetweenItAndTheDestination)
		{
			// A pawn behind the destination or on another line does not matter
			Position position(L"8/3P4/8/8/3R4/8/8/8");
			Piece* rook = position.At(L"D4");

			AssertMove(rook, L"D4", L"D6", position.Get(), VALID, LINE_INFO());
			AssertMove(rook, L"D4", L"D7", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(rook, L"D4", L"D8", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(rook, L"D4", L"H4", position.Get(), VALID, LINE_INFO());
		}
	};

	TEST_CLASS(BishopTests)
	{
	public:
		TEST_METHOD(MovesAlongItsDiagonals)
		{
			Position position(L"8/8/8/8/3B4/8/8/8");
			Piece* bishop = position.At(L"D4");

			Assert::AreEqual(std::wstring(L"A1 A7 B2 B6 C3 C5 E3 E5 F2 F6 G1 G7 H8"), Destinations(bishop, L"D4", position.Get()));
		}

		TEST_METHOD(CannotMoveAlongFilesRanksOrLikeAKnight)
		{
			Position position(L"8/8/8/8/3B4/8/8/8");
			Piece* bishop = position.At(L"D4");

			AssertMove(bishop, L"D4", L"D6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(bishop, L"D4", L"F4", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(bishop, L"D4", L"E6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(bishop, L"D4", L"D4", position.Get(), ChessErrHandler::DEST_EQ_SOURCE, LINE_INFO());
		}

		TEST_METHOD(CapturesEnemyPiecesButCannotJumpOverThemInAnyDirection)
		{
			Position position(L"8/8/8/2p1p3/3B4/2p1p3/8/8");
			Piece* bishop = position.At(L"D4");

			Assert::AreEqual(std::wstring(L"C3 C5 E3 E5"), Destinations(bishop, L"D4", position.Get()));
			AssertMove(bishop, L"D4", L"A1", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(bishop, L"D4", L"A7", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(bishop, L"D4", L"G1", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(bishop, L"D4", L"H8", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
		}

		TEST_METHOD(CannotCaptureOrJumpOverFriendlyPieces)
		{
			Position position(L"8/8/8/2P1P3/3B4/2P1P3/8/8");
			Piece* bishop = position.At(L"D4");

			Assert::AreEqual(std::wstring(L""), Destinations(bishop, L"D4", position.Get()));
			AssertMove(bishop, L"D4", L"C3", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(bishop, L"D4", L"E5", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(bishop, L"D4", L"B2", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(bishop, L"D4", L"G7", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
		}

		TEST_METHOD(StopsOnlyAtPiecesOnTheDiagonalItUses)
		{
			// The pawn on E5 blocks the way to F6 but not the other diagonals
			Position position(L"8/8/8/4p3/3B4/8/8/8");
			Piece* bishop = position.At(L"D4");

			AssertMove(bishop, L"D4", L"E5", position.Get(), VALID, LINE_INFO());
			AssertMove(bishop, L"D4", L"F6", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(bishop, L"D4", L"A7", position.Get(), VALID, LINE_INFO());
			AssertMove(bishop, L"D4", L"G1", position.Get(), VALID, LINE_INFO());
			AssertMove(bishop, L"D4", L"A1", position.Get(), VALID, LINE_INFO());
		}
	};

	TEST_CLASS(QueenTests)
	{
	public:
		TEST_METHOD(MovesLikeARookAndABishop)
		{
			Position position(L"8/8/8/8/3Q4/8/8/8");
			Piece* queen = position.At(L"D4");

			Assert::AreEqual(27, CountDestinations(queen, L"D4", position.Get()));
			AssertMove(queen, L"D4", L"D8", position.Get(), VALID, LINE_INFO());
			AssertMove(queen, L"D4", L"A4", position.Get(), VALID, LINE_INFO());
			AssertMove(queen, L"D4", L"H8", position.Get(), VALID, LINE_INFO());
			AssertMove(queen, L"D4", L"G1", position.Get(), VALID, LINE_INFO());
		}

		TEST_METHOD(CannotMoveLikeAKnight)
		{
			Position position(L"8/8/8/8/3Q4/8/8/8");
			Piece* queen = position.At(L"D4");

			AssertMove(queen, L"D4", L"E6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(queen, L"D4", L"B3", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(queen, L"D4", L"D4", position.Get(), ChessErrHandler::DEST_EQ_SOURCE, LINE_INFO());
		}

		TEST_METHOD(CapturesEnemyPiecesButCannotJumpOverThemOnAnyLine)
		{
			Position position(L"8/8/3p1p2/8/3Q1p2/8/8/8");
			Piece* queen = position.At(L"D4");

			AssertMove(queen, L"D4", L"D6", position.Get(), VALID, LINE_INFO());
			AssertMove(queen, L"D4", L"D8", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(queen, L"D4", L"F4", position.Get(), VALID, LINE_INFO());
			AssertMove(queen, L"D4", L"H4", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(queen, L"D4", L"F6", position.Get(), VALID, LINE_INFO());
			AssertMove(queen, L"D4", L"G7", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
		}

		TEST_METHOD(CannotCaptureOrJumpOverFriendlyPieces)
		{
			Position position(L"8/8/3P1P2/8/3Q1P2/8/8/8");
			Piece* queen = position.At(L"D4");

			AssertMove(queen, L"D4", L"D6", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(queen, L"D4", L"D8", position.Get(), ChessErrHandler::OBSTRUCTION_EN_ROUTE, LINE_INFO());
			AssertMove(queen, L"D4", L"F4", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(queen, L"D4", L"F6", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			AssertMove(queen, L"D4", L"D5", position.Get(), VALID, LINE_INFO());
		}
	};

	TEST_CLASS(KnightTests)
	{
	public:
		TEST_METHOD(MovesInAnLShape)
		{
			Position position(L"8/8/8/8/3N4/8/8/8");
			Piece* knight = position.At(L"D4");

			Assert::AreEqual(std::wstring(L"B3 B5 C2 C6 E2 E6 F3 F5"), Destinations(knight, L"D4", position.Get()));
		}

		TEST_METHOD(StaysOnTheBoardInACorner)
		{
			Position position(L"8/8/8/8/8/8/8/N7");
			Piece* knight = position.At(L"A1");

			Assert::AreEqual(std::wstring(L"B3 C2"), Destinations(knight, L"A1", position.Get()));
		}

		TEST_METHOD(CannotMoveInOtherPatterns)
		{
			Position position(L"8/8/8/8/3N4/8/8/8");
			Piece* knight = position.At(L"D4");

			AssertMove(knight, L"D4", L"D6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(knight, L"D4", L"F6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(knight, L"D4", L"E5", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(knight, L"D4", L"F4", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(knight, L"D4", L"D4", position.Get(), ChessErrHandler::DEST_EQ_SOURCE, LINE_INFO());
		}

		TEST_METHOD(JumpsOverSurroundingPieces)
		{
			Position position(L"8/8/8/2PPP3/2PNP3/2PPP3/8/8");
			Piece* knight = position.At(L"D4");

			Assert::AreEqual(std::wstring(L"B3 B5 C2 C6 E2 E6 F3 F5"), Destinations(knight, L"D4", position.Get()));
		}

		TEST_METHOD(CapturesEnemyPiecesButNotFriendlyOnes)
		{
			Position position(L"8/8/8/5p2/3N4/5P2/8/8");
			Piece* knight = position.At(L"D4");

			AssertMove(knight, L"D4", L"F5", position.Get(), VALID, LINE_INFO());
			AssertMove(knight, L"D4", L"F3", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			Assert::AreEqual(std::wstring(L"B3 B5 C2 C6 E2 E6 F5"), Destinations(knight, L"D4", position.Get()));
		}
	};

	TEST_CLASS(KingTests)
	{
	public:
		TEST_METHOD(MovesOneSquareInAnyDirection)
		{
			Position position(L"8/8/8/8/3K4/8/8/8");
			Piece* king = position.At(L"D4");

			Assert::AreEqual(std::wstring(L"C3 C4 C5 D3 D5 E3 E4 E5"), Destinations(king, L"D4", position.Get()));
		}

		TEST_METHOD(StaysOnTheBoardInACorner)
		{
			Position position(L"8/8/8/8/8/8/8/K7");
			Piece* king = position.At(L"A1");

			Assert::AreEqual(std::wstring(L"A2 B1 B2"), Destinations(king, L"A1", position.Get()));
		}

		TEST_METHOD(CannotMoveTwoSquares)
		{
			Position position(L"8/8/8/8/3K4/8/8/8");
			Piece* king = position.At(L"D4");

			AssertMove(king, L"D4", L"D6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(king, L"D4", L"F4", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(king, L"D4", L"F6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(king, L"D4", L"E6", position.Get(), ChessErrHandler::ILLEGAL_MOVE_PATTERN, LINE_INFO());
			AssertMove(king, L"D4", L"D4", position.Get(), ChessErrHandler::DEST_EQ_SOURCE, LINE_INFO());
		}

		TEST_METHOD(CapturesEnemyPiecesButNotFriendlyOnes)
		{
			Position position(L"8/8/8/2p5/3K4/3P4/8/8");
			Piece* king = position.At(L"D4");

			AssertMove(king, L"D4", L"C5", position.Get(), VALID, LINE_INFO());
			AssertMove(king, L"D4", L"D3", position.Get(), ChessErrHandler::FRIENDLY_AT_DEST, LINE_INFO());
			Assert::AreEqual(std::wstring(L"C3 C4 C5 D5 E3 E4 E5"), Destinations(king, L"D4", position.Get()));
		}
	};
}
