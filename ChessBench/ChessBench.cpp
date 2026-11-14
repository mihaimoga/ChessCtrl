// ChessBench - standalone benchmark and equivalence harness for ComputerThreadProc (not part of the product).
// It compiles the engine sources of the solution folder, or the ones of the SrcDir MSBuild property (for example
// a "git archive" of an older commit, built with /p:SrcDir=<folder> /p:OutDir=<folder>\out\ /p:IntDir=<folder>\obj\),
// so that two versions of the engine can be run on the same corpus and the lines they print compared.
#include "pch.h"
#include "ChessBoard.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

extern bool g_bThreadRunning;
bool WhitePlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard);
bool BlackPlayer_BacktrackingAlgorithm(const int nLevel, int& nScore, wstring& strMoveFrom, wstring& strMoveTo, Board* pBoard);
DWORD WINAPI ComputerThreadProc(LPVOID lpParam);

namespace
{
	typedef std::pair<wstring, wstring> Move;
	typedef std::vector<Move> MoveList;
	typedef bool (*SearchFn)(const int, int&, wstring&, wstring&, Board*);

	double g_dFrequency = 1.0;
	volatile LONG64 g_nExceptions = 0;

	double NowMs()
	{
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		return 1000.0 * static_cast<double>(counter.QuadPart) / g_dFrequency;
	}

