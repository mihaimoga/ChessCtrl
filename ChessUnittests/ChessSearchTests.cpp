// ChessSearchTests.cpp - unit tests of the search that the computer player plays with: the moves of a position, its value
// and the choice of the best move
//
// The number of lines of the positions that are counted (see MoveGeneratorTests) were checked with another chess library.
// The engine has no castling, no capture en passant and no promotion, so only the positions where they do not happen
// within the plies counted are compared with the numbers of chess.

#include "pch.h"
#include "TestSupport.hpp"
#include "ChessSearch.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace ChessTests;

namespace
{
	typedef ChessSearch::Position SearchPosition;
	typedef ChessSearch::Move SearchMove;
	typedef ChessSearch::Result SearchResult;

	// A middle game, and then positions with a pin, a check to answer and pawns about to reach the last rank
	const wchar_t* const MIDDLE_GAME = L"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1";

	const wchar_t* const POSITIONS[] =
	{
		INITIAL_POSITION,
		L"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1",
		L"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R",
		L"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8",
		L"r3k1n1/4b3/8/8/7Q/8/8/R3R2K",
		L"4k3/P5P1/8/8/8/8/p5p1/4K3",
		L"4k3/8/8/8/8/8/4r3/R3K3",
	};

	// The moves of a list in alphabetical order separated by spaces, like L"E2E3 E2E4"
	std::wstring Names(const std::vector<SearchMove>& moves)
	{
		std::set<std::wstring> names;
		for (const SearchMove& move : moves)
		{
			names.insert(SearchMoveName(move));
		}

		std::wstring list;
		for (const std::wstring& name : names)
		{
			if (!list.empty())
			{
				list += L' ';
			}
			list += name;
		}
		return list;
	}

	// The legal moves of the piece standing on a square
	std::wstring MovesFrom(const SearchPosition& position, const wchar_t* square)
	{
		std::vector<SearchMove> moves;
		for (const SearchMove& move : position.LegalMoves())
		{
			if (move.nFrom == SearchSquareOf(square))
			{
				moves.push_back(move);
			}
		}
		return Names(moves);
	}

	SearchResult Search(const SearchPosition& position, const int depth, const uint64_t maxNodes = 0)
	{
		ChessSearch::Limits limits;
		limits.nMaxDepth = depth;
		limits.nMaxNodes = maxNodes;
		return ChessSearch::FindBestMove(position, std::vector<SearchMove>(), limits);
	}

	// The position after a move, which has to be legal
	SearchPosition After(const SearchPosition& position, const SearchMove& move)
	{
		SearchPosition next(position);
		ChessSearch::Undo undo;
		Assert::IsTrue(next.MakeMove(move, undo), (L"Illegal move " + SearchMoveName(move)).c_str());
		return next;
	}

	bool IsCheckmate(const SearchPosition& position)
	{
		return position.IsInCheck() && position.LegalMoves().empty();
	}

	// True when the player to move has a move that gives checkmate, found without the search
	bool HasCheckmateInOneMove(const SearchPosition& position)
	{
		for (const SearchMove& move : position.LegalMoves())
		{
			if (IsCheckmate(After(position, move)))
			{
				return true;
			}
		}
		return false;
	}

	// The position after the player to move passes: what its opponent could do if it was not answered
	SearchPosition Passed(const SearchPosition& position)
	{
		SearchPosition next(position);
		ChessSearch::Undo undo;
		next.MakeNullMove(undo);
		return next;
	}

	// The pieces of a position put one by one on an empty board: it has the hash that a position has whatever moves led to it
	SearchPosition Rebuilt(const SearchPosition& position)
	{
		SearchPosition rebuilt;
		for (int square = 0; square < ChessSearch::SQUARE_COUNT; ++square)
		{
			const int piece = position.PieceAt(square);
			if (piece != 0)
			{
				rebuilt.Put(square, std::abs(piece), piece > 0);
			}
		}
		rebuilt.SetWhiteToMove(position.IsWhiteToMove());
		return rebuilt;
	}

	// The position seen from the other side of the board: the ranks are reversed, the pieces change color and so does the player to move
	SearchPosition Mirror(const SearchPosition& position)
	{
		SearchPosition mirror;
		for (int square = 0; square < ChessSearch::SQUARE_COUNT; ++square)
		{
			const int piece = position.PieceAt(square);
			if (piece != 0)
			{
				// Reversing the rank of a square is reversing the three bits of its number that hold the rank
				mirror.Put(square ^ (ChessSearch::SQUARE_COUNT - FILES_PER_RANK), std::abs(piece), piece < 0);
			}
		}
		mirror.SetWhiteToMove(!position.IsWhiteToMove());
		return mirror;
	}

