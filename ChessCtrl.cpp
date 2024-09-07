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

// ChessCtrl.cpp : implementation file
//

#include "pch.h"
#include "ChessDemo.h"
#include "ChessCtrl.h"
#include "memdc.h"

// CChessCtrl

IMPLEMENT_DYNAMIC(CChessCtrl, CStatic)

CChessCtrl::CChessCtrl()
{
	m_pColorStatic = nullptr;
	m_ctrlProgress = nullptr;
	m_rgbText = GetSysColor(COLOR_BTNTEXT);
	m_rgbBackground = GetSysColor(COLOR_BTNFACE);
	m_pCtrlBrush = new CBrush(m_rgbBackground);
	m_pRedPen = new CPen(PS_SOLID, 2, RGB(0xA0, 0x00, 0x00));
	m_pGreenPen = new CPen(PS_SOLID, 2, RGB(0x00, 0xA0, 0x00));
	m_pBluePen = new CPen(PS_SOLID, 2, RGB(0x00, 0x00, 0xA0));
	m_nSquareLength = 0;
	m_nAdjustmentX = 0;
	m_nAdjustmentY = 0;
	m_bChessPiece = false;
	m_nPreviousPoint = CPoint(-1, -1);
	m_nCurrentSquare = CPoint(-1, -1);
	m_bComputerPlayer = false;
}

CChessCtrl::~CChessCtrl()
{
	if (m_pTextFont.GetSafeHandle())
		VERIFY(m_pTextFont.DeleteObject());

	if (m_pDrawFont.GetSafeHandle())
		VERIFY(m_pDrawFont.DeleteObject());

	if (m_pCtrlBrush != nullptr)
	{
		VERIFY(m_pCtrlBrush->DeleteObject());
		delete m_pCtrlBrush;
		m_pCtrlBrush = nullptr;
	}

	if (m_pRedPen != nullptr)
	{
		VERIFY(m_pRedPen->DeleteObject());
		delete m_pRedPen;
		m_pRedPen = nullptr;
	}

	if (m_pGreenPen != nullptr)
	{
		VERIFY(m_pGreenPen->DeleteObject());
		delete m_pGreenPen;
		m_pGreenPen = nullptr;
	}

	if (m_pBluePen != nullptr)
	{
		VERIFY(m_pBluePen->DeleteObject());
		delete m_pBluePen;
		m_pBluePen = nullptr;
	}
}

BEGIN_MESSAGE_MAP(CChessCtrl, CStatic)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

// CChessCtrl message handlers
void CChessCtrl::PreSubclassWindow()
{
	// get current font
	CFont* pFont = GetFont();
	if (!pFont)
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (hFont == NULL)
			hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
		if (hFont)
			pFont = CFont::FromHandle(hFont);
	}
	ASSERT(pFont);
	ASSERT(pFont->GetSafeHandle());

	// create the font for this control
	LOGFONT lf;
	pFont->GetLogFont(&lf);
	VERIFY(m_pTextFont.CreateFontIndirect(&lf));

	lf.lfHeight = -MulDiv(35, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72);
	VERIFY(m_pDrawFont.CreateFontIndirect(&lf));
}

BOOL CChessCtrl::OnEraseBkgnd(CDC* pDC)
{
	UNREFERENCED_PARAMETER(pDC);
	/*CRect cr;
	GetClientRect(cr);
	pDC->FillRect(&cr, m_pCtrlBrush);
	// return CStatic::OnEraseBkgnd(pDC);*/
	return TRUE;
}

void CChessCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// Do not call CStatic::OnPaint() for painting messages
	CodeProject::CMemDC pDC(&dc);

	pDC.SaveDC();

	CRect pRect;
	GetClientRect(pRect);
	m_nSquareLength = min(pRect.Width() / 10, pRect.Height() / 10);
	m_nAdjustmentX = (pRect.Width() - 10 * m_nSquareLength) / 2;
	m_nAdjustmentY = (pRect.Height() - 10 * m_nSquareLength) / 2;
	for (int nLineIndex = 0; nLineIndex < 8; nLineIndex++)
	{
		const int nOffsetX = pRect.Width() - m_nAdjustmentX - (9 - nLineIndex) * m_nSquareLength;
		for (int nColumnIndex = 0; nColumnIndex < 8; nColumnIndex++)
		{
			const int nOffsetY = pRect.Height() - m_nAdjustmentY - (9 - nColumnIndex) * m_nSquareLength;
			CRect pFillRect(CPoint(nOffsetX, nOffsetY), CSize(m_nSquareLength, m_nSquareLength));
			// pDC.FillSolidRect(pFillRect, (((nLineIndex + nColumnIndex) % 2) == 0) ? RGB(0xB8, 0x86, 0x0B) : RGB(0xEE, 0xE8, 0xAA));
			pDC.FillSolidRect(pFillRect, (((nLineIndex + nColumnIndex) % 2) != 0) ? RGB(0xB0, 0xB0, 0xB0) : RGB(0xFF, 0xFF, 0xFF));
		}
	}

	pDC.SetTextColor(m_rgbText);
	pDC.SetBkColor(m_rgbBackground);
	pDC.SetBkMode(TRANSPARENT);
	pDC.SelectObject(m_pCtrlBrush);
	pDC.SelectObject(&m_pTextFont);

	for (int nLineIndex = 0; nLineIndex <= 8; nLineIndex++)
	{
		const int nOffsetX = pRect.Width() - m_nAdjustmentX - (9 - nLineIndex) * m_nSquareLength;
		const int nOffsetY = pRect.Height() - m_nAdjustmentY - 9 * m_nSquareLength;

		CPoint pStartPoint(nOffsetX, nOffsetY);
		pDC.MoveTo(pStartPoint);
		CPoint pEndPoint(nOffsetX, nOffsetY + 8 * m_nSquareLength);
		pDC.LineTo(pEndPoint);

		if (nLineIndex < 8)
		{
			CString strTextLine;
			strTextLine.Format(_T("%c"), _T('A') + nLineIndex);
			CRect pTextRect(CPoint(nOffsetX, nOffsetY - m_nSquareLength), CSize(m_nSquareLength, m_nSquareLength));
			pDC.DrawText(strTextLine, pTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			pTextRect = CRect(CPoint(nOffsetX, nOffsetY + 8 * m_nSquareLength), CSize(m_nSquareLength, m_nSquareLength));
			pDC.DrawText(strTextLine, pTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
	}

	for (int nColumnIndex = 0; nColumnIndex <= 8; nColumnIndex++)
	{
		const int nOffsetX = pRect.Width() - m_nAdjustmentX - 9 * m_nSquareLength;
		const int nOffsetY = pRect.Height() - m_nAdjustmentY - (9 - nColumnIndex) * m_nSquareLength;

		CPoint pStartPoint(nOffsetX, nOffsetY);
		pDC.MoveTo(pStartPoint);
		CPoint pEndPoint(nOffsetX + 8 * m_nSquareLength, nOffsetY);
		pDC.LineTo(pEndPoint);

		if (nColumnIndex < 8)
		{
			CString strTextColumn;
			strTextColumn.Format(_T("%d"), 8 - nColumnIndex);
			CRect pTextRect(CPoint(nOffsetX - m_nSquareLength, nOffsetY), CSize(m_nSquareLength, m_nSquareLength));
			pDC.DrawText(strTextColumn, pTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			pTextRect = CRect(CPoint(nOffsetX + 8 * m_nSquareLength, nOffsetY), CSize(m_nSquareLength, m_nSquareLength));
			pDC.DrawText(strTextColumn, pTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
	}

	if ((m_pChessBoard.IsWhiteTurn() || !m_bComputerPlayer) && !m_pChessBoard.HasGameEnded())
	{
		if ((m_nCurrentSquare.x >= 0) && (m_nCurrentSquare.x <= 7) &&
			(m_nCurrentSquare.y >= 0) && (m_nCurrentSquare.y <= 7))
		{
			if (m_bChessPiece)
			{
				CPen* pOldPen = pDC.SelectObject(m_pBluePen);
				if ((m_nPreviousPoint.x >= 0) && (m_nPreviousPoint.x <= 7) &&
					(m_nPreviousPoint.y >= 0) && (m_nPreviousPoint.y <= 7))
				{
					const int nMinX = pRect.Width() - m_nAdjustmentX - 10 * m_nSquareLength + (1 + m_nPreviousPoint.x) * m_nSquareLength + 1;
					const int nMinY = pRect.Height() - m_nAdjustmentY - 10 * m_nSquareLength + (1 + m_nPreviousPoint.y) * m_nSquareLength + 1;
					pDC.MoveTo(CPoint(nMinX + 1, nMinY + 1));
					pDC.LineTo(CPoint(nMinX + m_nSquareLength - 2, nMinY + 1));
					pDC.LineTo(CPoint(nMinX + m_nSquareLength - 2, nMinY + m_nSquareLength - 2));
					pDC.LineTo(CPoint(nMinX + 1, nMinY + m_nSquareLength - 2));
					pDC.LineTo(CPoint(nMinX + 1, nMinY + 1));
				}

				if ((m_nCurrentSquare.x != m_nPreviousPoint.x) ||
					(m_nCurrentSquare.y != m_nPreviousPoint.y))
				{
					pDC.SelectObject(IsValidMove() ? m_pGreenPen : m_pRedPen);
					const int nMinX = pRect.Width() - m_nAdjustmentX - 10 * m_nSquareLength + (1 + m_nCurrentSquare.x) * m_nSquareLength + 1;
					const int nMinY = pRect.Height() - m_nAdjustmentY - 10 * m_nSquareLength + (1 + m_nCurrentSquare.y) * m_nSquareLength + 1;
					pDC.MoveTo(CPoint(nMinX + 1, nMinY + 1));
					pDC.LineTo(CPoint(nMinX + m_nSquareLength - 2, nMinY + 1));
					pDC.LineTo(CPoint(nMinX + m_nSquareLength - 2, nMinY + m_nSquareLength - 2));
					pDC.LineTo(CPoint(nMinX + 1, nMinY + m_nSquareLength - 2));
					pDC.LineTo(CPoint(nMinX + 1, nMinY + 1));
				}

				pDC.SelectObject(pOldPen);
			}
			else
			{
				CPen* pOldPen = pDC.SelectObject(m_pBluePen);
				const int nMinX = pRect.Width() - m_nAdjustmentX - 10 * m_nSquareLength + (1 + m_nCurrentSquare.x) * m_nSquareLength + 1;
				const int nMinY = pRect.Height() - m_nAdjustmentY - 10 * m_nSquareLength + (1 + m_nCurrentSquare.y) * m_nSquareLength + 1;
				pDC.MoveTo(CPoint(nMinX + 1, nMinY + 1));
				pDC.LineTo(CPoint(nMinX + m_nSquareLength - 2, nMinY + 1));
				pDC.LineTo(CPoint(nMinX + m_nSquareLength - 2, nMinY + m_nSquareLength - 2));
				pDC.LineTo(CPoint(nMinX + 1, nMinY + m_nSquareLength - 2));
				pDC.LineTo(CPoint(nMinX + 1, nMinY + 1));
				pDC.SelectObject(pOldPen);
			}
		}
	}

	pDC.SelectObject(&m_pDrawFont);
	for (TCHAR i = ChessInfo::MAX_RANK; i >= ChessInfo::MIN_RANK; i--)
	{
		for (TCHAR j = ChessInfo::MIN_FILE; j <= ChessInfo::MAX_FILE; j++)
		{
			try
			{
				wstring fileRank({ j, i });
				Piece* piece = m_pChessBoard._board->at(fileRank);
				if (piece != nullptr)
				{
					const int nOffsetX = pRect.Width() - m_nAdjustmentX - (2 + (ChessInfo::MAX_FILE - j)) * m_nSquareLength;
					const int nOffsetY = pRect.Height() - m_nAdjustmentY - (2 + (i - ChessInfo::MIN_RANK)) * m_nSquareLength;
					CRect pTextRect(CPoint(nOffsetX, nOffsetY), CSize(m_nSquareLength, m_nSquareLength));
					pDC.DrawText(piece->toGraphics().c_str(), pTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				}
			}
			catch (const std::out_of_range& err)
			{
				UNREFERENCED_PARAMETER(err);
			}
		}
	}

	pDC.RestoreDC(-1);
}

void CChessCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	UNREFERENCED_PARAMETER(nFlags);
	m_nCurrentSquare = CPoint(-1, -1);
	const int nMinX = m_nAdjustmentX + m_nSquareLength;
	const int nMinY = m_nAdjustmentY + m_nSquareLength;
	const int nMaxX = nMinX + 8 * m_nSquareLength;
	const int nMaxY = nMinY + 8 * m_nSquareLength;

	if ((m_pChessBoard.IsWhiteTurn() || !m_bComputerPlayer) && !m_pChessBoard.HasGameEnded())
	{
		if ((nMinX < point.x) && (point.x < nMaxX) &&
			(nMinY < point.y) && (point.y < nMaxY))
		{
			m_nCurrentSquare.x = (point.x - nMinX) / m_nSquareLength;
			m_nCurrentSquare.y = (point.y - nMinY) / m_nSquareLength;

			if (!m_bChessPiece)
			{
				if ((m_nCurrentSquare.x != m_nPreviousPoint.x) ||
					(m_nCurrentSquare.y != m_nPreviousPoint.y))
				{
					m_nPreviousPoint = m_nCurrentSquare;
					RedrawWindow();
					UpdateWindow();
					if (m_pColorStatic != nullptr)
					{
						m_pColorStatic->RedrawWindow();
						m_pColorStatic->UpdateWindow();
					}
					if (m_ctrlProgress != nullptr)
					{
						m_ctrlProgress->RedrawWindow();
						m_ctrlProgress->UpdateWindow();
					}
				}
			}
			else
			{
				if ((m_nCurrentSquare.x != m_nPreviousPoint.x) ||
					(m_nCurrentSquare.y != m_nPreviousPoint.y))
				{
					RedrawWindow();
					UpdateWindow();
					if (m_pColorStatic != nullptr)
					{
						m_pColorStatic->RedrawWindow();
						m_pColorStatic->UpdateWindow();
					}
					if (m_ctrlProgress != nullptr)
					{
						m_ctrlProgress->RedrawWindow();
						m_ctrlProgress->UpdateWindow();
					}
				}
			}
		}
	}

	CStatic::OnMouseMove(nFlags, point);
}

void CChessCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_pChessBoard.IsWhiteTurn() || !m_bComputerPlayer) && !m_pChessBoard.HasGameEnded())
	{
		if ((m_nCurrentSquare.x >= 0) && (m_nCurrentSquare.x <= 7) &&
			(m_nCurrentSquare.y >= 0) && (m_nCurrentSquare.y <= 7))
		{
			if (!m_bChessPiece)
			{
				try
				{
					wstring fileRank({ (TCHAR)(ChessInfo::MIN_FILE + m_nCurrentSquare.x), (TCHAR)(ChessInfo::MAX_RANK - m_nCurrentSquare.y) });
					Piece* piece = m_pChessBoard._board->at(fileRank);
					m_strMoveFrom = fileRank;
					if (piece != nullptr)
					{ 
						m_bChessPiece = true;
						RedrawWindow();
						UpdateWindow();
						if (m_pColorStatic != nullptr)
						{
							m_pColorStatic->RedrawWindow();
							m_pColorStatic->UpdateWindow();
						}
						if (m_ctrlProgress != nullptr)
						{
							m_ctrlProgress->RedrawWindow();
							m_ctrlProgress->UpdateWindow();
						}
					}
				}
				catch (const std::out_of_range& err)
				{
					UNREFERENCED_PARAMETER(err);
				}
			}
			else
			{
				m_bChessPiece = false;
				// Move piece
				try
				{
					wstring fileRank({ (TCHAR)(ChessInfo::MIN_FILE + m_nCurrentSquare.x), (TCHAR)(ChessInfo::MAX_RANK - m_nCurrentSquare.y) });
					m_strMoveTo = fileRank;
					m_pChessBoard.submitMove(m_strMoveFrom.c_str(), m_strMoveTo.c_str());

					RedrawWindow();
					UpdateWindow();
					if (m_pColorStatic != nullptr)
					{
						m_pColorStatic->RedrawWindow();
						m_pColorStatic->UpdateWindow();
					}
					if (m_ctrlProgress != nullptr)
					{
						m_ctrlProgress->RedrawWindow();
						m_ctrlProgress->UpdateWindow();
					}

					if (m_bComputerPlayer && !m_pChessBoard.IsWhiteTurn() && !m_pChessBoard.HasGameEnded())
					{
						m_pChessBoard.ComputerPlayer();

						/* RedrawWindow();
						UpdateWindow();
						if (m_pColorStatic != nullptr)
						{
							m_pColorStatic->RedrawWindow();
							m_pColorStatic->UpdateWindow();
						}
						if (m_ctrlProgress != nullptr)
						{
							m_ctrlProgress->RedrawWindow();
							m_ctrlProgress->UpdateWindow();
						}*/
					}
				}
				catch (const std::out_of_range& err)
				{
					UNREFERENCED_PARAMETER(err);
				}
			}
		}
	}

	CStatic::OnLButtonDown(nFlags, point);
}

bool CChessCtrl::IsValidMove()
{
	try
	{
		wstring fileRank({ (TCHAR)(ChessInfo::MIN_FILE + m_nCurrentSquare.x), (TCHAR)(ChessInfo::MAX_RANK - m_nCurrentSquare.y) });
		Piece* piece = m_pChessBoard._board->at(m_strMoveFrom);
		if (piece != nullptr)
		{
			return (ChessErrHandler::CHESS_NO_ERROR == piece->isValidMove(m_strMoveFrom, fileRank, m_pChessBoard._board));
		}
	}
	catch (const std::out_of_range& err)
	{
		UNREFERENCED_PARAMETER(err);
	}
	return false;
}
