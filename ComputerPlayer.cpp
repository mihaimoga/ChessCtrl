/* Copyright (C) 2024-2025 Stefan-Mihai MOGA
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
#include <iostream>
#include <list>

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
			if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
				return true;
		}
	}
	return false;
}

bool GetListOfValidPieces(list<wstring>& pList, const bool isWhitePlayer, Board* pBoard)
{
	pList.clear();
	for (TCHAR i = ChessInfo::MAX_RANK; i >= ChessInfo::MIN_RANK; i--)
	{
		for (TCHAR j = ChessInfo::MIN_FILE; j <= ChessInfo::MAX_FILE; j++)
		{
			try
			{
				wstring strMoveFrom({ j, i });
				Piece* piece = pBoard->at(strMoveFrom);
				if (piece != nullptr)
				{
					if (piece->isWhitePlayer() == isWhitePlayer)
					{
						pList.push_back(strMoveFrom);
					}
				}
			}
			catch (const std::out_of_range& err)
			{
				UNREFERENCED_PARAMETER(err);
			}
		}
	}
	return (pList.size() > 0);
}

bool GetListOfValidMoves(list<wstring>& pList, const wstring& strMoveFrom, Board* pBoard)
{
	pList.clear();
	for (TCHAR i = ChessInfo::MAX_RANK; i >= ChessInfo::MIN_RANK; i--)
	{
		for (TCHAR j = ChessInfo::MIN_FILE; j <= ChessInfo::MAX_FILE; j++)
		{
			try
			{
				wstring strMoveTo({ j, i });
				if (strMoveFrom.compare(strMoveTo) != 0)
				{
					Piece* piece = pBoard->at(strMoveFrom);
					if (piece != nullptr)
					{
						if (ChessErrHandler::CHESS_NO_ERROR == piece->isValidMove(strMoveFrom, strMoveTo, pBoard))
						{
							pList.push_back(strMoveTo);
						}
					}
				}
			}
			catch (const std::out_of_range& err)
			{
				UNREFERENCED_PARAMETER(err);
			}
		}
	}
	return (pList.size() > 0);
}

bool IsValidPiece(const wstring& strFileMark, const bool isWhitePlayer, Board* pBoard, int &nScore)
{
	nScore = 0;
	try
	{
		Piece* piece = pBoard->at(strFileMark);
		if (piece != nullptr)
		{
			nScore = piece->Score();
			return (piece->isWhitePlayer() == isWhitePlayer);
		}
	}
	catch (const std::out_of_range& err)
	{
		UNREFERENCED_PARAMETER(err);
	}
	return false;
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

Piece* TryMoveAndReturnCaptured(wstring sourceFileRank, wstring destFileRank, Board* board)
{
	Piece* sourcePiece = board->at(sourceFileRank);
	Piece* destPiece;

	try
	{
		destPiece = board->at(destFileRank);
		board->at(destFileRank) = sourcePiece;
	}
	catch (const std::out_of_range& err)
	{
		UNREFERENCED_PARAMETER(err);
		destPiece = NULL;
		board->insert({ destFileRank, sourcePiece });
	}
	board->erase(sourceFileRank);
	return destPiece;
}

bool g_bThreadRunning = true;
bool BlackPlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard);

bool WhitePlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard)
{
	strMoveFrom.clear();
	strMoveTo.clear();
	list<wstring> whitePieces;
	if (GetListOfValidPieces(whitePieces, true, pBoard))
	{
		for (std::list<wstring>::iterator pieceMoveFrom = whitePieces.begin(); g_bThreadRunning && (pieceMoveFrom != whitePieces.end()); ++pieceMoveFrom)
		{
			list<wstring> whiteMoves;
			if (GetListOfValidMoves(whiteMoves, *pieceMoveFrom, pBoard))
			{
				int nBestBlackScore = 0, nRecursiveScore = 0;
				for (std::list<wstring>::iterator pieceMoveTo = whiteMoves.begin(); g_bThreadRunning && (pieceMoveTo != whiteMoves.end()); ++pieceMoveTo)
				{
					int nPieceScore = 0;
					int nWhiteScore = 0;
					Board* pClone = CloneBoard(pBoard);
					Piece* capturedPiece = TryMoveAndReturnCaptured(*pieceMoveFrom, *pieceMoveTo, pClone);
					wstring strTempFrom, strTempTo;
					if (BlackPlayer_BacktrackingAlgorithm(nLevel - 1, nWhiteScore, strTempFrom, strTempTo, pClone))
					{
					}
					if (capturedPiece != nullptr)
					{
						nPieceScore = -capturedPiece->Score();
						delete capturedPiece;
						capturedPiece = nullptr;
					}
					CleanBoard(pClone);
					if ((nPieceScore > nBestBlackScore) || (strMoveFrom.empty() || strMoveTo.empty()))
					{
						nBestBlackScore = nPieceScore;
						nRecursiveScore = nWhiteScore;
						strMoveFrom = *pieceMoveFrom;
						strMoveTo = *pieceMoveTo;
					}
				}
				nScore = nBestBlackScore + nRecursiveScore;
			}
			whiteMoves.clear();
		}
		whitePieces.clear();
	}
	return (!strMoveFrom.empty() && !strMoveTo.empty());
}

bool BlackPlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard)
{
	strMoveFrom.clear();
	strMoveTo.clear();
	list<wstring> blackPieces;
	if (GetListOfValidPieces(blackPieces, false, pBoard))
	{
		for (std::list<wstring>::iterator pieceMoveFrom = blackPieces.begin(); g_bThreadRunning && (pieceMoveFrom != blackPieces.end()); ++pieceMoveFrom)
		{
			list<wstring> blackMoves;
			if (GetListOfValidMoves(blackMoves, *pieceMoveFrom, pBoard))
			{
				int nBestWhiteScore = 0, nRecursiveScore = 0;
				for (std::list<wstring>::iterator pieceMoveTo = blackMoves.begin(); g_bThreadRunning && (pieceMoveTo != blackMoves.end()); ++pieceMoveTo)
				{
					int nPieceScore = 0;
					int nBlackScore = 0;
					if ((nLevel - 1) > 0)
					{
						Board* pClone = CloneBoard(pBoard);
						Piece* capturedPiece = TryMoveAndReturnCaptured(*pieceMoveFrom, *pieceMoveTo, pClone);
						wstring strTempFrom, strTempTo;
						if (WhitePlayer_BacktrackingAlgorithm(nLevel, nBlackScore, strTempFrom, strTempTo, pClone))
						{
						}
						if (capturedPiece != nullptr)
						{
							nPieceScore = capturedPiece->Score();
							delete capturedPiece;
							capturedPiece = nullptr;
						}
						CleanBoard(pClone);
					}
					else
					{
						if (IsValidPiece(*pieceMoveTo, true, pBoard, nPieceScore))
						{
							TRACE(_T("piece captured!"));
						}
					}
					if ((nPieceScore > nBestWhiteScore) || (strMoveFrom.empty() || strMoveTo.empty()))
					{
						nBestWhiteScore = nPieceScore;
						nRecursiveScore = nBlackScore;
						strMoveFrom = *pieceMoveFrom;
						strMoveTo = *pieceMoveTo;
					}
				}
				nScore = nBestWhiteScore + nRecursiveScore;
			}
			blackMoves.clear();
		}
		blackPieces.clear();
	}
	return (!strMoveFrom.empty() && !strMoveTo.empty());
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
