# AGENTS.md

## Project overview

ChessDemo is a native Windows desktop chess application built with MFC and C++. The solution consists of one Visual Studio project, `ChessDemo.vcxproj`, targeting Unicode Windows applications on Win32 and x64.

## Repository layout

- `ChessDemo.cpp`, `ChessDemo.h` — MFC application entry point and initialization.
- `ChessDemoDlg.cpp`, `ChessDemoDlg.h` — main dialog and UI event handling.
- `ChessBoard.cpp`, `ChessBoard.hpp` — chess game state, move validation, and board operations.
- `Piece.hpp` and `*Piece.cpp`/`*Piece.hpp` files — abstract piece behavior and concrete chess-piece rules.
- `ChessCtrl.*`, `HLinkCtrl.*`, `EdgeWebBrowser.*`, `WebBrowserDlg.*` — custom controls and browser-related UI.
- `ChessDemo.rc`, `Resource.h`, `res/` — Windows resources; maintain resource identifiers consistently across `.rc` and `Resource.h`.
- `pch.h`, `pch.cpp` — precompiled-header configuration.
- `packages.config` — native NuGet dependencies, including WebView2 and WIL.

## Build and validation

- Open `ChessDemo.vcxproj` in Visual Studio 2022 or later with the installed C++ desktop/MFC workload and Windows SDK.
- Supported configurations are `Debug` and `Release` for `Win32` and `x64`; prefer `Debug|x64` for development.
- Build from a Developer PowerShell with:
  `msbuild ChessDemo.vcxproj /p:Configuration=Debug /p:Platform=x64`
- The project uses toolset `v145`, C++ latest language mode, warning level 4, SDL checks, and static MFC. Keep changes warning-free.
- No automated test project is currently present. For behavior changes, build the affected configuration and manually exercise the relevant dialog/control and chess workflow.

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

## Resources and dependencies

- When adding UI resources, update both `ChessDemo.rc` and `Resource.h`; do not reuse existing numeric resource IDs.
- Do not edit files under `packages/`. Manage dependency declarations through `packages.config` and restore packages through Visual Studio or NuGet.