	// The squares of a list like L"A2 A4" that a player attacks, and the squares of another list that it does not attack
	void AssertAttacks(const wchar_t* placement, const bool white, const wchar_t* attacked, const wchar_t* notAttacked)
	{
		const SearchPosition position = ParseSearchPosition(placement, white);

		std::wistringstream attackedSquares(attacked);
		std::wstring square;
		while (attackedSquares >> square)
		{
			const std::wstring message = square + L" should be attacked in " + placement;
			Assert::IsTrue(position.IsAttacked(SearchSquareOf(square.c_str()), white), message.c_str());
		}

		std::wistringstream otherSquares(notAttacked);
		while (otherSquares >> square)
		{
			const std::wstring message = square + L" should not be attacked in " + placement;
			Assert::IsFalse(position.IsAttacked(SearchSquareOf(square.c_str()), white), message.c_str());
		}
	}

	// A mate in two: the move gives a position where every answer of the opponent can be followed by a checkmate
	void AssertForcedMateInTwo(const SearchPosition& position, const SearchMove& move)
	{
		const SearchPosition afterMove = After(position, move);
		const std::vector<SearchMove> answers = afterMove.LegalMoves();
		Assert::IsFalse(answers.empty(), L"The move ends the game at once");
		for (const SearchMove& answer : answers)
		{
			const std::wstring message = SearchMoveName(move) + L" " + SearchMoveName(answer) + L" is not followed by a checkmate";
			Assert::IsTrue(HasCheckmateInOneMove(After(afterMove, answer)), message.c_str());
		}
	}
}

