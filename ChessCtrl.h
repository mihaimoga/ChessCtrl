/* Copyright (C) 2024 Stefan-Mihai MOGA
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

#pragma once

#include "ChessBoard.hpp"

// CChessCtrl

class CChessCtrl : public CStatic
{
	DECLARE_DYNAMIC(CChessCtrl)

public:
	CChessCtrl();
	virtual ~CChessCtrl();

	CStatic* m_pColorStatic;
	CProgressCtrl* m_ctrlProgress;
	ChessBoard m_pChessBoard;
protected:
	CFont m_pTextFont;
	CFont m_pDrawFont;
	COLORREF m_rgbText;
	COLORREF m_rgbBackground;
	CBrush* m_pCtrlBrush;
	CPen* m_pRedPen;
	CPen* m_pGreenPen;
	CPen* m_pBluePen;
	int m_nSquareLength;
	int m_nAdjustmentX;
	int m_nAdjustmentY;
	bool m_bChessPiece;
	CPoint m_nPreviousPoint;
	CPoint m_nCurrentSquare;
	wstring m_strMoveFrom, m_strMoveTo;
	bool m_bComputerPlayer;

public:
	void SetUI()
	{
		m_pChessBoard.m_pParentWnd = this;
		m_pChessBoard.m_pColorStatic = m_pColorStatic;
		m_pChessBoard.m_ctrlProgress = m_ctrlProgress;
		m_pChessBoard.resetBoard();
	};
	void SetComputerPlayer(bool bOnOff)
	{
		m_bComputerPlayer = bOnOff;
	};
protected:
	virtual void PreSubclassWindow();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	bool IsValidMove();

	DECLARE_MESSAGE_MAP()
};
