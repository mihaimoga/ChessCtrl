// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// Queen.cpp - implementation of Queen extending Piece (info in Queen.hpp)

#include "pch.h"
#include "Queen.hpp"

Queen::Queen(bool isWhitePlayer) : Piece(isWhitePlayer)
{
}

Queen::~Queen()
{
}

Queen* Queen::clone() {
	return new Queen(this->isWhitePlayer());
}

/* A Queen's move is valid if:
   - Destination is on the same file/rank/diagonal (note: mutually exclusive)
   - There are no other pieces in intermediate spaces
   - Piece at destination, if exist, is not a friendly
	 (or there are no piece at destination)

   Queen.isValidMove() post-cond: return 0 if move is valid
								  respective error code otherwise
*/
int Queen::isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board)
{

	if (isSameFile(sourceFileRank, destFileRank) &&
		isSameRank(sourceFileRank, destFileRank))
	{
		return ChessErrHandler::DEST_EQ_SOURCE;
	}

	if (!(isSameFile(sourceFileRank, destFileRank) ||
		isSameRank(sourceFileRank, destFileRank) ||
		isSameDiagonal(sourceFileRank, destFileRank)))
	{
		return ChessErrHandler::ILLEGAL_MOVE_PATTERN;
	}

	/* Obstruction for a Queen only occurs if it is on its intended way
	   Lazy boolean evalution of C++ ensure !no_Obstruction is evaluated
		only after isSame_ is satisfied, preventing breaking pre-cond of
		no_Obstruction methods
	*/
	if ((isSameFile(sourceFileRank, destFileRank) &&
		!noVerticalObstruction(sourceFileRank, destFileRank, board)) ||
		(isSameRank(sourceFileRank, destFileRank) &&
			!noHorizontalObstruction(sourceFileRank, destFileRank, board)) ||
		(isSameDiagonal(sourceFileRank, destFileRank) &&
			!noDiagonalObstruction(sourceFileRank, destFileRank, board))) {
		return ChessErrHandler::OBSTRUCTION_EN_ROUTE;
	}

	if (destExistFriendlyPiece(destFileRank, board)) {
		return ChessErrHandler::FRIENDLY_AT_DEST;
	}

	return ChessErrHandler::CHESS_NO_ERROR;
}

wstring Queen::toString()
{

	wstring name(playerToString());
	name.append(_T(" Queen"));
	return name;
}

wstring Queen::toGraphics()
{

	if (_isWhitePlayer)
	{
		// return wstring("♕");
		return wstring(_T("\x2655"));
	}
	// return wstring("♛");
	return wstring(_T("\x265B"));
}

int Queen::Score()
{
	return 100;
}
