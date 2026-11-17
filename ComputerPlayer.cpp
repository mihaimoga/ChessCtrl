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
#include "ChessBoard.hpp"
#include "ChessCtrl.h"
#include "ChessSearch.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

bool WaitWithMessageLoop(HANDLE hEvent, DWORD dwTimeout)
{
	DWORD dwRet;
	MSG msg;
	hEvent = hEvent ? hEvent : CreateEvent(NULL, FALSE, FALSE, NULL);

	while (true)
	{
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwTimeout, QS_ALLINPUT);
		if (dwRet == WAIT_OBJECT_0)
			return TRUE;
		if (dwRet != WAIT_OBJECT_0 + 1)
			break;
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (hEvent && (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0))
				return true;
		}
	}
	return false;
}

// A board has 8 files x 8 ranks, its squares are numbered in the order they are visited: rank 8 down to rank 1, file A to H
static constexpr int FILE_COUNT = ChessInfo::MAX_FILE - ChessInfo::MIN_FILE + 1;
static constexpr int RANK_COUNT = ChessInfo::MAX_RANK - ChessInfo::MIN_RANK + 1;
static constexpr int SQUARE_COUNT = FILE_COUNT * RANK_COUNT;

// File & rank representation of every square, built once instead of once per square visited
static const std::array<wstring, SQUARE_COUNT>& GetSquareNames()
{
	static const std::array<wstring, SQUARE_COUNT> squareNames = []()
	{
		std::array<wstring, SQUARE_COUNT> names;
		int nSquare = 0;
		for (TCHAR i = ChessInfo::MAX_RANK; i >= ChessInfo::MIN_RANK; i--)
		{
			for (TCHAR j = ChessInfo::MIN_FILE; j <= ChessInfo::MAX_FILE; j++)
			{
				names[nSquare++] = wstring({ j, i });
			}
		}
		return names;
	}();
	return squareNames;
}

// Square number of a file & rank representation, -1 if it is not a square of the board
static int GetSquareNumber(const wstring& strFileRank)
{
	if (strFileRank.length() != ChessInfo::FILERANK_LENGTH)
	{
		return -1;
	}
	const int nFile = strFileRank[ChessInfo::FILE_INDEX] - ChessInfo::MIN_FILE;
	const int nRank = ChessInfo::MAX_RANK - strFileRank[ChessInfo::RANK_INDEX];
	if ((nFile < 0) || (nFile >= FILE_COUNT) || (nRank < 0) || (nRank >= RANK_COUNT))
	{
		return -1;
	}
	return nRank * FILE_COUNT + nFile;
}

Board* CloneBoard(Board* board)
{
	Board* clone = new Board;
	for (Board::iterator it = board->begin(); it != board->end(); it++)
	{
		clone->insert({ it->first, it->second->clone() });
	}
	return clone;
}

void CleanBoard(Board* board)
{
	for (Board::iterator it = board->begin(); it != board->end(); it++)
	{
		delete it->second;
	}
	delete board;
}

Piece* TryMoveAndReturnCaptured(const wstring& sourceFileRank, const wstring& destFileRank, Board* board)
{
	Piece* sourcePiece = board->at(sourceFileRank);
	Piece* destPiece = nullptr;

	Board::iterator itDest = board->find(destFileRank);
	if (itDest != board->end())
	{
		destPiece = itDest->second;
		itDest->second = sourcePiece;
	}
	else
	{
		board->insert({ destFileRank, sourcePiece });
	}
	board->erase(sourceFileRank);
	return destPiece;
}

