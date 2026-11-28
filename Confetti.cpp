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

// Confetti.cpp : implementation file
//

#include "pch.h"
#include "Confetti.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstring>
#include <numbers>
#include <random>
#include <vector>

namespace
{
	constexpr UINT_PTR CONFETTI_TIMER = 1;
	constexpr UINT CONFETTI_INTERVAL = 15; // milliseconds; a timer does not tick faster than about 15.6 ms
	constexpr float CONFETTI_DURATION = 7.0f; // seconds; the window closes itself after this time
	constexpr float CONFETTI_FADE = 1.0f; // seconds; the end of the duration fades out
	constexpr float MAX_STEP = 0.05f; // seconds; a slow frame never moves the pieces further than this
	constexpr float DRAG = 2.0f; // per second; the air slows a piece down to its falling speed
	constexpr float FALL_SPEED = 0.28f; // falling speed of a piece, in screen heights per second
	constexpr float MIN_FLIP = 0.2f; // a piece seen edge-on keeps this part of its width
	constexpr int SUBSAMPLES = 8; // lines per pixel row that anti-alias the edges of a piece
	constexpr float PI = std::numbers::pi_v<float>;

	constexpr COLORREF PALETTE[] =
	{
		RGB(0xE6, 0x39, 0x46), // red
		RGB(0xFF, 0x7F, 0x11), // orange
		RGB(0xFF, 0xC8, 0x00), // yellow
		RGB(0x2E, 0xC4, 0x64), // green
		RGB(0x00, 0xB4, 0xD8), // cyan
		RGB(0x3A, 0x86, 0xFF), // blue
		RGB(0x83, 0x38, 0xEC), // purple
		RGB(0xFF, 0x00, 0x6E), // pink
	};

	// Draws a convex quadrilateral over a premultiplied ARGB bitmap. The edges are anti-aliased with
	// SUBSAMPLES lines per pixel row; every line adds the exact part of each pixel that it crosses.
	void FillQuad(DWORD* pBits, int nWidth, int nHeight, std::vector<float>& coverage,
		const float (&x)[4], const float (&y)[4], float fRed, float fGreen, float fBlue)
	{
		const int nLeft = (std::max)(static_cast<int>(std::floor((std::min)({ x[0], x[1], x[2], x[3] }))), 0);
		const int nRight = (std::min)(static_cast<int>(std::floor((std::max)({ x[0], x[1], x[2], x[3] }))), nWidth - 1);
		const int nTop = (std::max)(static_cast<int>(std::floor((std::min)({ y[0], y[1], y[2], y[3] }))), 0);
		const int nBottom = (std::min)(static_cast<int>(std::floor((std::max)({ y[0], y[1], y[2], y[3] }))), nHeight - 1);
		if ((nLeft > nRight) || (nTop > nBottom))
			return;

		const int nSpan = nRight - nLeft + 1;
		if (coverage.size() < static_cast<size_t>(nSpan))
			coverage.resize(static_cast<size_t>(nSpan));

		for (int nRow = nTop; nRow <= nBottom; nRow++)
		{
			std::fill_n(coverage.begin(), nSpan, 0.0f);
			for (int nSub = 0; nSub < SUBSAMPLES; nSub++)
			{
				// A line crosses the edges of a convex shape twice at most
				const float fLine = static_cast<float>(nRow) + (static_cast<float>(nSub) + 0.5f) / static_cast<float>(SUBSAMPLES);
				float fFrom = FLT_MAX;
				float fTo = -FLT_MAX;
				for (int nEdge = 0; nEdge < 4; nEdge++)
				{
					const int nNext = (nEdge + 1) & 3;
					const float y0 = y[nEdge];
					const float y1 = y[nNext];
					if (((y0 <= fLine) && (fLine < y1)) || ((y1 <= fLine) && (fLine < y0)))
					{
						const float fCross = x[nEdge] + (fLine - y0) * (x[nNext] - x[nEdge]) / (y1 - y0);
						fFrom = (std::min)(fFrom, fCross);
						fTo = (std::max)(fTo, fCross);
					}
				}
				if (fFrom >= fTo)
					continue;

				const int nFirst = (std::max)(static_cast<int>(std::floor(fFrom)), nLeft);
				const int nLast = (std::min)(static_cast<int>(std::floor(fTo)), nRight);
				for (int nCol = nFirst; nCol <= nLast; nCol++)
				{
					const float fPart = (std::min)(fTo, static_cast<float>(nCol + 1)) - (std::max)(fFrom, static_cast<float>(nCol));
					if (fPart > 0.0f)
						coverage[static_cast<size_t>(nCol - nLeft)] += fPart / static_cast<float>(SUBSAMPLES);
				}
			}

			DWORD* pPixel = pBits + static_cast<size_t>(nRow) * static_cast<size_t>(nWidth) + static_cast<size_t>(nLeft);
			for (int nCol = 0; nCol < nSpan; nCol++, pPixel++)
			{
				const float fAlpha = (std::min)(coverage[static_cast<size_t>(nCol)], 1.0f);
				if (fAlpha <= 0.0f)
					continue;

				// The color is blended over the old one, both multiplied by their alpha
				const float fRest = 1.0f - fAlpha;
				const DWORD dwOld = *pPixel;
				const DWORD dwAlpha = static_cast<DWORD>(fAlpha * 255.0f + static_cast<float>(dwOld >> 24) * fRest + 0.5f);
				const DWORD dwRed = static_cast<DWORD>(fRed * fAlpha + static_cast<float>((dwOld >> 16) & 0xFF) * fRest + 0.5f);
				const DWORD dwGreen = static_cast<DWORD>(fGreen * fAlpha + static_cast<float>((dwOld >> 8) & 0xFF) * fRest + 0.5f);
				const DWORD dwBlue = static_cast<DWORD>(fBlue * fAlpha + static_cast<float>(dwOld & 0xFF) * fRest + 0.5f);
				*pPixel = (dwAlpha << 24) | (dwRed << 16) | (dwGreen << 8) | dwBlue;
			}
		}
	}
}