	// Counts every C++ exception (MSVC code 0xE06D7363) raised while installed.
	LONG CALLBACK CountCxxExceptions(PEXCEPTION_POINTERS pInfo)
	{
		if (pInfo->ExceptionRecord->ExceptionCode == 0xE06D7363UL)
		{
			InterlockedIncrement64(&g_nExceptions);
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}

	struct Rng
	{
		uint64_t m_state;
		explicit Rng(uint64_t seed) : m_state(seed * 0x9E3779B97F4A7C15ULL + 0x1234567ULL)
		{
			Next();
			Next();
		}
		uint32_t Next()
		{
			m_state = m_state * 6364136223846793005ULL + 1442695040888963407ULL;
			return static_cast<uint32_t>(m_state >> 33);
		}
		uint32_t Below(uint32_t n) { return Next() % n; }
	};

	uint64_t Mix(uint64_t h, uint64_t v)
	{
		h ^= v;
		h *= 1099511628211ULL;
		return h;
	}

	const uint64_t HASH_SEED = 1469598103934665603ULL;

	std::string Narrow(const wstring& s)
	{
		std::string r;
		for (wchar_t c : s)
		{
			r.push_back(static_cast<char>(c));
		}
		return r;
	}

	Board* BenchClone(Board* b)
	{
		Board* c = new Board;
		for (Board::iterator it = b->begin(); it != b->end(); ++it)
		{
			c->insert({ it->first, it->second->clone() });
		}
		return c;
	}

	void BenchFree(Board* b)
	{
		for (Board::iterator it = b->begin(); it != b->end(); ++it)
		{
			delete it->second;
		}
		delete b;
	}

	// Same effect as ChessBoard::confirmMoveOnBoard (real flags: confirmMove() is called).
	void ApplyMove(Board* b, const wstring& from, const wstring& to)
	{
		Piece* p = b->at(from);
		Board::iterator it = b->find(to);
		if (it != b->end())
		{
			delete it->second;
			it->second = p;
		}
		else
		{
			b->insert({ to, p });
		}
		p->confirmMove();
		b->erase(from);
	}

	uint64_t HashBoard(Board* b)
	{
		uint64_t h = HASH_SEED;
		for (Board::iterator it = b->begin(); it != b->end(); ++it)
		{
			for (wchar_t c : it->first)
			{
				h = Mix(h, c);
			}
			for (wchar_t c : it->second->toString())
			{
				h = Mix(h, c);
			}
		}
		return h;
	}

	// Hash of isValidMove() for every piece against all 64 squares (including its own).
	uint64_t RulesHash(Board* b, int& nValid)
	{
		uint64_t h = HASH_SEED;
		nValid = 0;
		for (Board::iterator it = b->begin(); it != b->end(); ++it)
		{
			for (TCHAR r = ChessInfo::MAX_RANK; r >= ChessInfo::MIN_RANK; --r)
			{
				for (TCHAR f = ChessInfo::MIN_FILE; f <= ChessInfo::MAX_FILE; ++f)
				{
					const int code = it->second->isValidMove(it->first, wstring({ f, r }), b);
					h = Mix(h, static_cast<uint64_t>(code) + 1);
					if (code == ChessErrHandler::CHESS_NO_ERROR)
					{
						++nValid;
					}
				}
			}
		}
		return h;
	}

	bool KingAttacked(Board* b, bool white)
	{
		const wstring* pKing = nullptr;
		for (Board::iterator it = b->begin(); it != b->end(); ++it)
		{
			if (it->second->isKing() && (it->second->isWhitePlayer() == white))
			{
				pKing = &it->first;
				break;
			}
		}
		if (pKing == nullptr)
		{
			return false;
		}
		for (Board::iterator it = b->begin(); it != b->end(); ++it)
		{
			if ((it->second->isWhitePlayer() != white) && (it->second->isValidMove(it->first, *pKing, b) == ChessErrHandler::CHESS_NO_ERROR))
			{
				return true;
			}
		}
		return false;
	}

	// Pseudo-legal moves of the engine, minus king captures and moves that leave the own king attacked.
	void LegalMoves(Board* b, bool white, MoveList& out)
	{
		out.clear();
		for (Board::iterator it = b->begin(); it != b->end(); ++it)
		{
			if (it->second->isWhitePlayer() != white)
			{
				continue;
			}
			for (TCHAR r = ChessInfo::MAX_RANK; r >= ChessInfo::MIN_RANK; --r)
			{
				for (TCHAR f = ChessInfo::MIN_FILE; f <= ChessInfo::MAX_FILE; ++f)
				{
					wstring to({ f, r });
					if (it->second->isValidMove(it->first, to, b) != ChessErrHandler::CHESS_NO_ERROR)
					{
						continue;
					}
					Board::iterator victim = b->find(to);
					if ((victim != b->end()) && victim->second->isKing())
					{
						continue;
					}
					Board* c = BenchClone(b);
					ApplyMove(c, it->first, to);
					const bool bSafe = !KingAttacked(c, white);
					BenchFree(c);
					if (bSafe)
					{
						out.push_back({ it->first, to });
					}
				}
			}
		}
	}

	bool GivesCheck(Board* b, const Move& m, bool white)
	{
		Board* c = BenchClone(b);
		ApplyMove(c, m.first, m.second);
		const bool bCheck = KingAttacked(c, !white);
		BenchFree(c);
		return bCheck;
	}

	Piece* MakePiece(uint32_t type, bool white)
	{
		static const int kTypes[] = { 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 5 };
		switch (kTypes[type % 11])
		{
		case 0: return new Pawn(white);
		case 1: return new Knight(white);
		case 2: return new Bishop(white);
		case 3: return new Rook(white);
		case 4: return new Queen(white);
		default: return new King(white);
		}
	}

	// Arbitrary (not necessarily reachable) position, with random "has moved" flags.
	Board* RandomBoard(Rng& rng, uint32_t nPieces)
	{
		Board* b = new Board;
		while (b->size() < nPieces)
		{
			wstring sq({ static_cast<TCHAR>(ChessInfo::MIN_FILE + rng.Below(8)), static_cast<TCHAR>(ChessInfo::MIN_RANK + rng.Below(8)) });
			if (b->find(sq) != b->end())
			{
				continue;
			}
			Piece* p = MakePiece(rng.Below(11), rng.Below(2) == 0);
			if (rng.Below(2) == 0)
			{
				p->confirmMove();
			}
			b->insert({ sq, p });
		}
		return b;
	}

	MoveList ParseLine(const std::string& line)
	{
		MoveList moves;
		std::istringstream ss(line);
		std::string tok;
		ss >> tok; // game id
		while (ss >> tok)
		{
			if (tok.size() == 4)
			{
				moves.push_back({ wstring({ static_cast<wchar_t>(tok[0]), static_cast<wchar_t>(tok[1]) }),
					wstring({ static_cast<wchar_t>(tok[2]), static_cast<wchar_t>(tok[3]) }) });
			}
		}
		return moves;
	}

	std::vector<MoveList> LoadCorpus(const char* path)
	{
		std::vector<MoveList> games;
		std::ifstream in(path);
		std::string line;
		while (std::getline(in, line))
		{
			MoveList m = ParseLine(line);
			if (!m.empty())
			{
				games.push_back(m);
			}
		}
		return games;
	}

	// Ply 1 goes through submitMove (E2-E4) so that it is Black's turn afterwards; the rest is applied directly.
	std::unique_ptr<ChessBoard> BuildPosition(const MoveList& moves, size_t nPlies)
	{
		std::unique_ptr<ChessBoard> cb(new ChessBoard());
		cb->submitMove(moves[0].first.c_str(), moves[0].second.c_str());
		for (size_t i = 1; i < nPlies; ++i)
		{
			ApplyMove(cb->_board, moves[i].first, moves[i].second);
		}
		return cb;
	}

	struct Outcome
	{
		bool ok;
		std::string from, to;
		int score;
		bool operator==(const Outcome& o) const { return ok == o.ok && from == o.from && to == o.to && score == o.score; }
	};

	Outcome RunOnce(SearchFn fn, int nLevel, Board* board)
	{
		g_bThreadRunning = true;
		int nScore = 777777; // sentinel: detects "nScore left untouched"
		wstring from, to;
		const bool ok = fn(nLevel, nScore, from, to, board);
		return { ok, Narrow(from), Narrow(to), nScore };
	}

	void MeasureSearch(const char* label, const char* fnName, SearchFn fn, int nLevel, Board* board, double budgetMs)
	{
		g_nExceptions = 0;
		PVOID hVeh = AddVectoredExceptionHandler(1, CountCxxExceptions);
		Outcome first = RunOnce(fn, nLevel, board);
		RemoveVectoredExceptionHandler(hVeh);
		const LONG64 nExceptions = g_nExceptions;

		std::vector<double> times;
		double estimate = 0.0;
		{
			const double t0 = NowMs();
			Outcome o = RunOnce(fn, nLevel, board);
			estimate = NowMs() - t0;
			times.push_back(estimate);
			if (!(o == first))
			{
				printf("!! NONDETERMINISTIC %s %s\n", label, fnName);
			}
		}
		const int nReps = (estimate > budgetMs) ? 0 : static_cast<int>((std::min)(200.0, (std::max)(2.0, budgetMs / (std::max)(estimate, 0.001))));
		for (int i = 0; i < nReps; ++i)
		{
			const double t0 = NowMs();
			Outcome o = RunOnce(fn, nLevel, board);
			times.push_back(NowMs() - t0);
			if (!(o == first))
			{
				printf("!! NONDETERMINISTIC %s %s\n", label, fnName);
			}
		}
		std::sort(times.begin(), times.end());
		const double dMin = times.front();
		const double dMed = times[times.size() / 2];
		printf("S|%s|%s%d|%d|%s|%s|%d|%.4f|%.4f|%lld\n", label, fnName, nLevel, first.ok ? 1 : 0, first.from.c_str(), first.to.c_str(), first.score, dMin, dMed, nExceptions);
		fflush(stdout);
	}

	void MeasureAll(const char* label, Board* board, double budgetMs, bool bLevel2)
	{
		if (bLevel2)
		{
			MeasureSearch(label, "B", BlackPlayer_BacktrackingAlgorithm, 2, board, budgetMs);
			MeasureSearch(label, "W", WhitePlayer_BacktrackingAlgorithm, 2, board, budgetMs);
		}
		MeasureSearch(label, "B", BlackPlayer_BacktrackingAlgorithm, 1, board, budgetMs);
		MeasureSearch(label, "W", WhitePlayer_BacktrackingAlgorithm, 1, board, budgetMs);
	}

	const int kPlies[] = { 1, 3, 5, 7, 9, 13, 17, 25, 33, 45, 61, 81 };

	int Gen(const char* path, int nGames, int nMaxPlies)
	{
		std::ofstream out(path);
		for (int g = 0; g < nGames; ++g)
		{
			Rng rng(1000 + g);
			std::unique_ptr<ChessBoard> cb(new ChessBoard());
			Board* b = cb->_board;
			MoveList line;
			ApplyMove(b, L"E2", L"E4");
			line.push_back({ L"E2", L"E4" });
			bool white = false;
			for (int ply = 1; ply < nMaxPlies; ++ply)
			{
				MoveList legal;
				LegalMoves(b, white, legal);
				if (legal.empty())
				{
					break;
				}
				const Move m = legal[rng.Below(static_cast<uint32_t>(legal.size()))];
				ApplyMove(b, m.first, m.second);
				line.push_back(m);
				white = !white;
			}
			out << g;
			for (const Move& m : line)
			{
				out << ' ' << Narrow(m.first) << Narrow(m.second);
			}
			out << '\n';
		}
		return 0;
	}

	int Rules(const char* corpusPath, int nGames, int nFuzz)
	{
		std::vector<MoveList> corpus = LoadCorpus(corpusPath);
		for (int g = 0; g < nGames && g < static_cast<int>(corpus.size()); ++g)
		{
			std::unique_ptr<ChessBoard> cb(new ChessBoard());
			int nValid = 0;
			uint64_t h = RulesHash(cb->_board, nValid);
			printf("R|%d|0|%016llx|%d\n", g, h, nValid);
			for (size_t ply = 0; ply < corpus[g].size(); ++ply)
			{
				ApplyMove(cb->_board, corpus[g][ply].first, corpus[g][ply].second);
				h = RulesHash(cb->_board, nValid);
				printf("R|%d|%u|%016llx|%d\n", g, static_cast<unsigned>(ply + 1), h, nValid);
			}
		}
		for (int i = 0; i < nFuzz; ++i)
		{
			Rng rng(777000 + i);
			Board* b = RandomBoard(rng, 2 + rng.Below(30));
			int nValid = 0;
			const uint64_t h = RulesHash(b, nValid);
			printf("F|%d|%016llx|%d\n", i, h, nValid);
			BenchFree(b);
		}
		fflush(stdout);
		return 0;
	}

	int Search(const char* corpusPath, int nGames, int nFuzz, double budgetMs, int nFuzzLevel2)
	{
		std::vector<MoveList> corpus = LoadCorpus(corpusPath);
		for (int g = 0; g < nGames && g < static_cast<int>(corpus.size()); ++g)
		{
			for (int nPlies : kPlies)
			{
				if (nPlies > static_cast<int>(corpus[g].size()))
				{
					break;
				}
				if ((g > 0) && (nPlies == 1))
				{
					continue;
				}
				std::unique_ptr<ChessBoard> cb = BuildPosition(corpus[g], static_cast<size_t>(nPlies));
				char label[64];
				sprintf_s(label, "G%d.P%d", g, nPlies);
				MeasureAll(label, cb->_board, budgetMs, true);
			}
		}
		for (int i = 0; i < nFuzz; ++i)
		{
			Rng rng(5000 + i);
			Board* b = RandomBoard(rng, 4 + rng.Below(17));
			char label[64];
			sprintf_s(label, "F%d", i);
			MeasureAll(label, b, budgetMs, i < nFuzzLevel2);
			BenchFree(b);
		}
		return 0;
	}

	int ThreadMode(const char* corpusPath, int nGames, int nReps)
	{
		static const int kThreadPlies[] = { 1, 5, 9, 15, 25, 41 };
		std::vector<MoveList> corpus = LoadCorpus(corpusPath);
		for (int g = 0; g < nGames && g < static_cast<int>(corpus.size()); ++g)
		{
			for (int nPlies : kThreadPlies)
			{
				if (nPlies > static_cast<int>(corpus[g].size()))
				{
					break;
				}
				if ((g > 0) && (nPlies == 1))
				{
					continue;
				}
				std::vector<double> times;
				uint64_t hashAfter = 0;
				bool bMoved = false;
				LONG64 nExceptions = 0;
				for (int rep = 0; rep < nReps; ++rep)
				{
					std::unique_ptr<ChessBoard> cb = BuildPosition(corpus[g], static_cast<size_t>(nPlies));
					const uint64_t hashBefore = HashBoard(cb->_board);
					PVOID hVeh = nullptr;
					if (rep == 0)
					{
						g_nExceptions = 0;
						hVeh = AddVectoredExceptionHandler(1, CountCxxExceptions);
					}
					const double t0 = NowMs();
					ComputerThreadProc(cb.get());
					const double dt = NowMs() - t0;
					if (rep == 0)
					{
						RemoveVectoredExceptionHandler(hVeh);
						nExceptions = g_nExceptions;
					}
					else
					{
						times.push_back(dt);
					}
					hashAfter = HashBoard(cb->_board);
					bMoved = (hashAfter != hashBefore);
				}
				std::sort(times.begin(), times.end());
				printf("T|G%d.P%d|%d|%016llx|%.4f|%.4f|%lld\n", g, nPlies, bMoved ? 1 : 0, hashAfter, times.empty() ? 0.0 : times.front(), times.empty() ? 0.0 : times[times.size() / 2], nExceptions);
				fflush(stdout);
			}
		}
		return 0;
	}

	// Scripted game: White plays seeded random quiet moves, Black is the real ComputerThreadProc.
	int GameMode(int nGames, int nMaxPlies)
	{
		for (int g = 0; g < nGames; ++g)
		{
			Rng rng(9000 + g);
			std::unique_ptr<ChessBoard> cb(new ChessBoard());
			uint64_t chain = HASH_SEED;
			int nPlies = 0, nComputer = 0;
			double dTotal = 0.0, dMax = 0.0;
			while ((nPlies < nMaxPlies) && !cb->HasGameEnded())
			{
				const bool bWhiteTurn = cb->IsWhiteTurn();
				if (bWhiteTurn)
				{
					MoveList legal, quiet;
					LegalMoves(cb->_board, true, legal);
					for (const Move& m : legal)
					{
						if (!GivesCheck(cb->_board, m, true))
						{
							quiet.push_back(m);
						}
					}
					const MoveList& pool = quiet.empty() ? legal : quiet;
					if (pool.empty())
					{
						break;
					}
					const Move m = pool[rng.Below(static_cast<uint32_t>(pool.size()))];
					cb->submitMove(m.first.c_str(), m.second.c_str());
				}
				else
				{
					const double t0 = NowMs();
					ComputerThreadProc(cb.get());
					const double dt = NowMs() - t0;
					dTotal += dt;
					dMax = (std::max)(dMax, dt);
					++nComputer;
				}
				++nPlies;
				if (cb->IsWhiteTurn() == bWhiteTurn)
				{
					break; // the move was rejected: stuck
				}
				chain = Mix(chain, HashBoard(cb->_board));
			}
			printf("G|%d|%d|%d|%016llx|%.3f|%.3f\n", g, nPlies, nComputer, chain, dTotal, dMax);
			fflush(stdout);
		}
		return 0;
	}

	void Usage()
	{
		fprintf(stderr, "usage: ChessBench gen <corpus> <games> <plies>\n"
			"       ChessBench rules <corpus> <games> <fuzz>\n"
			"       ChessBench search <corpus> <games> <fuzz> <budgetMs> <fuzzLevel2>\n"
			"       ChessBench thread <corpus> <games> <reps>\n"
			"       ChessBench game <games> <plies>\n");
	}
}

int main(int argc, char** argv)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	g_dFrequency = static_cast<double>(freq.QuadPart);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(1) << 2);
	std::cout.setstate(std::ios_base::failbit);

	if (argc < 2)
	{
		Usage();
		return 2;
	}
	const std::string mode = argv[1];
	if (mode == "gen" && argc >= 5)
	{
		return Gen(argv[2], atoi(argv[3]), atoi(argv[4]));
	}
	if (mode == "rules" && argc >= 5)
	{
		return Rules(argv[2], atoi(argv[3]), atoi(argv[4]));
	}
	if (mode == "search" && argc >= 7)
	{
		return Search(argv[2], atoi(argv[3]), atoi(argv[4]), atof(argv[5]), atoi(argv[6]));
	}
	if (mode == "thread" && argc >= 5)
	{
		return ThreadMode(argv[2], atoi(argv[3]), atoi(argv[4]));
	}
	if (mode == "game" && argc >= 4)
	{
		return GameMode(atoi(argv[2]), atoi(argv[3]));
	}
	Usage();
	return 2;
}