/* True when none of the pieces of the opponent can capture the king of the player, which is how ChessBoard decides that a
   move leaves the king in check (a player without a king is never in check)
*/
static bool IsKingSafe(const bool bWhitePlayer, Board* pBoard)
{
	const wstring* pKingFileRank = nullptr;
	for (Board::iterator it = pBoard->begin(); (pKingFileRank == nullptr) && (it != pBoard->end()); it++)
	{
		if ((it->second->isWhitePlayer() == bWhitePlayer) && it->second->isKing())
		{
			pKingFileRank = &it->first;
		}
	}
	if (pKingFileRank == nullptr)
	{
		return true;
	}

	for (Board::iterator it = pBoard->begin(); it != pBoard->end(); it++)
	{
		if ((it->second->isWhitePlayer() != bWhitePlayer) &&
			(ChessErrHandler::CHESS_NO_ERROR == it->second->isValidMove(it->first, *pKingFileRank, pBoard)))
		{
			return false;
		}
	}
	return true;
}

// A valid move of a piece is legal when it keeps the king of the player safe, it is tried on a copy of the board
static bool IsLegalMove(const bool bWhitePlayer, const wstring& strMoveFrom, const wstring& strMoveTo, Board* pBoard)
{
	Board* pClone = CloneBoard(pBoard);
	Piece* capturedPiece = TryMoveAndReturnCaptured(strMoveFrom, strMoveTo, pClone);
	const bool bLegal = IsKingSafe(bWhitePlayer, pClone);
	delete capturedPiece;
	CleanBoard(pClone);
	return bLegal;
}

bool g_bThreadRunning = true;

/* How hard the computer player thinks: the depth of its search in plies, and the number of positions after which every
   search stops, whatever its depth is. The search counts positions and not time, so a position always gives the same
   move, on any computer and in any build.
*/
static constexpr int COMPUTER_LEVEL = 10;
static constexpr uint64_t SEARCH_MAX_NODES = 2000000;

/* Copies a board in the representation of the search (see ChessSearch.hpp), which never touches the board of the game:
   it is shown on the screen while the computer player thinks. False if the board holds something the search does not
   know, a square that is not on the board or a piece that is not a chess piece.
*/
bool BoardToPosition(Board* pBoard, const bool bWhiteToMove, ChessSearch::Position& position)
{
	position.Clear();
	position.SetWhiteToMove(bWhiteToMove);
	for (Board::iterator it = pBoard->begin(); it != pBoard->end(); it++)
	{
		Piece* pPiece = it->second;
		int nKind = ChessSearch::NO_PIECE;
		if (dynamic_cast<Pawn*>(pPiece) != nullptr)
		{
			nKind = ChessSearch::PAWN;
		}
		else if (dynamic_cast<Knight*>(pPiece) != nullptr)
		{
			nKind = ChessSearch::KNIGHT;
		}
		else if (dynamic_cast<Bishop*>(pPiece) != nullptr)
		{
			nKind = ChessSearch::BISHOP;
		}
		else if (dynamic_cast<Rook*>(pPiece) != nullptr)
		{
			nKind = ChessSearch::ROOK;
		}
		else if (dynamic_cast<Queen*>(pPiece) != nullptr)
		{
			nKind = ChessSearch::QUEEN;
		}
		else if (dynamic_cast<King*>(pPiece) != nullptr)
		{
			nKind = ChessSearch::KING;
		}

		const int nSquare = GetSquareNumber(it->first);
		if ((nKind == ChessSearch::NO_PIECE) || (nSquare < 0))
		{
			return false;
		}
		position.Put(nSquare, nKind, pPiece->isWhitePlayer());
	}
	return true;
}

