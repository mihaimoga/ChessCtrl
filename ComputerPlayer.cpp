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

#include <algorithm>
#include <array>
#include <iostream>

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

/* Search shared by WhitePlayer_BacktrackingAlgorithm() and BlackPlayer_BacktrackingAlgorithm(), which only differ in
   the player and in the level handling: White always recurses with the next level (nLevel - 1), while Black keeps
   nLevel and only recurses as long as (nLevel - 1) > 0, otherwise it just scores the piece captured by the move.

   A move replaces the best one when it scores higher than the best move of the same piece or when no move has been
   selected yet, and nScore is set once per piece that can move. The score of the recursive search is only used by a
   move that becomes the best one, so the (expensive) recursion is skipped for every other move: revisit this if the
   use of that score ever changes.

   With bLegalOnly only the moves that keep the king of the player safe can be selected (the search of the next
   moves never needs it: it does not choose the move that is played).
*/
static bool SearchBestMove(const bool bWhitePlayer, const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard, const bool bLegalOnly)
{
	strMoveFrom.clear();
	strMoveTo.clear();

	const std::array<wstring, SQUARE_COUNT>& squareNames = GetSquareNames();

	// One pass over the board instead of looking up every square (an empty square is a null pointer)
	Piece* squares[SQUARE_COUNT] = {};
	for (Board::iterator it = pBoard->begin(); it != pBoard->end(); it++)
	{
		const int nSquare = GetSquareNumber(it->first);
		if (nSquare >= 0)
		{
			squares[nSquare] = it->second;
		}
	}

	int pieceSquares[SQUARE_COUNT];
	int nPieces = 0;
	for (int nSquare = 0; nSquare < SQUARE_COUNT; nSquare++)
	{
		if ((squares[nSquare] != nullptr) && (squares[nSquare]->isWhitePlayer() == bWhitePlayer))
		{
			pieceSquares[nPieces++] = nSquare;
		}
	}

	const bool bRecursive = bWhitePlayer || ((nLevel - 1) > 0);
	const int nNextLevel = bWhitePlayer ? (nLevel - 1) : nLevel;

	for (int nPiece = 0; g_bThreadRunning && (nPiece < nPieces); nPiece++)
	{
		const int nSquareFrom = pieceSquares[nPiece];
		const wstring& strSquareFrom = squareNames[nSquareFrom];
		Piece* pPiece = squares[nSquareFrom];

		int moveSquares[SQUARE_COUNT];
		int nMoves = 0;
		for (int nSquare = 0; nSquare < SQUARE_COUNT; nSquare++)
		{
			if ((nSquare != nSquareFrom) &&
				(ChessErrHandler::CHESS_NO_ERROR == pPiece->isValidMove(strSquareFrom, squareNames[nSquare], pBoard)))
			{
				moveSquares[nMoves++] = nSquare;
			}
		}

		if (nMoves > 0)
		{
			int nBestScore = 0, nRecursiveScore = 0;
			for (int nMove = 0; g_bThreadRunning && (nMove < nMoves); nMove++)
			{
				const wstring& strSquareTo = squareNames[moveSquares[nMove]];
				Piece* pDestPiece = squares[moveSquares[nMove]];
				int nPieceScore = 0;
				if (pDestPiece != nullptr)
				{
					nPieceScore = bWhitePlayer ? -pDestPiece->Score() : pDestPiece->Score();
					if (!bRecursive && pDestPiece->isWhitePlayer())
					{
						TRACE(_T("piece captured!"));
					}
				}
				if ((nPieceScore > nBestScore) || (strMoveFrom.empty() || strMoveTo.empty()))
				{
					if (bLegalOnly && !IsLegalMove(bWhitePlayer, strSquareFrom, strSquareTo, pBoard))
					{
						continue;
					}
					int nChildScore = 0;
					if (bRecursive)
					{
						Board* pClone = CloneBoard(pBoard);
						Piece* capturedPiece = TryMoveAndReturnCaptured(strSquareFrom, strSquareTo, pClone);
						wstring strTempFrom, strTempTo;
						SearchBestMove(!bWhitePlayer, nNextLevel, nChildScore, strTempFrom, strTempTo, pClone, false);
						delete capturedPiece;
						CleanBoard(pClone);
					}
					nBestScore = nPieceScore;
					nRecursiveScore = nChildScore;
					strMoveFrom = strSquareFrom;
					strMoveTo = strSquareTo;
				}
			}
			nScore = nBestScore + nRecursiveScore;
		}
	}
	return (!strMoveFrom.empty() && !strMoveTo.empty());
}

/* The move a player plays, which ChessBoard::submitMove() has to accept. The search does not look at the king, so its best
   move can leave the king of the player in check (for example the capture of a pinned piece): such a move is refused and
   the player would never move, then the best of the moves that keep the king safe is searched.
*/
static bool SearchLegalMove(const bool bWhitePlayer, const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard)
{
	if (!SearchBestMove(bWhitePlayer, nLevel, nScore, strMoveFrom, strMoveTo, pBoard, false))
	{
		return false;
	}
	if (IsLegalMove(bWhitePlayer, strMoveFrom, strMoveTo, pBoard))
	{
		return true;
	}
	return SearchBestMove(bWhitePlayer, nLevel, nScore, strMoveFrom, strMoveTo, pBoard, true);
}

bool WhitePlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard)
{
	return SearchLegalMove(true, nLevel, nScore, strMoveFrom, strMoveTo, pBoard);
}

bool BlackPlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard)
{
	return SearchLegalMove(false, nLevel, nScore, strMoveFrom, strMoveTo, pBoard);
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
		if (BlackPlayer_BacktrackingAlgorithm(2, nScore, strBestMoveFrom, strBestMoveTo, pChessBoard->_board))
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