// A transparent window as big as the work area of a monitor, which draws the pieces of confetti itself
class CConfettiWnd : public CWnd
{
public:
	void Run(const CRect& rcScreen, HWND hOwner);

protected:
	virtual void PostNcDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()

private:
	struct Particle
	{
		float x = 0.0f; // center of the piece, in pixels
		float y = 0.0f;
		float vx = 0.0f; // velocity, in pixels per second
		float vy = 0.0f;
		float fall = 0.0f; // falling speed, in pixels per second
		float angle = 0.0f; // rotation in the plane of the screen, in radians
		float spin = 0.0f; // radians per second
		float flip = 0.0f; // turn around its long axis, in radians; it makes the piece flutter
		float flipRate = 0.0f;
		float sway = 0.0f; // swing from side to side, the argument of a sine
		float swayRate = 0.0f;
		float swayWidth = 0.0f; // in pixels
		float halfWidth = 0.0f;
		float halfHeight = 0.0f;
		float delay = 0.0f; // seconds until the piece enters
		float red = 0.0f; // 0 to 255
		float green = 0.0f;
		float blue = 0.0f;
		bool done = false; // the piece has fallen out of the screen
	};

	bool CreateCanvas();
	void Launch();
	void Advance(float fStep);
	void Draw(const Particle& piece);
	bool Render(float fFade);

	CRect m_rcScreen;
	CBitmap m_bmpCanvas;
	CDC m_dcCanvas;
	DWORD* m_pBits = nullptr;
	std::vector<Particle> m_particles;
	std::vector<float> m_coverage;
	std::chrono::steady_clock::time_point m_tLast;
	float m_fTime = 0.0f;
};

static CConfettiWnd* g_pActiveConfetti = nullptr;