namespace ChessUnittests
{
	TEST_CLASS(ChessSearchMoveTests)
	{
	public:
		TEST_METHOD(TheInitialPositionHasTheNumberOfLinesOfChess)
		{
			const SearchPosition position = ParseSearchPosition(INITIAL_POSITION, true);

			Assert::AreEqual<uint64_t>(20, position.Perft(1), L"1 ply", LINE_INFO());
			Assert::AreEqual<uint64_t>(400, position.Perft(2), L"2 plies", LINE_INFO());
			Assert::AreEqual<uint64_t>(8902, position.Perft(3), L"3 plies", LINE_INFO());
			Assert::AreEqual<uint64_t>(197281, position.Perft(4), L"4 plies", LINE_INFO());
		}

		TEST_METHOD(AMiddleGameHasTheNumberOfLinesOfChess)
		{
			const SearchPosition position = ParseSearchPosition(MIDDLE_GAME, true);

			Assert::AreEqual<uint64_t>(46, position.Perft(1), L"1 ply", LINE_INFO());
			Assert::AreEqual<uint64_t>(2079, position.Perft(2), L"2 plies", LINE_INFO());
			Assert::AreEqual<uint64_t>(89890, position.Perft(3), L"3 plies", LINE_INFO());
		}

		TEST_METHOD(APositionWithPinsAndChecksHasTheNumberOfLinesOfChess)
		{
			const SearchPosition position = ParseSearchPosition(L"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8", true);

			Assert::AreEqual<uint64_t>(14, position.Perft(1), L"1 ply", LINE_INFO());
			Assert::AreEqual<uint64_t>(191, position.Perft(2), L"2 plies", LINE_INFO());

			// Chess has 2812 lines of 3 plies: 2 of them end with a capture en passant, which the engine does not have
			Assert::AreEqual<uint64_t>(2810, position.Perft(3), L"3 plies", LINE_INFO());
		}

		TEST_METHOD(APawnAdvancesTwoSquaresOnlyFromItsStartingRank)
		{
			const SearchPosition white = ParseSearchPosition(L"4k3/8/8/8/P7/8/4P3/4K3", true);
			Assert::AreEqual(std::wstring(L"E2E3 E2E4"), MovesFrom(white, L"E2"), L"Pawn of White on its starting rank", LINE_INFO());
			Assert::AreEqual(std::wstring(L"A4A5"), MovesFrom(white, L"A4"), L"Pawn of White that has moved", LINE_INFO());

			const SearchPosition black = ParseSearchPosition(L"4k3/4p3/8/7p/8/8/8/4K3", false);
			Assert::AreEqual(std::wstring(L"E7E5 E7E6"), MovesFrom(black, L"E7"), L"Pawn of Black on its starting rank", LINE_INFO());
			Assert::AreEqual(std::wstring(L"H5H4"), MovesFrom(black, L"H5"), L"Pawn of Black that has moved", LINE_INFO());
		}

		TEST_METHOD(APawnNeitherJumpsOverAPieceNorCapturesAheadOfIt)
		{
			const SearchPosition position = ParseSearchPosition(L"4k3/8/8/8/8/4n3/4P3/4K3", true);

			Assert::AreEqual(std::wstring(), MovesFrom(position, L"E2"), L"Pawn that is blocked", LINE_INFO());
		}

		TEST_METHOD(APawnCapturesDiagonally)
		{
			const SearchPosition position = ParseSearchPosition(L"4k3/8/8/8/8/3p1p2/4P3/4K3", true);

			Assert::AreEqual(std::wstring(L"E2D3 E2E3 E2E4 E2F3"), MovesFrom(position, L"E2"), L"Pawn that can capture on both sides", LINE_INFO());
		}

		TEST_METHOD(APawnReachesTheLastRankAndCannotMoveAnyMore)
		{
			const SearchPosition seventh = ParseSearchPosition(L"4k3/P7/8/8/8/8/8/4K3", true);
			Assert::AreEqual(std::wstring(L"A7A8"), MovesFrom(seventh, L"A7"), L"Pawn of White on the 7th rank", LINE_INFO());

			const SearchPosition white = ParseSearchPosition(L"P3k3/8/8/8/8/8/8/4K3", true);
			Assert::AreEqual(std::wstring(), MovesFrom(white, L"A8"), L"Pawn of White on the 8th rank", LINE_INFO());

			const SearchPosition black = ParseSearchPosition(L"4k3/8/8/8/8/8/8/p3K3", false);
			Assert::AreEqual(std::wstring(), MovesFrom(black, L"A1"), L"Pawn of Black on the 1st rank", LINE_INFO());
		}

		TEST_METHOD(ThereIsNeitherCastlingNorCaptureEnPassant)
		{
			// The king and the rooks stand where they could castle, but the king moves one square
			const SearchPosition castling = ParseSearchPosition(L"r3k2r/8/8/8/8/8/8/R3K2R", true);
			Assert::AreEqual(std::wstring(L"E1D1 E1D2 E1E2 E1F1 E1F2"), MovesFrom(castling, L"E1"), L"King between its rooks", LINE_INFO());

			// The pawn of Black stands next to the pawn of White as if it had just advanced two squares
			const SearchPosition enPassant = ParseSearchPosition(L"4k3/8/8/3pP3/8/8/8/4K3", true);
			Assert::AreEqual(std::wstring(L"E5E6"), MovesFrom(enPassant, L"E5"), L"Pawn next to a pawn of the opponent", LINE_INFO());
		}

		TEST_METHOD(AKingDoesNotMoveIntoCheck)
		{
			const SearchPosition position = ParseSearchPosition(L"4k3/8/8/8/8/8/r7/4K3", true);

			Assert::AreEqual(std::wstring(L"E1D1 E1F1"), MovesFrom(position, L"E1"), L"King above a rook", LINE_INFO());
		}

		TEST_METHOD(APinnedPieceStaysOnTheLineOfThePin)
		{
			const SearchPosition knight = ParseSearchPosition(L"4k3/4r3/8/8/8/8/4N3/4K3", true);
			Assert::AreEqual(std::wstring(), MovesFrom(knight, L"E2"), L"Pinned knight", LINE_INFO());

			const SearchPosition rook = ParseSearchPosition(L"4k3/4r3/8/8/8/8/4R3/4K3", true);
			Assert::AreEqual(std::wstring(L"E2E3 E2E4 E2E5 E2E6 E2E7"), MovesFrom(rook, L"E2"), L"Pinned rook", LINE_INFO());
		}

		TEST_METHOD(ACheckIsAnswered)
		{
			// The rook of Black next to the king cannot be blocked, the king moves away or takes it
			const SearchPosition position = ParseSearchPosition(L"4k3/8/8/8/8/8/4r3/R3K3", true);

			Assert::IsTrue(position.IsInCheck());
			Assert::AreEqual(std::wstring(L"E1D1 E1E2 E1F1"), MovesFrom(position, L"E1"), L"King", LINE_INFO());
			Assert::AreEqual(std::wstring(), MovesFrom(position, L"A1"), L"Rook", LINE_INFO());
		}

		TEST_METHOD(ACheckmateAndAStalemateHaveNoMove)
		{
			// The queen is protected by the king, so the king of Black cannot take it
			const SearchPosition checkmate = ParseSearchPosition(L"7k/6Q1/5K2/8/8/8/8/8", false);
			Assert::IsTrue(checkmate.IsInCheck(), L"Checkmate: the king is attacked");
			Assert::IsTrue(checkmate.LegalMoves().empty(), L"Checkmate: no move");

			// The king of Black is not attacked but every square it could go to is
			const SearchPosition stalemate = ParseSearchPosition(L"k7/2Q5/1K6/8/8/8/8/8", false);
			Assert::IsFalse(stalemate.IsInCheck(), L"Stalemate: the king is not attacked");
			Assert::IsTrue(stalemate.LegalMoves().empty(), L"Stalemate: no move");
		}

		TEST_METHOD(AMoveThatLeavesTheKingAttackedIsRefusedAndChangesNothing)
		{
			// The knight shields its king from the rook
			SearchPosition position = ParseSearchPosition(L"4k3/4r3/8/8/8/8/4N3/4K3", true);
			const SearchPosition before(position);

			ChessSearch::Undo undo;
			Assert::IsFalse(position.MakeMove(SearchMoveOf(L"E2C3"), undo));
			Assert::IsTrue(SameSearchPosition(before, position));
		}

		TEST_METHOD(UnmakingAMoveGivesThePositionBack)
		{
			for (const wchar_t* placement : POSITIONS)
			{
				for (int side = 0; side < 2; ++side)
				{
					SearchPosition position = ParseSearchPosition(placement, side == 0);
					const SearchPosition before(position);
					const std::wstring where = std::wstring(placement) + ((side == 0) ? L" with White to move" : L" with Black to move");

					SearchMove moves[ChessSearch::MAX_MOVES];
					const int count = position.GenerateMoves(moves, false);
					for (int index = 0; index < count; ++index)
					{
						const SearchMove& move = moves[index];
						const std::wstring message = where + L", move " + SearchMoveName(move);

						ChessSearch::Undo undo;
						if (position.MakeMove(move, undo))
						{
							Assert::IsTrue(position.IsWhiteToMove() != before.IsWhiteToMove(), message.c_str(), LINE_INFO());
							Assert::AreEqual(before.PieceAt(move.nFrom), position.PieceAt(move.nTo), message.c_str(), LINE_INFO());
							Assert::AreEqual(0, position.PieceAt(move.nFrom), message.c_str(), LINE_INFO());
							Assert::IsTrue(Rebuilt(position).Hash() == position.Hash(), message.c_str(), LINE_INFO());
							position.UnmakeMove(move, undo);
						}
						Assert::IsTrue(SameSearchPosition(before, position), message.c_str(), LINE_INFO());
					}
				}
			}
		}

		TEST_METHOD(APositionHasTheSameHashWhateverTheOrderOfTheMoves)
		{
			const wchar_t* const first[] = { L"G1F3", L"G8F6", L"B1C3", L"B8C6" };
			const wchar_t* const second[] = { L"B1C3", L"B8C6", L"G1F3", L"G8F6" };

			SearchPosition one = ParseSearchPosition(INITIAL_POSITION, true);
			for (const wchar_t* name : first)
			{
				ChessSearch::Undo undo;
				Assert::IsTrue(one.MakeMove(SearchMoveOf(name), undo), name);
			}
			SearchPosition other = ParseSearchPosition(INITIAL_POSITION, true);
			for (const wchar_t* name : second)
			{
				ChessSearch::Undo undo;
				Assert::IsTrue(other.MakeMove(SearchMoveOf(name), undo), name);
			}
			Assert::IsTrue(SameSearchPosition(one, other));

			SearchPosition different = ParseSearchPosition(INITIAL_POSITION, true);
			ChessSearch::Undo undo;
			Assert::IsTrue(different.MakeMove(SearchMoveOf(L"E2E4"), undo));
			Assert::IsTrue(different.Hash() != one.Hash());
		}

		TEST_METHOD(ANullMoveGivesTheTurnToTheOpponentAndIsTakenBack)
		{
			SearchPosition position = ParseSearchPosition(MIDDLE_GAME, true);
			const SearchPosition before(position);

			ChessSearch::Undo undo;
			position.MakeNullMove(undo);
			Assert::IsFalse(position.IsWhiteToMove());
			Assert::IsTrue(position.Hash() != before.Hash());
			Assert::IsTrue(Rebuilt(position).Hash() == position.Hash());

			position.UnmakeNullMove(undo);
			Assert::IsTrue(SameSearchPosition(before, position));
		}

		TEST_METHOD(APawnAttacksTheSquaresDiagonallyAheadOfIt)
		{
			AssertAttacks(L"4k3/8/8/4p3/4P3/8/8/4K3", true, L"D5 F5", L"E5 D3 F3 D4");
			AssertAttacks(L"4k3/8/8/4p3/4P3/8/8/4K3", false, L"D4 F4", L"E4 D6 F6 D5");
		}

		TEST_METHOD(AKnightAttacksTheSquaresAKnightsMoveAway)
		{
			AssertAttacks(L"4k3/8/8/8/8/2N5/8/4K3", true, L"A2 A4 B1 B5 D1 D5 E2 E4", L"C4 D3 B3 C5");
		}

		TEST_METHOD(ABishopAttacksAlongADiagonalUpToTheFirstPiece)
		{
			AssertAttacks(L"4k3/8/8/8/8/2p5/8/B3K3", true, L"B2 C3", L"D4 E5 A2 B1");
		}

		TEST_METHOD(ARookAttacksAlongALineUpToTheFirstPiece)
		{
			AssertAttacks(L"4k3/8/8/8/R2p4/8/8/4K3", true, L"A1 A8 B4 C4 D4", L"E4 B5 B3");
		}

		TEST_METHOD(AQueenAttacksAlongALineAndADiagonal)
		{
			AssertAttacks(L"4k3/8/8/8/3Q4/8/8/4K3", true, L"D8 A7 G7 H4 G1 A1 A4 D1 B6", L"C6 E6 B5 H3");
		}

		TEST_METHOD(AKingAttacksTheSquaresNextToIt)
		{
			AssertAttacks(L"4k3/8/8/8/8/8/8/4K3", true, L"D1 D2 E2 F1 F2", L"E3 C1 G1 C2");
			AssertAttacks(L"4k3/8/8/8/8/8/8/4K3", false, L"D8 D7 E7 F8 F7", L"E6 C8 G8 C7");
		}
	};

