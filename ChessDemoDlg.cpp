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

/// @file ChessDemoDlg.cpp
/// @brief Implementation of the main application dialog and the About dialog
///        for the ChessDemo application.

#include "pch.h"
#include "framework.h"
#include "ChessDemo.h"
#include "ChessDemoDlg.h"
#include "WebBrowserDlg.h"

#include "VersionInfo.h"
#include "HLinkCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @class CAboutDlg
 * @brief Modal dialog that displays application version, license information,
 *        and hyperlinks to the project website, email, and contributors page.
 * @extends CDialog
 */
class CAboutDlg : public CDialog
{
public:
	/** @brief Default constructor. Initializes the dialog with IDD_ABOUTBOX. */
	CAboutDlg();

	// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	/**
	 * @brief Exchanges data between dialog controls and member variables.
	 * @param pDX Pointer to the CDataExchange object used for DDX/DDV.
	 */
	virtual void DoDataExchange(CDataExchange* pDX);

	// Implementation
public:
	/**
	 * @brief Initializes the About dialog: populates version, license text,
	 *        and sets hyperlinks for website, email, and contributors.
	 * @return TRUE to set focus to the first control; FALSE if focus is set manually.
	 */
	virtual BOOL OnInitDialog();

	/** @brief Cleans up resources when the dialog is destroyed. */
	afx_msg void OnDestroy();

protected:
	/** @brief Static control displaying the product name and version string. */
	CStatic m_ctrlVersion;

	/** @brief Read-only edit control displaying the GPL license warning text. */
	CEdit m_ctrlWarning;

	/** @brief Helper object used to read the executable's version resource. */
	CVersionInfo m_pVersionInfo;

	/** @brief Hyperlink control navigating to the author's website. */
	CHLinkCtrl m_ctrlWebsite;

	/** @brief Hyperlink control opening a mailto link to the author's email address. */
	CHLinkCtrl m_ctrlEmail;

	/** @brief Hyperlink control navigating to the GitHub contributors graph. */
	CHLinkCtrl m_ctrlContributors;

	DECLARE_MESSAGE_MAP()
};

/**
 * @brief Constructs a CAboutDlg, linking to the IDD_ABOUTBOX dialog resource.
 */
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

/**
 * @brief Binds dialog controls to their corresponding member variables.
 * @param pDX Pointer to the CDataExchange object facilitating DDX/DDV.
 */
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);                              // let base class handle its own DDX first
	DDX_Control(pDX, IDC_VERSION, m_ctrlVersion);              // bind version label
	DDX_Control(pDX, IDC_WARNING, m_ctrlWarning);              // bind GPL license text box
	DDX_Control(pDX, IDC_WEBSITE, m_ctrlWebsite);              // bind website hyperlink
	DDX_Control(pDX, IDC_EMAIL, m_ctrlEmail);                  // bind email hyperlink
	DDX_Control(pDX, IDC_CONTRIBUTORS, m_ctrlContributors);    // bind GitHub contributors hyperlink
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/**
 * @brief Retrieves the full file path of the currently running executable.
 *
 * Dynamically grows the buffer until the path fits or an error occurs,
 * which handles paths longer than MAX_PATH on modern Windows.
 *
 * @param[out] pdwLastError Optional pointer that receives the last Win32 error
 *                          code, or ERROR_SUCCESS on success. May be nullptr.
 * @return The full path of the executable as a CString, or an empty CString on failure.
 */
CString GetModuleFileName(_Inout_opt_ DWORD* pdwLastError = nullptr)
{
	CString strModuleFileName;
	DWORD dwSize{ _MAX_PATH };   // start with a typical path length
	while (true)
	{
		TCHAR* pszModuleFileName{ strModuleFileName.GetBuffer(dwSize) }; // reserve a writable buffer
		const DWORD dwResult{ ::GetModuleFileName(nullptr, pszModuleFileName, dwSize) }; // query the exe path
		if (dwResult == 0)
		{
			// Win32 API failure — capture the error code and return empty
			if (pdwLastError != nullptr)
				*pdwLastError = GetLastError();
			strModuleFileName.ReleaseBuffer(0);  // discard partial content
			return CString{};
		}
		else if (dwResult < dwSize)
		{
			// Path fit in the buffer — seal the string and return it
			if (pdwLastError != nullptr)
				*pdwLastError = ERROR_SUCCESS;
			strModuleFileName.ReleaseBuffer(dwResult); // trim to actual length
			return strModuleFileName;
		}
		else if (dwResult == dwSize)
		{
			// Buffer was exactly full, meaning the path may be truncated — double and retry
			strModuleFileName.ReleaseBuffer(0);
			dwSize *= 2;
		}
	}
}

/**
 * @brief Initializes the About dialog.
 *
 * Loads the version info from the running executable and populates
 * the version label with the product name and version number (32/64-bit).
 * Also sets the GPL license text and configures all hyperlink controls.
 *
 * @return TRUE to set focus to the first control automatically.
 */
BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString strFullPath{ GetModuleFileName() }; // get the running exe's full path
	if (strFullPath.IsEmpty())