BEGIN_MESSAGE_MAP(CConfettiWnd, CWnd)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// Creates the window and starts the animation. The object deletes itself when the animation ends or
// cannot start, and MFC calls PostNcDestroy when the window cannot be created, so the caller must not
// use or delete it after this call
void CConfettiWnd::Run(const CRect& rcScreen, HWND hOwner)
{
	m_rcScreen = rcScreen;
	// Layered and transparent for the mouse, always on top, and never active
	if (!CreateEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
		AfxRegisterWndClass(0), _T(""), WS_POPUP, rcScreen.left, rcScreen.top, rcScreen.Width(), rcScreen.Height(), hOwner, nullptr))
		return;

	g_pActiveConfetti = this;
	if (!CreateCanvas())
	{
		DestroyWindow();
		return;
	}

	Launch();
	m_tLast = std::chrono::steady_clock::now();
	if (!Render(1.0f) || (SetTimer(CONFETTI_TIMER, CONFETTI_INTERVAL, nullptr) == 0))
	{
		DestroyWindow();
		return;
	}
	ShowWindow(SW_SHOWNOACTIVATE);
}

void CConfettiWnd::PostNcDestroy()
{
	if (g_pActiveConfetti == this)
		g_pActiveConfetti = nullptr;
	delete this;
}

void CConfettiWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != CONFETTI_TIMER)
	{
		CWnd::OnTimer(nIDEvent);
		return;
	}

	const std::chrono::steady_clock::time_point tNow = std::chrono::steady_clock::now();
	const float fStep = (std::min)(std::chrono::duration<float>(tNow - m_tLast).count(), MAX_STEP);
	m_tLast = tNow;
	m_fTime += fStep;
	Advance(fStep);

	const float fFade = std::clamp((CONFETTI_DURATION - m_fTime) / CONFETTI_FADE, 0.0f, 1.0f);
	const bool bFalling = std::any_of(m_particles.begin(), m_particles.end(), [](const Particle& piece) { return !piece.done; });
	if (!bFalling || (fFade <= 0.0f) || !Render(fFade))
		DestroyWindow(); // this deletes the object
}

// A top-down 32-bit bitmap, which the window shows through UpdateLayeredWindow
bool CConfettiWnd::CreateCanvas()
{
	BITMAPINFO info = {};
	info.bmiHeader.biSize = sizeof(info.bmiHeader);
	info.bmiHeader.biWidth = m_rcScreen.Width();
	info.bmiHeader.biHeight = -m_rcScreen.Height();
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;

	void* pBits = nullptr;
	HBITMAP hBitmap = ::CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &pBits, nullptr, 0);
	if ((hBitmap == nullptr) || (pBits == nullptr))
		return false;

	m_bmpCanvas.Attach(hBitmap);
	if (!m_dcCanvas.CreateCompatibleDC(nullptr))
		return false;
	m_dcCanvas.SelectObject(&m_bmpCanvas);
	m_pBits = static_cast<DWORD*>(pBits);
	return true;
}