/* Search shared by WhitePlayer_BacktrackingAlgorithm() and BlackPlayer_BacktrackingAlgorithm(), which only differ in
   the player: the best move of the player, searched nLevel plies deep (ChessSearch::FindBestMove() deepens the search
   one ply at a time, so the best move of the shallower searches is kept if the search is stopped). nScore is the value
   of the move for the player, in centipawns, and it is left alone when there is no move.

   ChessBoard::submitMove() has to accept the move, otherwise the player would never move: the search only chooses
   among the moves that keep the king of the player safe, and among them the ones that the engine accepts as well, so
   it gives no move only when the player has none (checkmate or stalemate), when the game is stopped or when the board
   cannot be searched.
*/
static bool SearchMove(const bool bWhitePlayer, const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard)
{
	strMoveFrom.clear();
	strMoveTo.clear();
	if (!g_bThreadRunning)
	{
		return false;
	}

	ChessSearch::Position position;
	if (!BoardToPosition(pBoard, bWhitePlayer, position))
	{
		return false;
	}

	const std::array<wstring, SQUARE_COUNT>& squareNames = GetSquareNames();
	std::vector<ChessSearch::Move> moves;
	for (const ChessSearch::Move& move : position.LegalMoves())
	{
		const wstring& strSquareFrom = squareNames[move.nFrom];
		const wstring& strSquareTo = squareNames[move.nTo];
		Board::iterator it = pBoard->find(strSquareFrom);
		if ((it != pBoard->end()) && (it->second->isWhitePlayer() == bWhitePlayer) &&
			(ChessErrHandler::CHESS_NO_ERROR == it->second->isValidMove(strSquareFrom, strSquareTo, pBoard)) &&
			IsLegalMove(bWhitePlayer, strSquareFrom, strSquareTo, pBoard))
		{
			moves.push_back(move);
		}
	}
	if (moves.empty())
	{
		return false;
	}

	ChessSearch::Limits limits;
	limits.nMaxDepth = nLevel;
	limits.nMaxNodes = SEARCH_MAX_NODES;
	limits.pbContinue = &g_bThreadRunning;
	const ChessSearch::Result result = ChessSearch::FindBestMove(position, moves, limits);
	if (!result.bFound)
	{
		return false;
	}
	strMoveFrom = squareNames[result.move.nFrom];
	strMoveTo = squareNames[result.move.nTo];
	nScore = result.nScore;
	return true;
}

bool WhitePlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard)
{
	return SearchMove(true, nLevel, nScore, strMoveFrom, strMoveTo, pBoard);
}

bool BlackPlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard)
{
	return SearchMove(false, nLevel, nScore, strMoveFrom, strMoveTo, pBoard);
}

DWORD WINAPI ComputerThreadProc(LPVOID lpParam)
{
	g_bThreadRunning = true;
	ChessBoard* pChessBoard = reinterpret_cast<ChessBoard*>(lpParam);
	// ::MessageBox(nullptr, _T("ComputerPlayer() is not yet implemented!"), _T("ChessCtrl"), MB_OK);
	if (pChessBoard != nullptr)
	{
		CChessCtrl* pParentWnd = pChessBoard->m_pParentWnd;
		CProgressCtrl* ctrlProgress = pChessBoard->m_ctrlProgress;
		CStatic* pColorStatic = pChessBoard->m_pColorStatic;

		if (ctrlProgress != nullptr)
		{
			ctrlProgress->SetMarquee(TRUE, 30);
		}

		int nScore = 0;
		wstring strBestMoveFrom, strBestMoveTo;
		if (BlackPlayer_BacktrackingAlgorithm(COMPUTER_LEVEL, nScore, strBestMoveFrom, strBestMoveTo, pChessBoard->_board))
		{
			try
			{ 
				pChessBoard->submitMove(strBestMoveFrom.c_str(), strBestMoveTo.c_str());
			}
			catch (const std::out_of_range& err)
			{
				UNREFERENCED_PARAMETER(err);
			}
		}

		if (ctrlProgress != nullptr)
		{
			ctrlProgress->SetMarquee(FALSE, 30);
		}

		if (pParentWnd != nullptr)
		{
			pParentWnd->RedrawWindow();
			pParentWnd->UpdateWindow();
		}
		if (pColorStatic != nullptr)
		{
			pColorStatic->RedrawWindow();
			pColorStatic->UpdateWindow();
		}
		if (ctrlProgress != nullptr)
		{
			ctrlProgress->RedrawWindow();
			ctrlProgress->UpdateWindow();
		}
	}
	g_bThreadRunning = false;
	return 0;
}

void ChessBoard::ComputerPlayer()
{
	m_hComputerThread = ::CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ComputerThreadProc, this, 0, &m_nComputerThreadID);

	// VERIFY(WaitWithMessageLoop(m_hComputerThread, INFINITE));
}
