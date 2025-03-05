// Bryan Liu (chl312), Dept. of Computing, Imperial College London
/* Implementation for abstract class Piece - info in Piece.hpp
   (Constructor, Deconstructor and common methods for extending classes)
*/

#include "pch.h"
#include "Piece.hpp"

Piece::Piece(bool isWhitePlayer)
{
	_isWhitePlayer = isWhitePlayer;
}

Piece::~Piece()
{
}

/* Piece.confirmMove():
   set isFirstMove false when called as it has moved at least once
*/
void Piece::confirmMove()
{
	isFirstMove = false;
}

// Piece.isWhitePlayer() post-cond.: return if this piece belongs to White p.
bool Piece::isWhitePlayer()
{
	return (_isWhitePlayer);
}

/* Piece.isKing() post-cond.: return true if the piece is a King
	 (represented by _isKing - only set to true by King constructor)
*/
bool Piece::isKing()
{
	return _isKing;
}

/* Piece.isFriendly()
   Pre-cond.: that is a valid reference to a Piece
   Post-cond.: return true if this and that Piece belongs to same player
*/
bool Piece::isFriendly(Piece* that)
{
	return this->isWhitePlayer() == that->isWhitePlayer();
}

// Piece.playerToString() Post-cond.: return wstring rep. of the piece's player
wstring Piece::playerToString()
{
	return _isWhitePlayer ? wstring(_T("White's")) : wstring(_T("Black's"));
}

/* Piece::isSame_ ()
   Pre-cond.: sourceFileRank, destFileRank valid file & rank representation
   Post-cond.: return true if sourceFileRank and destFileRank is on the same
			   file/ rank/ diagonal respectively
*/
bool Piece::isSameFile(wstring sourceFileRank, wstring destFileRank)
{
	return sourceFileRank.at(ChessInfo::FILE_INDEX) ==
		destFileRank.at(ChessInfo::FILE_INDEX);
}

bool Piece::isSameRank(wstring sourceFileRank, wstring destFileRank)
{
	return sourceFileRank.at(ChessInfo::RANK_INDEX) ==
		destFileRank.at(ChessInfo::RANK_INDEX);

}

bool Piece::isSameDiagonal(wstring sourceFileRank, wstring destFileRank)
{
	return abs(sourceFileRank.at(ChessInfo::FILE_INDEX) -
		destFileRank.at(ChessInfo::FILE_INDEX)) ==
		abs(sourceFileRank.at(ChessInfo::RANK_INDEX) -
			destFileRank.at(ChessInfo::RANK_INDEX));
}

/* Piece.noVerticalObstruction()
   Pre-cond.: sourceFileRank, destFileRank valid file & rank representation
			  board a valid, existing reference to map rep. of the board
   Post-cond.: return true if there are no pieces on intermediate ranks
			   on the board
*/
bool Piece::noVerticalObstruction(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board)
{

	TCHAR sourceFile = sourceFileRank.at(ChessInfo::FILE_INDEX);
	TCHAR sourceRank = sourceFileRank.at(ChessInfo::RANK_INDEX);
	TCHAR destRank = destFileRank.at(ChessInfo::RANK_INDEX);
	TCHAR lowRank = (sourceRank < destRank) ? sourceRank : destRank;
	TCHAR highRank = (sourceRank < destRank) ? destRank : sourceRank;

	for (TCHAR i = lowRank + ChessInfo::EXCLUSIVE_SHIFT; i < highRank; i++)
	{
		wstring between({ sourceFile, i });
		try
		{
			auto temp = board->at(between);
			UNREFERENCED_PARAMETER(temp);
			return false;
		}
		catch (const std::out_of_range& err)
		{
			UNREFERENCED_PARAMETER(err);
			// No piece for this file & rank, no action required
		}
	}
	return true;
}

/* Piece.noVerticalObstruction()
   Pre-cond.: sourceFileRank, destFileRank valid file & rank representation
			  board a valid, existing reference to map rep. of the board
   Post-cond.: return true if there are no pieces on intermediate files
			   on the board
*/
bool Piece::noHorizontalObstruction(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board)
{

	TCHAR sourceFile = sourceFileRank.at(ChessInfo::FILE_INDEX);
	TCHAR sourceRank = sourceFileRank.at(ChessInfo::RANK_INDEX);
	TCHAR destFile = destFileRank.at(ChessInfo::FILE_INDEX);
	TCHAR lowFile = (sourceFile < destFile) ? sourceFile : destFile;
	TCHAR highFile = (sourceFile < destFile) ? destFile : sourceFile;

	for (TCHAR i = lowFile + ChessInfo::EXCLUSIVE_SHIFT; i < highFile; i++)
	{
		wstring between({ i, sourceRank });
		try
		{
			auto temp = board->at(between);
			UNREFERENCED_PARAMETER(temp);
			return false;
		}
		catch (const std::out_of_range& err)
		{
			UNREFERENCED_PARAMETER(err);
			// No piece for this file & rank, no action required
		}
	}
	return true;
}

/* Piece.noVerticalObstruction()
   Pre-cond.: sourceFileRank, destFileRank valid file & rank representation
			  board a valid, existing reference to map rep. of the board
   Post-cond.: return true if there are no pieces on intermediate files
			   and ranks, which is on the same diagonal, on the board
*/
bool Piece::noDiagonalObstruction(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board)
{

	TCHAR sourceFile = sourceFileRank.at(ChessInfo::FILE_INDEX);
	TCHAR destFile = destFileRank.at(ChessInfo::FILE_INDEX);
	TCHAR lowFile = (sourceFile < destFile) ? sourceFile : destFile;
	TCHAR highFile = (sourceFile < destFile) ? destFile : sourceFile;

	TCHAR sourceRank = sourceFileRank.at(ChessInfo::RANK_INDEX);
	TCHAR destRank = destFileRank.at(ChessInfo::RANK_INDEX);
	TCHAR lowRank = (sourceRank < destRank) ? sourceRank : destRank;
	TCHAR highRank = (sourceRank < destRank) ? destRank : sourceRank;

	bool isPositiveSlope = (destFile - sourceFile) == (destRank - sourceRank);
	wstring between;

	for (TCHAR i = ChessInfo::EXCLUSIVE_SHIFT; i < (highFile - lowFile); i++)
	{
		if (isPositiveSlope) 
		{
			between = { (TCHAR)(lowFile + i), (TCHAR)(lowRank + i) };
		}
		else {
			between = { (TCHAR)(lowFile + i), (TCHAR)(highRank - i) };
		}

		try
		{
			auto temp = board->at(between);
			UNREFERENCED_PARAMETER(temp);
			return false;
		}
		catch (const std::out_of_range& err)
		{
			UNREFERENCED_PARAMETER(err);
			// No piece for this file & rank, no action required
		}
	}
	return true;
}

/* Piece.destExistFriendlyPiece()
   Pre-cond.: destFileRank is a valid file & rank representation
   Post-cond.: return true if piece on the dest file & rank is a friendly,
			   false otherwise (rivalry on dest or empty square)
*/
bool Piece::destExistFriendlyPiece(wstring destFileRank, map<wstring, Piece*>* board) {

	try
	{
		if (isFriendly(board->at(destFileRank)))
		{
			return true;
		}
	}
	catch (const std::out_of_range& err)
	{
		UNREFERENCED_PARAMETER(err);
		// No piece for this file & rank, expect to return false
	}
	return false;
}

int Piece::Score()
{
	return 1;
}