// Throws two bursts from the bottom corners, and lets a rain of pieces fall from the top
void CConfettiWnd::Launch()
{
	std::mt19937 generator{ std::random_device{}() };
	const auto random = [&generator](float fLow, float fHigh)
	{
		return std::uniform_real_distribution<float>(fLow, fHigh)(generator);
	};

	const float fWidth = static_cast<float>(m_rcScreen.Width());
	const float fHeight = static_cast<float>(m_rcScreen.Height());
	const float fScale = std::clamp(fHeight / 1080.0f, 0.8f, 2.5f); // size of the pieces
	const float fDensity = std::clamp(fWidth / 1920.0f, 0.5f, 1.5f); // number of the pieces

	// A piece with its own color, size and way of falling, which is not placed yet
	const auto create = [&]()
	{
		Particle piece;
		const COLORREF color = PALETTE[std::uniform_int_distribution<size_t>(0, std::size(PALETTE) - 1)(generator)];
		piece.red = static_cast<float>(GetRValue(color));
		piece.green = static_cast<float>(GetGValue(color));
		piece.blue = static_cast<float>(GetBValue(color));
		if (random(0.0f, 1.0f) < 0.2f) // a streamer
		{
			piece.halfWidth = random(2.0f, 3.0f) * fScale;
			piece.halfHeight = random(11.0f, 18.0f) * fScale;
		}
		else
		{
			piece.halfWidth = random(4.5f, 7.5f) * fScale;
			piece.halfHeight = random(2.5f, 4.5f) * fScale;
		}
		piece.angle = random(0.0f, 2.0f * PI);
		piece.spin = random(-6.0f, 6.0f);
		piece.flip = random(0.0f, 2.0f * PI);
		piece.flipRate = random(4.0f, 11.0f) * ((random(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f);
		piece.sway = random(0.0f, 2.0f * PI);
		piece.swayRate = random(2.0f, 4.5f);
		piece.swayWidth = random(8.0f, 24.0f) * fScale;
		piece.fall = random(0.85f, 1.15f) * FALL_SPEED * fHeight;
		return piece;
	};

	const int nBurst = static_cast<int>(70.0f * fDensity);
	const int nRain = static_cast<int>(130.0f * fDensity);
	m_particles.clear();
	m_particles.reserve(static_cast<size_t>(2 * nBurst + nRain));

	for (int nSide = 0; nSide < 2; nSide++)
	{
		const float fDirection = (nSide == 0) ? 1.0f : -1.0f;
		for (int nPiece = 0; nPiece < nBurst; nPiece++)
		{
			Particle piece = create();
			const float fAngle = random(45.0f, 80.0f) * PI / 180.0f;
			const float fSpeed = random(1.0f, 2.6f) * fHeight;
			const float fEdge = random(0.0f, 0.02f) * fWidth;
			piece.x = (nSide == 0) ? fEdge : fWidth - fEdge;
			piece.y = fHeight - random(0.0f, 0.02f) * fHeight;
			piece.vx = fDirection * fSpeed * std::cos(fAngle);
			piece.vy = -fSpeed * std::sin(fAngle);
			piece.delay = random(0.0f, 0.12f);
			m_particles.push_back(piece);
		}
	}

	for (int nPiece = 0; nPiece < nRain; nPiece++)
	{
		Particle piece = create();
		piece.x = random(0.0f, fWidth);
		piece.y = -(piece.halfWidth + piece.halfHeight + piece.swayWidth) - random(0.0f, 0.1f) * fHeight;
		piece.vx = random(-0.03f, 0.03f) * fHeight;
		piece.vy = piece.fall * random(0.5f, 1.0f);
		piece.delay = random(0.0f, 1.5f);
		m_particles.push_back(piece);
	}
}

// Moves the pieces for fStep seconds. The air slows a piece down in proportion to its speed, and the
// motion over the step is computed exactly, so a slow frame does not change where a piece goes.
void CConfettiWnd::Advance(float fStep)
{
	const float fHeight = static_cast<float>(m_rcScreen.Height());
	for (Particle& piece : m_particles)
	{
		if (piece.done)
			continue;

		float fMove = fStep;
		if (piece.delay > 0.0f)
		{
			if (piece.delay >= fStep)
			{
				piece.delay -= fStep;
				continue;
			}
			fMove = fStep - piece.delay;
			piece.delay = 0.0f;
		}

		const float fKeep = std::exp(-DRAG * fMove);
		const float fSlide = (1.0f - fKeep) / DRAG;
		piece.x += piece.vx * fSlide;
		piece.vx *= fKeep;
		piece.y += piece.fall * fMove + (piece.vy - piece.fall) * fSlide;
		piece.vy = piece.fall + (piece.vy - piece.fall) * fKeep;
		piece.angle += piece.spin * fMove;
		piece.flip += piece.flipRate * fMove;
		piece.sway += piece.swayRate * fMove;

		const float fReach = piece.halfWidth + piece.halfHeight + piece.swayWidth;
		if ((piece.vy > 0.0f) && (piece.y - fReach > fHeight))
			piece.done = true;
	}
}

void CConfettiWnd::Draw(const Particle& piece)
{
	// The flip narrows the piece, and its two faces have different brightness
	const float fFlip = std::cos(piece.flip);
	const float fHalfWidth = (std::max)(piece.halfWidth * (std::max)(std::fabs(fFlip), MIN_FLIP), 0.6f);
	const float fShade = ((fFlip >= 0.0f) ? 1.0f : 0.72f) * (0.8f + 0.2f * std::fabs(fFlip));
	const float fCos = std::cos(piece.angle);
	const float fSin = std::sin(piece.angle);
	const float fCenter = piece.x + piece.swayWidth * std::sin(piece.sway);

	const float fCornerX[4] = { -fHalfWidth, fHalfWidth, fHalfWidth, -fHalfWidth };
	const float fCornerY[4] = { -piece.halfHeight, -piece.halfHeight, piece.halfHeight, piece.halfHeight };
	float x[4] = {};
	float y[4] = {};
	for (int nCorner = 0; nCorner < 4; nCorner++)
	{
		x[nCorner] = fCenter + fCornerX[nCorner] * fCos - fCornerY[nCorner] * fSin;
		y[nCorner] = piece.y + fCornerX[nCorner] * fSin + fCornerY[nCorner] * fCos;
	}
	FillQuad(m_pBits, m_rcScreen.Width(), m_rcScreen.Height(), m_coverage, x, y,
		piece.red * fShade, piece.green * fShade, piece.blue * fShade);
}

// Draws the pieces that are on the screen, and shows them with the given overall opacity
bool CConfettiWnd::Render(float fFade)
{
	const int nWidth = m_rcScreen.Width();
	const int nHeight = m_rcScreen.Height();
	std::memset(m_pBits, 0, static_cast<size_t>(nWidth) * static_cast<size_t>(nHeight) * sizeof(DWORD));
	for (const Particle& piece : m_particles)
	{
		if (!piece.done && (piece.delay <= 0.0f))
			Draw(piece);
	}

	BLENDFUNCTION blend = { AC_SRC_OVER, 0, static_cast<BYTE>(fFade * 255.0f + 0.5f), AC_SRC_ALPHA };
	POINT ptSource = { 0, 0 };
	POINT ptScreen = { m_rcScreen.left, m_rcScreen.top };
	SIZE size = { nWidth, nHeight };
	return ::UpdateLayeredWindow(m_hWnd, nullptr, &ptScreen, &size, m_dcCanvas.GetSafeHdc(), &ptSource, 0, &blend, ULW_ALPHA) != FALSE;
}

void ShowConfetti(CWnd* pOwner)
{
	CWnd* pTopLevel = ((pOwner != nullptr) && ::IsWindow(pOwner->GetSafeHwnd())) ? pOwner->GetTopLevelParent() : nullptr;
	if ((pTopLevel != nullptr) && (pTopLevel->IsIconic() || !pTopLevel->IsWindowVisible()))
		return;

	if (g_pActiveConfetti != nullptr)
		g_pActiveConfetti->DestroyWindow(); // this deletes it and clears the pointer

	const HMONITOR hMonitor = (pTopLevel != nullptr)
		? ::MonitorFromWindow(pTopLevel->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST)
		: ::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
	CRect rcScreen(0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));
	MONITORINFO monitor = {};
	monitor.cbSize = sizeof(monitor);
	if (::GetMonitorInfo(hMonitor, &monitor))
		rcScreen = monitor.rcWork; // a window over the whole monitor would make the shell move the taskbar aside
	if (rcScreen.IsRectEmpty())
		return;

	CConfettiWnd* pConfetti = new CConfettiWnd();
	pConfetti->Run(rcScreen, (pTopLevel != nullptr) ? pTopLevel->GetSafeHwnd() : nullptr);
}
