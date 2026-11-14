# AGENTS.md

## Project overview

ChessDemo is a native Windows desktop chess application built with MFC and C++. The solution consists of three Visual Studio C++ projects: `ChessDemo.vcxproj`, the application; `ChessBench/ChessBench.vcxproj`, a console benchmark harness for the chess engine; and `ChessUnittests/ChessUnittests.vcxproj`, the unit tests of the chess engine. All of them target Unicode on Win32 and x64.

## Repository layout

- `ChessDemo.cpp`, `ChessDemo.h` — MFC application entry point and initialization.
- `ChessDemoDlg.cpp`, `ChessDemoDlg.h` — main dialog and UI event handling.
- `ChessBoard.cpp`, `ChessBoard.hpp` — chess game state, move validation, and board operations.
- `Piece.hpp` and `*Piece.cpp`/`*Piece.hpp` files — abstract piece behavior and concrete chess-piece rules.
- `ChessCtrl.*`, `HLinkCtrl.*`, `EdgeWebBrowser.*`, `WebBrowserDlg.*` — custom controls and browser-related UI.
- `ChessDemo.rc`, `Resource.h`, `res/` — Windows resources; maintain resource identifiers consistently across `.rc` and `Resource.h`.
- `pch.h`, `pch.cpp` — precompiled-header configuration.
- `packages.config` — native NuGet dependencies, including WebView2 and WIL.
- `ChessBench/` — `ChessBench.vcxproj`, `ChessBench.cpp`, and `corpus.txt` (sample games); a console harness that compiles the chess-engine sources from the repository root to benchmark `ComputerThreadProc` and to verify that engine changes keep the same rules, moves, and board states.
- `ChessUnittests/` — `ChessUnittests.vcxproj`, `TestSupport.hpp` (position and move helpers), and the test sources (`PieceTests.cpp`, `PawnTests.cpp`, `ChessBoardTests.cpp`, `KingSafetyTests.cpp`, `ComputerPlayerTests.cpp`); a Microsoft Native C++ Unit Test Framework (CppUnitTest) DLL that compiles the chess-engine sources from the repository root and tests the piece rules, the game rules, and the computer player's search.

## Build and validation

- Open `ChessDemo.sln` in Visual Studio 2022 or later with the installed C++ desktop/MFC workload and Windows SDK.
- Supported configurations are `Debug` and `Release` for `Win32` and `x64`; prefer `Debug|x64` for development.
- Build from a Developer PowerShell with:
  `msbuild ChessDemo.vcxproj /p:Configuration=Debug /p:Platform=x64`
- `msbuild ChessDemo.sln /p:Configuration=Debug /p:Platform=x64` builds all three projects; the solution platforms are `x64` and `x86` (the latter maps to the `Win32` project configurations).
- The project uses toolset `v145`, C++ latest language mode, warning level 4, SDL checks, and static MFC. Keep changes warning-free.
- `ChessUnittests` holds the automated tests of the chess engine. Run them from Test Explorer, or build and run them from a Developer PowerShell with `msbuild ChessDemo.sln /t:ChessUnittests /p:Configuration=Debug /p:Platform=x64` and `vstest.console.exe x64\Debug\ChessUnittests.dll /Platform:x64` (the `x86` DLL is in `Debug\`). Run them after every chess-engine change, and add or update tests when a rule changes.
- The unit tests do not cover the UI (dialogs, controls, browser). For UI changes, build the affected configuration and manually exercise the relevant dialog/control and chess workflow.
- `ChessBench` is a console benchmark and equivalence harness, not a pass/fail test suite. Run it from the `ChessBench` folder so `corpus.txt` is found, for example `..\x64\Release\ChessBench.exe thread corpus.txt 3 10`; run it without arguments to list its modes (`gen`, `rules`, `search`, `thread`, `game`). Use `Release` for timings.
- For chess-engine changes, run `rules corpus.txt 24 6000`, `search corpus.txt 8 300 500 300`, `thread corpus.txt 3 10`, and `game 8 24` before and after the change and compare the output: hashes, chosen moves, and scores must stay identical (only the timing and exception-count columns may differ) unless the change is meant to alter them, as the legal-move guarantee below did for the moves the old search picked and `submitMove` refused: then check that only those differ. To measure another version of the engine, build `ChessBench.vcxproj` with `/p:SrcDir=<folder with the engine sources> /p:OutDir=<out>\ /p:IntDir=<obj>\`.

## C++ and MFC conventions

- Include `pch.h` first in every `.cpp` file that participates in the precompiled-header build.
- Preserve the existing MFC message-map macros, `TCHAR`/`_T()` Unicode usage, and `CString`/`std::wstring` conventions where already used.
- Keep headers and implementation files paired using the repository's existing names and extensions (`.hpp` for chess-engine headers, `.h` for MFC/UI headers).
- Match the surrounding formatting: tabs for indentation, opening braces on the following line, and concise existing-style comments only where needed.
- Avoid unrelated refactors, formatting churn, or changes to generated Visual Studio project metadata.

## Chess-engine changes

- Preserve board ownership rules: boards hold raw `Piece*` values and cleanup is performed explicitly with `deepCleanBoard`; cloned boards and captured pieces must be released on every path.
- Keep move validation behavior consistent with `ChessErrHandler` error codes and existing `Piece::isValidMove` contracts.
- When adding or changing a piece rule, account for source/destination validation, friendly-piece handling, king safety, turn switching, and clone behavior as applicable.
- `ChessBench/ChessBench.vcxproj` and `ChessUnittests/ChessUnittests.vcxproj` list the engine `.cpp` files explicitly; update both when engine source files are added or removed.
- Unit tests describe a position as the piece-placement field of a FEN record (see `ChessUnittests/TestSupport.hpp`). Every position played through `ChessBoard` needs both kings. `ComputerPlayer.cpp` has no header, so `ComputerPlayerTests.cpp` declares the search functions itself; keep those declarations in sync with their definitions.
- The computer player must only play moves that `ChessBoard::submitMove` accepts, otherwise the game freezes with the human locked out: the search ignores king safety, so `SearchLegalMove` searches again among the moves that keep the king safe when the best move would be refused, and returns `false` when there is none (checkmate or stalemate). Keep that guarantee when changing the search.
- `NewPiece` in `ChessUnittests/TestSupport.hpp` creates counted pieces: compare `LivePieces()` before and after an operation to detect leaked or double-freed pieces (see `ChessBoardResourceTests`).

## Resources and dependencies

- When adding UI resources, update both `ChessDemo.rc` and `Resource.h`; do not reuse existing numeric resource IDs.
- Do not edit files under `packages/`. Manage dependency declarations through `packages.config` and restore packages through Visual Studio or NuGet.
