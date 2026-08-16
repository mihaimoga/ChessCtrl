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

/// @file ChessDemoDlg.h
/// @brief Header file for the main application dialog of the ChessDemo application.

#pragma once

#include "ChessCtrl.h"

/**
 * @class CChessDemoDlg
 * @brief Main dialog class for the ChessDemo application.
 *
 * Serves as the primary UI window, hosting the chess control,
 * a color indicator static control, and a progress bar. It also
 * manages the system menu extensions for social media links,
 * GitHub resources, and the in-app user manual.
 *
 * @extends CDialogEx
 */
class CChessDemoDlg : public CDialogEx
{
	// Construction
public:
	/**
	 * @brief Standard constructor.
	 * @param pParent Pointer to the parent window. Defaults to nullptr.
	 */
	CChessDemoDlg(CWnd* pParent = nullptr);

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHESSDEMO_DIALOG };
#endif

protected:
	/**
	 * @brief Exchanges data between dialog controls and member variables.
	 * @param pDX Pointer to the CDataExchange object used for DDX/DDV.
	 */
	virtual void DoDataExchange(CDataExchange* pDX);

	// Implementation
protected:
	/** @brief Handle to the application icon displayed in the title bar. */
	HICON m_hIcon;

	/** @brief Static control used to display the current player's color indicator. */
	CStatic m_pColorStatic;

	/** @brief Progress bar control used to visualize engine thinking progress. */
	CProgressCtrl m_ctrlProgress;

	/** @brief The embedded chess control that renders the board and manages game logic. */
	CChessCtrl m_pChessCtrl;

	// Generated message map functions

	/**
	 * @brief Initializes the dialog: sets up the system menu, icons, and chess control.
	 * @return TRUE to set focus to the first control; FALSE if focus is set manually.
	 */
	virtual BOOL OnInitDialog();

	/**
	 * @brief Handles system menu commands, including About, social media links,
	 *        GitHub resources, and the user manual.
	 * @param nID  The system command identifier.
	 * @param lParam Additional message-specific data.
	 */
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	/**
	 * @brief Handles WM_PAINT to draw the application icon when the window is minimized.
	 */
	afx_msg void OnPaint();

	/**
	 * @brief Returns the cursor to display while the user drags the minimized window.
	 * @return Handle to the cursor to display.
	 */
	afx_msg HCURSOR OnQueryDragIcon();

	/**
	 * @brief Overrides the default cancel behavior to prevent accidental dialog closure.
	 */
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()
};
