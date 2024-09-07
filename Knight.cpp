// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// Knight.cpp - implementation of Knight extending Piece (info in Knight.hpp)

#include "pch.h"
#include "Knight.hpp"

Knight::Knight(bool isWhitePlayer) : Piece(isWhitePlayer)
{
}

Knight::~Knight()
{
}

Knight* Knight::clone()
{
	return new Knight(this->isWhitePlayer());
}

/* A Knight's move is valid if:
   - It moves in "L"-pattern
   - The (possibly) existing piece on destination square is not a friendly
	 (or destination is empty)

   Knight.isValidMove() post-cond: retrun 0 if move is valid as above
								   respective error code otherwise
*/
int Knight::isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board)
{

	if (isSameFile(sourceFileRank, destFileRank) &&
		isSameRank(sourceFileRank, destFileRank))
	{
		return ChessErrHandler::DEST_EQ_SOURCE;
	}

	if (!movesInLShape(sourceFileRank, destFileRank))
	{
		return ChessErrHandler::ILLEGAL_MOVE_PATTERN;
	}

	if (destExistFriendlyPiece(destFileRank, board))
	{
		return ChessErrHandler::FRIENDLY_AT_DEST;
	}

	return ChessErrHandler::CHESS_NO_ERROR;
}

wstring Knight::toString()
{

	wstring name(playerToString());
	name.append(_T(" Knight"));
	return name;
}

wstring Knight::toGraphics()
{

	if (_isWhitePlayer) {
		// return wstring("♘");
		return wstring(_T("\x2658"));
	}
	// return wstring("♞");
	return wstring(_T("\x265E"));
}

/* Knight.movesInLShape()
   pre-cond: args are valid file & rank representations of a different square
   post-cond: return true if abs file diff = 2(1) & abs rank diff = 1(2)
*/
bool Knight::movesInLShape(wstring sourceFileRank, wstring destFileRank)
{

	TCHAR sourceFile = sourceFileRank.at(ChessInfo::FILE_INDEX);
	TCHAR sourceRank = sourceFileRank.at(ChessInfo::RANK_INDEX);
	TCHAR destFile = destFileRank.at(ChessInfo::FILE_INDEX);
	TCHAR destRank = destFileRank.at(ChessInfo::RANK_INDEX);

	return abs(sourceFile - destFile) * abs(sourceRank - destRank) == 2;
}

int Knight::Score()
{
	return 10;
}
