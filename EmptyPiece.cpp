// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// EmptyPiece.cpp - Implementation of EmptyPiece, Null Object of Piece

#include "pch.h"
#include "EmptyPiece.hpp"

EmptyPiece::EmptyPiece(bool isWhitePlayer) : Piece(isWhitePlayer)
{
}

EmptyPiece::~EmptyPiece()
{
}

EmptyPiece* EmptyPiece::clone()
{
	return new EmptyPiece(this->isWhitePlayer());
}

// isValidMove(): return special code indicating it is an empty piece
int EmptyPiece::isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board)
{
	UNREFERENCED_PARAMETER(sourceFileRank);
	UNREFERENCED_PARAMETER(destFileRank);
	UNREFERENCED_PARAMETER(board);
	return ChessErrHandler::MOVED_EMPTY_PIECE;
}

wstring EmptyPiece::toString()
{
	return wstring(_T(""));
}

wstring EmptyPiece::toGraphics()
{
	return wstring(_T(" "));
}

int EmptyPiece::Score()
{
	return 0;
}
