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

#pragma once

/**
 * @brief Shows an animated confetti on the monitor of the given window.
 * @param pOwner Window of the application that celebrates; the confetti belongs to its top-level window
 * and is closed with it. It may be nullptr, and nothing is shown while the top-level window is minimized.
 *
 * The confetti is drawn on a transparent window over the work area of the monitor (the taskbar stays
 * untouched), which never takes the focus and lets every mouse click through. The window closes itself
 * when the animation ends, and a new call replaces the one that is still running. Call it only from the
 * user interface thread.
 */
void ShowConfetti(CWnd* pOwner);