#pragma warning(suppress: 26487)
		return FALSE;  // can't read version info without a valid path

	if (m_pVersionInfo.Load(strFullPath.GetString()))  // parse the VS_VERSION_INFO resource
	{
		CString strName = m_pVersionInfo.GetProductName().c_str();            // e.g. "ChessCtrl"
		CString strVersion = m_pVersionInfo.GetProductVersionAsString().c_str(); // e.g. "1, 0, 0, 1"
		strVersion.Replace(_T(" "), _T(""));   // remove spaces between components
		strVersion.Replace(_T(","), _T("."));  // convert comma separators to dots
		const int nFirst  = strVersion.Find(_T('.'));           // position of first dot
		const int nSecond = strVersion.Find(_T('.'), nFirst + 1); // position of second dot
		strVersion.Truncate(nSecond);  // keep only major.minor, drop patch and build
		if (nSecond == (nFirst + 2))   // single-digit minor (e.g. "1.0") — pad to "1.00"
			strVersion.Insert(nFirst + 1, _T("0"));
#if _WIN32 || _WIN64
#if _WIN64
		m_ctrlVersion.SetWindowText(strName + _T(" version ") + strVersion + _T(" (64-bit)"));
#else
		m_ctrlVersion.SetWindowText(strName + _T(" version ") + strVersion + _T(" (32-bit)"));
#endif
#endif
	}

	// Populate the read-only license text box with the standard GPL v3 notice
	m_ctrlWarning.SetWindowText(_T("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>."));

	m_ctrlWebsite.SetHyperLink(_T("https://www.moga.doctor/"));                                           // author's personal website
	m_ctrlEmail.SetHyperLink(_T("mailto:stefan-mihai@moga.doctor"));                                      // direct email link
	m_ctrlContributors.SetHyperLink(_T("https://github.com/mihaimoga/ChessCtrl/graphs/contributors"));    // GitHub contributors graph

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

/**
 * @brief Handles WM_DESTROY; delegates cleanup to the base class.
 */
void CAboutDlg::OnDestroy()
{
	CDialog::OnDestroy();
}

// CChessDemoDlg dialog

/**
 * @brief Constructs the main application dialog.
 *
 * Links to the IDD_CHESSDEMO_DIALOG resource and loads the main frame icon.
 *
 * @param pParent Pointer to the parent window. Defaults to nullptr.
 */
CChessDemoDlg::CChessDemoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHESSDEMO_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

/**
 * @brief Binds dialog controls to their corresponding member variables.
 *
 * Maps IDC_COLOR_STATIC to m_pColorStatic, IDC_PROGRESS to m_ctrlProgress,
 * and IDC_CHESS_STATIC to m_pChessCtrl.
 *
 * @param pDX Pointer to the CDataExchange object facilitating DDX/DDV.
 */
void CChessDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);                           // let base class handle its own DDX first
	DDX_Control(pDX, IDC_COLOR_STATIC, m_pColorStatic);       // bind player color indicator static
	DDX_Control(pDX, IDC_PROGRESS,     m_ctrlProgress);       // bind engine thinking progress bar
	DDX_Control(pDX, IDC_CHESS_STATIC, m_pChessCtrl);         // bind the chess board control
}

BEGIN_MESSAGE_MAP(CChessDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

// CChessDemoDlg message handlers

/**
 * @brief Initializes the main dialog.
 *
 * Adds extended entries to the system menu (About, social media links,
 * GitHub issue tracker, discussions, wiki, and user manual). Sets the
 * dialog icons, wires up the chess control with the color static and
 * progress bar, and enables the computer player.
 *
 * @return TRUE to set focus to the first control automatically.
 */
BOOL CChessDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);  // retrieve a copy of the window's system menu
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);  // load the "About" caption from resources
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);                      // visual divider before About
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu); // "About ChessDemo..."
		}
		pSysMenu->AppendMenu(MF_SEPARATOR);                          // divider before social media links
		pSysMenu->AppendMenu(MF_STRING, IDM_TWITTER,   _T("Twitter"));   // author's Twitter/X profile
		pSysMenu->AppendMenu(MF_STRING, IDM_LINKEDIN,  _T("LinkedIn"));  // author's LinkedIn profile
		pSysMenu->AppendMenu(MF_STRING, IDM_FACEBOOK,  _T("Facebook"));  // author's Facebook profile
		pSysMenu->AppendMenu(MF_STRING, IDM_INSTAGRAM, _T("Instagram")); // author's Instagram profile
		pSysMenu->AppendMenu(MF_SEPARATOR);                          // divider before GitHub links
		pSysMenu->AppendMenu(MF_STRING, IDM_ISSUES,      _T("Issues"));      // GitHub issue tracker
		pSysMenu->AppendMenu(MF_STRING, IDM_DISCUSSIONS, _T("Discussions")); // GitHub discussions
		pSysMenu->AppendMenu(MF_STRING, IDM_WIKI,        _T("Wiki"));        // GitHub project wiki
		pSysMenu->AppendMenu(MF_SEPARATOR);                          // divider before documentation
		pSysMenu->AppendMenu(MF_STRING, IDM_USER_MANUAL, _T("User Manual")); // embedded user manual
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Wire up the chess control's back-references so it can update the UI directly
	m_pChessCtrl.m_pColorStatic = &m_pColorStatic;  // lets the control update the color indicator
	m_pChessCtrl.m_ctrlProgress = &m_ctrlProgress;  // lets the control drive the progress bar
	m_pChessCtrl.SetUI();                            // perform initial board rendering
	m_pChessCtrl.SetComputerPlayer(true);            // enable the computer opponent

	return TRUE;  // return TRUE  unless you set the focus to a control
}

