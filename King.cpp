// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// King.cpp - implementation of King extending Piece (info in King.hpp)

#include "pch.h"
#include "King.hpp"

King::King(bool isWhitePlayer) : Piece(isWhitePlayer)
{
	_isKing = true;
}

King::~King()
{
}

King* King::clone()
{
	return new King(this->isWhitePlayer());
}

/* A King's move is valid if:
   - The destination is on its adjacent square
   - The (possibly) existing piece at destination is not a friendly
	 (or destination is empty)

   King.isValidMove() post-cond: retrun 0 if move is valid as above
								 respective error code otherwise
*/
int King::isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board)
{

	if (isSameFile(sourceFileRank, destFileRank) &&
		isSameRank(sourceFileRank, destFileRank))
	{
		return ChessErrHandler::DEST_EQ_SOURCE;
	}

	if (!isAdjacentSquare(sourceFileRank, destFileRank))
	{
		return ChessErrHandler::ILLEGAL_MOVE_PATTERN;
	}

	if (destExistFriendlyPiece(destFileRank, board))
	{
		return ChessErrHandler::FRIENDLY_AT_DEST;
	}

	return ChessErrHandler::CHESS_NO_ERROR;
}

wstring King::toString()
{

	wstring name(playerToString());
	name.append(_T(" King"));
	return name;
}

wstring King::toGraphics()
{
	if (_isWhitePlayer) {
		// return wstring("♔");
		return wstring(_T("\x2654"));
	}
	// return wstring("♚");
	return wstring(_T("\x265A"));
}

/* King.isAdjacentSquare():
   pre-cond: args are valid file & rank representations of a different square
   post-cond: return true if the displacement of file and rank is at most 1
*/
bool King::isAdjacentSquare(wstring sourceFileRank, wstring destFileRank)
{

	TCHAR sourceFile = sourceFileRank.at(ChessInfo::FILE_INDEX);
	TCHAR sourceRank = sourceFileRank.at(ChessInfo::RANK_INDEX);
	TCHAR destFile = destFileRank.at(ChessInfo::FILE_INDEX);
	TCHAR destRank = destFileRank.at(ChessInfo::RANK_INDEX);

	return max(abs(destFile - sourceFile), abs(destRank - sourceRank)) == 1;
}

int King::Score()
{
	return 1000;
}
