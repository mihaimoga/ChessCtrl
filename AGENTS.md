# AGENTS.md

## Project overview

ChessDemo is a native Windows desktop chess application built with MFC and C++. The solution consists of three Visual Studio C++ projects: `ChessDemo.vcxproj`, the application; `Benchmark/Benchmark.vcxproj`, a console benchmark harness for the chess engine; and `UnitTests/UnitTests.vcxproj`, the unit tests of the chess engine. All of them target Unicode on Win32 and x64.

## Repository layout

- `ChessDemo.cpp`, `ChessDemo.h` — MFC application entry point and initialization.
- `ChessDemoDlg.cpp`, `ChessDemoDlg.h` — main dialog and UI event handling.
- `ChessBoard.cpp`, `ChessBoard.hpp` — chess game state, move validation, and board operations.
- `Piece.hpp` and `*Piece.cpp`/`*Piece.hpp` files — abstract piece behavior and concrete chess-piece rules.
- `ComputerPlayer.cpp` — the computer player: `BoardToPosition` copies a `Board` to a `ChessSearch::Position`, `SearchMove` roots the search on the moves that the game accepts, and `ComputerThreadProc` runs it on a thread; `COMPUTER_LEVEL` (depth in plies) and `SEARCH_MAX_NODES` (positions per move) set how hard it thinks.
- `ChessSearch.hpp`, `ChessSearch.cpp` — the search engine of the computer player: a compact `ChessSearch::Position` with its own move generator and evaluation, and `FindBestMove` (iterative-deepening alpha-beta search). It never touches a `Board`.
- `ChessCtrl.*`, `HLinkCtrl.*`, `EdgeWebBrowser.*`, `WebBrowserDlg.*` — custom controls and browser-related UI.
- `Confetti.h`, `Confetti.cpp` — `ShowConfetti`, the confetti that falls over the work area of the screen when a game ends: a topmost, click-through layered window with per-pixel transparency, drawn by its own small software rasteriser on a timer, which destroys itself when the pieces have fallen or after seven seconds. It is user interface code, so it is compiled into `ChessDemo` only, not into `Benchmark` or `UnitTests`.
- `Messages.h` — the private window messages of the application (`MSG_NAVIGATE`, `MSG_RUN_ASYNC_CALLBACK`, `MSG_GAME_ENDED`).
- `ChessDemo.rc`, `Resource.h`, `res/` — Windows resources; maintain resource identifiers consistently across `.rc` and `Resource.h`.
- `pch.h`, `pch.cpp` — precompiled-header configuration.
- `packages.config` — native NuGet dependencies, including WebView2 and WIL.
- `Benchmark/` — `Benchmark.vcxproj`, `Benchmark.cpp`, and `corpus.txt` (sample games); a console harness that compiles the chess-engine sources from the repository root to benchmark `ComputerThreadProc` and to verify that engine changes keep the same rules, moves, and board states. Unlike `ChessDemo`, which builds into the output folders of the solution (`x64\Debug` and `x64\Release`, or `Debug` and `Release` for Win32), it builds into its own folder, `Benchmark\<Platform>\<Configuration>\` (for example `Benchmark\x64\Release\Benchmark.exe`), with the intermediate files in `obj\` below it.
- `UnitTests/` — `UnitTests.vcxproj`, `TestSupport.hpp` (position and move helpers), and the test sources (`PieceTests.cpp`, `PawnTests.cpp`, `ChessBoardTests.cpp`, `KingSafetyTests.cpp`, `ChessSearchTests.cpp`, `ComputerPlayerTests.cpp`); a Microsoft Native C++ Unit Test Framework (CppUnitTest) DLL that compiles the chess-engine sources from the repository root and tests the piece rules, the game rules, and the computer player's search. Like `Benchmark`, it builds into its own folder, `UnitTests\<Platform>\<Configuration>\` (for example `UnitTests\x64\Debug\UnitTests.dll`), with the intermediate files in `obj\` below it, and a post-build step runs its tests.
- `Setup/Setup.vdproj` — the Visual Studio Installer project; the solution build does not include it. It packages the `Release|x64` outputs `ChessDemo.exe`, `Benchmark.exe` and `UnitTests.dll`, the help page, and the release notes. The `SourcePath` of the `ChessDemo.exe` and `UnitTests.dll` entries names the built file (the one of `Benchmark.exe` is empty), so update the entries when a project or its output is renamed or moved: `ChessDemo.exe` is in `x64\Release`, `Benchmark.exe` is in `Benchmark\x64\Release`, and `UnitTests.dll` is in `UnitTests\x64\Release`.

## Build and validation

- Open `ChessDemo.sln` in Visual Studio 2022 or later with the installed C++ desktop/MFC workload and Windows SDK.
- Supported configurations are `Debug` and `Release` for `Win32` and `x64`; prefer `Debug|x64` for development.
- Build from a Developer PowerShell with:
  `msbuild ChessDemo.vcxproj /p:Configuration=Debug /p:Platform=x64`
- `msbuild ChessDemo.sln /p:Configuration=Debug /p:Platform=x64` builds all three projects and runs the unit tests (see below); the solution platforms are `x64` and `x86` (the latter maps to the `Win32` project configurations).
- The project uses toolset `v145`, C++ latest language mode, warning level 4, SDL checks, and static MFC. Keep changes warning-free.
- `UnitTests` holds the automated tests of the chess engine. Building it runs them: a post-build step starts `vstest.console.exe` on the new `UnitTests.dll`, so `msbuild ChessDemo.sln /t:UnitTests /p:Configuration=Debug /p:Platform=x64` from a Developer PowerShell builds and tests in one step, and a failing test fails the build (`error MSB3073`, exit code 1). The DLL is in `UnitTests\x64\Debug\` (`UnitTests\Win32\Debug\` for Win32, and `Release` instead of `Debug` for a Release build). To run the tests again without building, use Test Explorer or `vstest.console.exe UnitTests\x64\Debug\UnitTests.dll /Platform:x64`. Run them after every chess-engine change, and add or update tests when a rule changes.
- The unit tests do not cover the UI (dialogs, controls, browser). For UI changes, build the affected configuration and manually exercise the relevant dialog/control and chess workflow. The confetti shows when a game ends; to see it without playing a game to the end, post `MSG_GAME_ENDED` to the chess control (`IDC_CHESS_STATIC`).
- `Benchmark` is a console benchmark and equivalence harness, not a pass/fail test suite. Run it from the `Benchmark` folder so `corpus.txt` is found, for example `x64\Release\Benchmark.exe thread corpus.txt 3 10`; run it without arguments to list its modes (`gen`, `rules`, `search`, `thread`, `game`). Use `Release` for timings.
- For chess-engine changes, run `rules corpus.txt 24 6000`, `search corpus.txt 8 300 500 300`, `thread corpus.txt 3 10`, and `game 8 24` before and after the change and compare the output: hashes, chosen moves, and scores must stay identical (only the timing and exception-count columns may differ) unless the change is meant to alter them, as the legal-move guarantee below did for the moves the old search picked and `submitMove` refused: then check that only those differ. The search of `ChessSearch.cpp` replaced the old one on purpose, so it changed `search`, `thread`, and `game`; `rules` only depends on the rules of the game and must stay identical unless a rule changes. After a change of the search, `thread` and `game` also show the time of a move: keep the longest one in `Release` around one second. To measure another version of the engine, build `Benchmark.vcxproj` with `/p:SrcDir=<folder with the engine sources> /p:OutDir=<out>\ /p:IntDir=<obj>\`.

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
- `ChessBoard::endTheGame()` is the one place where a game ends (checkmate or stalemate; on the UI thread for a move of the human, on the computer thread for a move of the computer). It posts `MSG_GAME_ENDED` to the chess control (`m_pParentWnd`, which is `nullptr` in `Benchmark` and `UnitTests`), and `CChessCtrl::OnGameEnded` shows the confetti. Keep it a posted message: the engine files are compiled into `Benchmark` and `UnitTests` without the UI files, so they must not call UI code such as `ShowConfetti` directly.
- `Benchmark/Benchmark.vcxproj` and `UnitTests/UnitTests.vcxproj` list the engine `.cpp` files explicitly; update both when engine source files are added or removed (and `ChessDemo.vcxproj` with its `.filters`). The `ChessSearch.cpp` item of `Benchmark.vcxproj` is conditional on `Exists('$(SrcDir)\ChessSearch.cpp')`, so that an older engine without that file can still be measured with `/p:SrcDir`.
- Unit tests describe a position as the piece-placement field of a FEN record (see `UnitTests/TestSupport.hpp`). Every position played through `ChessBoard` needs both kings. `ComputerPlayer.cpp` has no header, so `ComputerPlayerTests.cpp` declares the functions it tests (`WhitePlayer_BacktrackingAlgorithm`, `BlackPlayer_BacktrackingAlgorithm`, `ComputerThreadProc`, `BoardToPosition`) itself; keep those declarations in sync with their definitions. `ChessSearchTests.cpp` tests the engine on positions written the same way (`ParseSearchPosition`): qualify `ChessSearch::Position`, because the tests have a `ChessTests::Position` of their own.
- The computer player must only play moves that `ChessBoard::submitMove` accepts, otherwise the game freezes with the human locked out: `SearchMove` roots the search on the moves of the `ChessSearch` generator that `isValidMove` and `IsLegalMove` (king safety, tried on a copy of the board) accept, the search itself only plays moves that keep the king safe, and `SearchMove` returns `false` when there is none (checkmate or stalemate), when the game is stopped, or when the board holds something the search does not know. Keep that guarantee when changing the search or the rules.
- `ChessSearch.cpp` follows the rules of the engine, which are not all the rules of chess: no castling, en passant or promotion, a pawn on the last rank has no moves, and a pawn advances two squares only from its starting rank. When a rule changes, change the move generator of `ChessSearch.cpp` as well: `ComputerMovesTests` compares it with the moves that the game accepts, in special positions and in random games.
- The search counts positions and never time, so a position always gets the same move on any computer and in any build; it also polls `g_bThreadRunning`, which closing the dialog clears, to stop at once. Keep it that way. After changing the search, check that the new version still wins against the previous one from varied openings with both colors, and that the time of a move in `Release` stays around one second at most.
- The test classes are in `namespace UnitTests`, so the fully qualified name of a test is `UnitTests::<Class>::<Test>`; the helpers of `TestSupport.hpp` are in `namespace ChessTests`. Put new test classes in `namespace UnitTests`.
- `NewPiece` in `UnitTests/TestSupport.hpp` creates counted pieces: compare `LivePieces()` before and after an operation to detect leaked or double-freed pieces (see `ChessBoardResourceTests`).

## Resources and dependencies

- When adding UI resources, update both `ChessDemo.rc` and `Resource.h`; do not reuse existing numeric resource IDs.
- Do not edit files under `packages/`. Manage dependency declarations through `packages.config` and restore packages through Visual Studio or NuGet.
