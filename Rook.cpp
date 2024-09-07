// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// Rook.cpp - implementation of Rook extending Piece (info in Rook.hpp)

#include "pch.h"
#include "Rook.hpp"

Rook::Rook(bool isWhitePlayer) : Piece(isWhitePlayer)
{
}

Rook::~Rook()
{
}

Rook* Rook::clone()
{
	return new Rook(this->isWhitePlayer());
}

/* A Rook's move is valid if:
   - Destination is on the same file/rank (note: mutually exclusive)
   - There are no other pieces in intermediate spaces
   - Piece at destination, if exist, is not a friendly
	 (or there are no piece at destination)

   Rook.isValidMove() post-cond: return 0 if move is valid
								 respective error code otherwise
*/
int Rook::isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board)
{

	if (isSameFile(sourceFileRank, destFileRank) &&
		isSameRank(sourceFileRank, destFileRank))
	{
		return ChessErrHandler::DEST_EQ_SOURCE;
	}

	if (!(isSameFile(sourceFileRank, destFileRank) ||
		isSameRank(sourceFileRank, destFileRank)))
	{
		return ChessErrHandler::ILLEGAL_MOVE_PATTERN;
	}

	/* Obstruction for a Rook only occurs if it is on its intended way
	   Lazy boolean evalution of C++ ensure !no_Obstruction is evaluated only
		after satisfying isSame_, prevents breaking pre-cond of no_Obstruction
	*/
	if ((isSameFile(sourceFileRank, destFileRank) &&
		!noVerticalObstruction(sourceFileRank, destFileRank, board)) ||
		(isSameRank(sourceFileRank, destFileRank) &&
			!noHorizontalObstruction(sourceFileRank, destFileRank, board))) 
	{
		return ChessErrHandler::OBSTRUCTION_EN_ROUTE;
	}

	if (destExistFriendlyPiece(destFileRank, board))
	{
		return ChessErrHandler::FRIENDLY_AT_DEST;
	}

	return ChessErrHandler::CHESS_NO_ERROR;
}

wstring Rook::toString()
{

	wstring name(playerToString());
	name.append(_T(" Rook"));
	return name;
}

wstring Rook::toGraphics()
{

	if (_isWhitePlayer) {
		// return wstring("♖");
		return wstring(_T("\x2656"));
	}
	// return wstring("♜");
	return wstring(_T("\x265C"));
}

int Rook::Score()
{
	return 10;
}