	TEST_CLASS(ChessSearchEvaluationTests)
	{
	public:
		TEST_METHOD(AMirroredPositionHasTheSameValue)
		{
			for (const wchar_t* placement : POSITIONS)
			{
				for (int side = 0; side < 2; ++side)
				{
					const SearchPosition position = ParseSearchPosition(placement, side == 0);
					const std::wstring message = std::wstring(L"Position ") + placement;

					Assert::AreEqual(position.Evaluate(), Mirror(position).Evaluate(), message.c_str(), LINE_INFO());
				}
			}
		}

		TEST_METHOD(TheValueIsForThePlayerToMove)
		{
			// White has a queen more
			const wchar_t* const placement = L"4k3/8/8/8/8/8/8/3QK3";

			Assert::IsTrue(ParseSearchPosition(placement, true).Evaluate() > 0, L"White to move");
			Assert::IsTrue(ParseSearchPosition(placement, false).Evaluate() < 0, L"Black to move");
		}

		TEST_METHOD(AQueenIsWorthMoreThanARookAndARookMoreThanAPawn)
		{
			const int pawn = ParseSearchPosition(L"4k3/8/8/8/8/8/4P3/4K3", true).Evaluate();
			const int rook = ParseSearchPosition(L"4k3/8/8/8/8/8/8/3RK3", true).Evaluate();
			const int queen = ParseSearchPosition(L"4k3/8/8/8/8/8/8/3QK3", true).Evaluate();

			Assert::IsTrue((pawn > 50) && (pawn < 200), L"A pawn is worth about a hundred centipawns");
			Assert::IsTrue(rook > pawn + 200, L"A rook is worth more than a pawn");
			Assert::IsTrue(queen > rook + 200, L"A queen is worth more than a rook");
		}

		TEST_METHOD(AKingAndAMinorPieceCannotGiveCheckmateSoTheyAreADraw)
		{
			const wchar_t* const placements[] =
			{
				L"4k3/8/8/8/8/8/8/4K3",
				L"4k3/8/8/8/8/8/8/3BK3",
				L"4k3/8/8/8/8/8/8/3NK3",
				L"3bk3/8/8/8/8/8/8/3BK3",
				L"3nk3/8/8/8/8/8/8/3BK3",
			};
			for (const wchar_t* placement : placements)
			{
				const std::wstring message = std::wstring(L"Position ") + placement;

				Assert::AreEqual(0, ParseSearchPosition(placement, true).Evaluate(), message.c_str(), LINE_INFO());
				Assert::AreEqual(0, ParseSearchPosition(placement, false).Evaluate(), message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(ARookIsEnoughToWinAgainstABareKing)
		{
			Assert::IsTrue(ParseSearchPosition(L"4k3/8/8/8/8/8/8/R3K3", true).Evaluate() > 400);
		}

		TEST_METHOD(ABareKingIsBetterOffInTheCenterThanInACorner)
		{
			// White has a rook, the king of Black is pushed to the edge of the board
			const int corner = ParseSearchPosition(L"k7/8/8/8/8/8/8/R3K3", true).Evaluate();
			const int center = ParseSearchPosition(L"8/8/8/3k4/8/8/8/R3K3", true).Evaluate();

			Assert::IsTrue(corner > center);
		}

		TEST_METHOD(ABishopAndAKnightDriveTheKingToACornerOfTheColorOfTheBishop)
		{
			// The king of Black is in A8: the bishop of D1 is on a square of the color of A8, the one of C1 is not
			const int rightCorner = ParseSearchPosition(L"k7/8/8/8/8/8/8/3B1KN1", true).Evaluate();
			const int wrongCorner = ParseSearchPosition(L"k7/8/8/8/8/8/8/2B2KN1", true).Evaluate();

			Assert::IsTrue(rightCorner > wrongCorner + 40, L"The checkmate is in the corner of the color of the bishop");
		}
	};

	TEST_CLASS(ChessSearchPlayTests)
	{
	public:
		TEST_METHOD(White_GivesCheckmateInOneMove)
		{
			// The pawns of Black shut its king in on the 8th rank
			const SearchPosition position = ParseSearchPosition(L"6k1/5ppp/8/8/8/8/8/R5K1", true);

			const SearchResult result = Search(position, 3);

			Assert::IsTrue(result.bFound);
			Assert::AreEqual(std::wstring(L"A1A8"), SearchMoveName(result.move), L"The checkmate", LINE_INFO());
			Assert::AreEqual(ChessSearch::MATE_SCORE - 1, result.nScore, L"A checkmate on the next ply", LINE_INFO());
			Assert::IsTrue(IsCheckmate(After(position, result.move)));
		}

		TEST_METHOD(Black_GivesCheckmateInOneMove)
		{
			const SearchPosition position = ParseSearchPosition(L"r5k1/8/8/8/8/8/5PPP/6K1", false);

			const SearchResult result = Search(position, 3);

			Assert::IsTrue(result.bFound);
			Assert::AreEqual(std::wstring(L"A8A1"), SearchMoveName(result.move), L"The checkmate", LINE_INFO());
			Assert::AreEqual(ChessSearch::MATE_SCORE - 1, result.nScore, L"A checkmate on the next ply", LINE_INFO());
			Assert::IsTrue(IsCheckmate(After(position, result.move)));
		}

		TEST_METHOD(White_GivesCheckmateInTwoMoves)
		{
			// One rook cuts the king of Black off from the 7th rank, the other gives the checkmate on the 8th rank
			const SearchPosition position = ParseSearchPosition(L"7k/8/8/8/8/8/1R6/R5K1", true);

			const SearchResult result = Search(position, 5);

			Assert::IsTrue(result.bFound);
			Assert::AreEqual(ChessSearch::MATE_SCORE - 3, result.nScore, L"A checkmate on the 3rd ply", LINE_INFO());
			AssertForcedMateInTwo(position, result.move);
		}

		TEST_METHOD(Black_GivesCheckmateInTwoMoves)
		{
			const SearchPosition position = ParseSearchPosition(L"r5k1/1r6/8/8/8/8/8/7K", false);

			const SearchResult result = Search(position, 5);

			Assert::IsTrue(result.bFound);
			Assert::AreEqual(ChessSearch::MATE_SCORE - 3, result.nScore, L"A checkmate on the 3rd ply", LINE_INFO());
			AssertForcedMateInTwo(position, result.move);
		}

		TEST_METHOD(Black_AnswersTheThreatOfACheckmate)
		{
			// The rook of White threatens the checkmate on A8, and the move of Black has to prevent it
			const SearchPosition position = ParseSearchPosition(L"6k1/5ppp/8/8/8/8/8/R5K1", false);
			Assert::IsTrue(HasCheckmateInOneMove(Passed(position)), L"There is a threat to answer");

			const SearchResult result = Search(position, 4);

			Assert::IsTrue(result.bFound);
			const std::wstring message = L"White gives checkmate after " + SearchMoveName(result.move);
			Assert::IsFalse(HasCheckmateInOneMove(After(position, result.move)), message.c_str(), LINE_INFO());
		}

		TEST_METHOD(White_AnswersTheThreatOfACheckmate)
		{
			const SearchPosition position = ParseSearchPosition(L"r5k1/8/8/8/8/8/5PPP/6K1", true);
			Assert::IsTrue(HasCheckmateInOneMove(Passed(position)), L"There is a threat to answer");

			const SearchResult result = Search(position, 4);

			Assert::IsTrue(result.bFound);
			const std::wstring message = L"Black gives checkmate after " + SearchMoveName(result.move);
			Assert::IsFalse(HasCheckmateInOneMove(After(position, result.move)), message.c_str(), LINE_INFO());
		}

		TEST_METHOD(White_WinsARookWithAFork)
		{
			// The knight attacks the king and the rook at once, the pawn keeps White from a draw with a lone knight
			const SearchPosition position = ParseSearchPosition(L"r3k3/8/8/3N4/8/8/7P/4K3", true);

			const SearchResult result = Search(position, 3);

			Assert::IsTrue(result.bFound);
			Assert::AreEqual(std::wstring(L"D5C7"), SearchMoveName(result.move), L"The fork", LINE_INFO());
			Assert::IsTrue(result.nScore > 300, L"The rook is won");
		}

		TEST_METHOD(Black_WinsARookWithAFork)
		{
			const SearchPosition position = ParseSearchPosition(L"4k3/7p/8/8/3n4/8/8/R3K3", false);

			const SearchResult result = Search(position, 3);

			Assert::IsTrue(result.bFound);
			Assert::AreEqual(std::wstring(L"D4C2"), SearchMoveName(result.move), L"The fork", LINE_INFO());
			Assert::IsTrue(result.nScore > 300, L"The rook is won");
		}

		TEST_METHOD(DoesNotTakeAProtectedPawnWithTheQueen)
		{
			const SearchPosition position = ParseSearchPosition(L"4k3/8/2p5/3p4/8/8/8/3QK3", true);

			const SearchResult result = Search(position, 4);

			Assert::IsTrue(result.bFound);
			Assert::IsTrue(SearchMoveName(result.move) != L"D1D5", L"The queen is lost for a pawn");
			Assert::IsTrue(result.nScore > 400, L"White is a queen up against two pawns");
		}

		TEST_METHOD(TheMoveIsOneOfTheMovesItIsGiven)
		{
			// The checkmate of A1A8 is better than the move of the king, but only the move of the king is given
			const SearchPosition position = ParseSearchPosition(L"6k1/5ppp/8/8/8/8/8/R5K1", true);
			ChessSearch::Limits limits;
			limits.nMaxDepth = 3;

			const SearchResult result = ChessSearch::FindBestMove(position, std::vector<SearchMove>{ SearchMoveOf(L"G1H1") }, limits);

			Assert::IsTrue(result.bFound);
			Assert::AreEqual(std::wstring(L"G1H1"), SearchMoveName(result.move), L"The only move given", LINE_INFO());
		}

		TEST_METHOD(AMoveThatLeavesTheKingAttackedIsNotChosenEvenWhenItIsGiven)
		{
			// The pinned knight cannot move: there is nothing to choose
			const SearchPosition position = ParseSearchPosition(L"4k3/4r3/8/8/8/8/4N3/4K3", true);
			ChessSearch::Limits limits;
			limits.nMaxDepth = 3;

			const SearchResult result = ChessSearch::FindBestMove(position, std::vector<SearchMove>{ SearchMoveOf(L"E2C3") }, limits);

			Assert::IsFalse(result.bFound);
			Assert::IsFalse(result.bStopped);
		}

		TEST_METHOD(ThereIsNoMoveInACheckmateOrAStalemate)
		{
			const wchar_t* const placements[] = { L"7k/6Q1/5K2/8/8/8/8/8", L"k7/2Q5/1K6/8/8/8/8/8" };
			for (const wchar_t* placement : placements)
			{
				const std::wstring message = std::wstring(L"Position ") + placement;
				const SearchResult result = Search(ParseSearchPosition(placement, false), 4);

				Assert::IsFalse(result.bFound, message.c_str(), LINE_INFO());
				Assert::IsFalse(result.bStopped, message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(TheMoveIsAlwaysALegalMove)
		{
			for (const wchar_t* placement : POSITIONS)
			{
				for (int side = 0; side < 2; ++side)
				{
					const SearchPosition position = ParseSearchPosition(placement, side == 0);
					const std::vector<SearchMove> legal = position.LegalMoves();
					if (legal.empty())
					{
						continue;
					}
					const std::wstring message = std::wstring(L"Position ") + placement + ((side == 0) ? L" with White to move" : L" with Black to move");

					const SearchResult result = Search(position, 3, 50000);

					Assert::IsTrue(result.bFound, message.c_str(), LINE_INFO());
					Assert::IsTrue(std::find(legal.begin(), legal.end(), result.move) != legal.end(), message.c_str(), LINE_INFO());
				}
			}
		}

		TEST_METHOD(ADeeperSearchSeesFurther)
		{
			// A checkmate in two moves is 3 plies away, and the first move that leads to it gives no check
			const SearchPosition position = ParseSearchPosition(L"7k/8/8/8/8/8/1R6/R5K1", true);

			const SearchResult shallow = Search(position, 1);
			const SearchResult deep = Search(position, 3);

			Assert::IsTrue(shallow.bFound);
			Assert::AreEqual(1, shallow.nDepth, L"The depth that was asked for", LINE_INFO());
			Assert::IsTrue(shallow.nScore < ChessSearch::MATE_BOUND, L"The checkmate is out of sight of 1 ply");

			Assert::IsTrue(deep.bFound);
			Assert::AreEqual(3, deep.nDepth, L"The depth that was asked for", LINE_INFO());
			Assert::AreEqual(ChessSearch::MATE_SCORE - 3, deep.nScore, L"The checkmate is in sight of 3 plies", LINE_INFO());
			Assert::IsTrue(deep.nNodes > shallow.nNodes, L"A deeper search looks at more positions");
		}

		TEST_METHOD(TwoBishopsGiveCheckmateAgainstABareKing)
		{
			// The checkmate is too far for the search to see it from the start. A position that comes again in a line is a
			// draw for the search: without it the bishops go back and forth for ever because no move looks better than another
			SearchPosition position = ParseSearchPosition(L"8/8/8/8/B7/8/K7/4B2k", true);

			int plies = 0;
			while (!position.LegalMoves().empty() && (plies < 60))
			{
				const bool white = position.IsWhiteToMove();
				const SearchResult result = white ? Search(position, 6, 100000) : Search(position, 1);

				Assert::IsTrue(result.bFound, L"There is a move to play");
				position = After(position, result.move);
				++plies;
			}

			const std::wstring message = L"No checkmate in " + std::to_wstring(plies) + L" plies";
			Assert::IsTrue(IsCheckmate(position), message.c_str(), LINE_INFO());
		}
	};

	TEST_CLASS(ChessSearchLimitsTests)
	{
	public:
		TEST_METHOD(TheSearchStopsAfterTheNodesItIsGiven)
		{
			const SearchPosition position = ParseSearchPosition(MIDDLE_GAME, true);
			const uint64_t limit = 20000;

			const SearchResult result = Search(position, 20, limit);

			Assert::IsTrue(result.bFound);
			Assert::IsFalse(result.bStopped);
			Assert::IsTrue(result.nNodes >= limit, L"The search used the nodes it was given");
			Assert::IsTrue(result.nNodes < limit + 1024, L"The limit is checked every 1024 nodes");

			const std::vector<SearchMove> legal = position.LegalMoves();
			Assert::IsTrue(std::find(legal.begin(), legal.end(), result.move) != legal.end(), L"The move is legal");
		}

		TEST_METHOD(AVerySmallLimitStillGivesALegalMove)
		{
			const SearchPosition position = ParseSearchPosition(MIDDLE_GAME, true);

			const SearchResult result = Search(position, 20, 1);

			Assert::IsTrue(result.bFound);
			const std::vector<SearchMove> legal = position.LegalMoves();
			Assert::IsTrue(std::find(legal.begin(), legal.end(), result.move) != legal.end(), L"The move is legal");
		}

		TEST_METHOD(TheSameLimitsGiveTheSameResultEveryTime)
		{
			for (const wchar_t* placement : POSITIONS)
			{
				const SearchPosition position = ParseSearchPosition(placement, true);
				const std::wstring message = std::wstring(L"Position ") + placement;

				const SearchResult first = Search(position, 5, 100000);
				const SearchResult second = Search(position, 5, 100000);

				Assert::AreEqual(first.bFound, second.bFound, message.c_str(), LINE_INFO());
				Assert::AreEqual(SearchMoveName(first.move), SearchMoveName(second.move), message.c_str(), LINE_INFO());
				Assert::AreEqual(first.nScore, second.nScore, message.c_str(), LINE_INFO());
				Assert::AreEqual(first.nDepth, second.nDepth, message.c_str(), LINE_INFO());
				Assert::AreEqual(first.nNodes, second.nNodes, message.c_str(), LINE_INFO());
			}
		}

		TEST_METHOD(TheSearchGivesNothingWhenItIsAlreadyStopped)
		{
			volatile bool keepSearching = false;
			ChessSearch::Limits limits;
			limits.nMaxDepth = 4;
			limits.pbContinue = &keepSearching;

			const SearchResult result = ChessSearch::FindBestMove(ParseSearchPosition(MIDDLE_GAME, true), std::vector<SearchMove>(), limits);

			Assert::IsTrue(result.bStopped);
			Assert::IsFalse(result.bFound);
		}

		TEST_METHOD(TheSearchStopsWhenTheFlagIsCleared)
		{
			// The program stops the search of the computer player this way when it closes
			volatile bool keepSearching = true;
			const SearchPosition position = ParseSearchPosition(MIDDLE_GAME, true);
			ChessSearch::Limits limits;
			limits.nMaxDepth = 24;
			limits.nMaxNodes = 100000000; // the search is long over before this many nodes unless it is stopped
			limits.pbContinue = &keepSearching;

			SearchResult result;
			std::thread searching([&]()
			{
				result = ChessSearch::FindBestMove(position, std::vector<SearchMove>(), limits);
			});
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			keepSearching = false;
			searching.join();

			Assert::IsTrue(result.bStopped, L"The search did not stop");
			Assert::IsFalse(result.bFound);
		}
	};
}