/**
 * @brief Handles system menu command messages.
 *
 * Dispatches the following custom commands:
 * - IDM_ABOUTBOX   : Opens the About dialog.
 * - IDM_TWITTER    : Opens the author's Twitter/X profile in the default browser.
 * - IDM_LINKEDIN   : Opens the author's LinkedIn profile in the default browser.
 * - IDM_FACEBOOK   : Opens the author's Facebook profile in the default browser.
 * - IDM_INSTAGRAM  : Opens the author's Instagram profile in the default browser.
 * - IDM_ISSUES     : Opens the GitHub issues page in the default browser.
 * - IDM_DISCUSSIONS: Opens the GitHub discussions page in the default browser.
 * - IDM_WIKI       : Opens the GitHub wiki page in the default browser.
 * - IDM_USER_MANUAL: Opens the embedded WebBrowser dialog pointing to the user manual.
 *
 * All unrecognized commands are forwarded to the base class.
 *
 * @param nID    The system command identifier.
 * @param lParam Additional message-specific data.
 */
void CChessDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)  // mask low nibble; standard MFC pattern for custom sys commands
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();  // show the About dialog modally
	}
	else
	{
		if (nID == IDM_TWITTER)  // open author's Twitter/X profile
		{
			::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://x.com/stefanmihaimoga"), nullptr, nullptr, SW_SHOW);
		}
		else
		{
			if (nID == IDM_LINKEDIN)  // open author's LinkedIn profile
			{
				::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://www.linkedin.com/in/stefanmihaimoga/"), nullptr, nullptr, SW_SHOW);
			}
			else
			{
				if (nID == IDM_FACEBOOK)  // open author's Facebook profile
				{
					::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://www.facebook.com/stefanmihaimoga"), nullptr, nullptr, SW_SHOW);
				}
				else
				{
					if (nID == IDM_INSTAGRAM)  // open author's Instagram profile
					{
						::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://www.instagram.com/stefanmihaimoga/"), nullptr, nullptr, SW_SHOW);
					}
					else
					{
						if (nID == IDM_ISSUES)  // open GitHub issue tracker
						{
							::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://github.com/mihaimoga/ChessCtrl/issues"), nullptr, nullptr, SW_SHOW);
						}
						else
						{
							if (nID == IDM_DISCUSSIONS)  // open GitHub discussions page
							{
								::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://github.com/mihaimoga/ChessCtrl/discussions"), nullptr, nullptr, SW_SHOW);
							}
							else
							{
								if (nID == IDM_WIKI)  // open GitHub project wiki
								{
									::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://github.com/mihaimoga/ChessCtrl/wiki"), nullptr, nullptr, SW_SHOW);
								}
								else
								{
									if (nID == IDM_USER_MANUAL)  // open embedded web browser with user manual
									{
										CWebBrowserDlg dlgWebBrowser(this);
										dlgWebBrowser.DoModal();
									}
									else
									{
										CDialog::OnSysCommand(nID, lParam);  // forward unrecognized commands to the base class
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

/**
 * @brief Handles WM_PAINT.
 *
 * When the window is minimized, manually centers and draws the application
 * icon in the client area. Otherwise, delegates to the base class painter.
 */
void CChessDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

/**
 * @brief Returns the cursor to display while the user drags the minimized window.
 *
 * Casts the application icon handle to an HCURSOR so the icon follows
 * the mouse when the window is dragged in its minimized state.
 *
 * @return HCURSOR representing the application icon.
 */
HCURSOR CChessDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

extern bool g_bThreadRunning;
bool WaitWithMessageLoop(HANDLE hEvent, DWORD dwTimeout);

/**
 * @brief Handles the Cancel action (Escape key or close button).
 *
 * If the computer-player thread is still running, signals it to stop by
 * clearing g_bThreadRunning and waits for the thread to finish via a
 * message-pumping wait loop before delegating to the base class cancel handler.
 */
void CChessDemoDlg::OnCancel()
{
	if (g_bThreadRunning)  // computer player thread is still active
	{
		g_bThreadRunning = false;  // signal the thread to exit its loop
		// pump messages while waiting so the UI stays responsive during shutdown
		VERIFY(WaitWithMessageLoop(m_pChessCtrl.m_pChessBoard.m_hComputerThread, INFINITE));
	}
	CDialogEx::OnCancel();  // proceed with normal dialog teardown
}
