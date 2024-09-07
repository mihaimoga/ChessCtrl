// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// Knight.hpp - Knight extending Piece

/* Represent Implementation of a Knight piece in chess
   Including concrete implementation of clone(), isValidMove(), toString()
	 and toGraphics(), inherited from abstract superclass Piece
   Private method movesInLShape() used in logic of validing move of a Knight
*/

#ifndef KNIGHT_H
#define KNIGHT_H

#include <map>
#include <string>

#include "Piece.hpp"

class Knight : public Piece {

public:
	Knight(bool isWhitePlayer);
	~Knight();
	Knight* clone();

	int isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board);
	wstring toString();
	wstring toGraphics();

private:
	bool movesInLShape(wstring sourceFileRank, wstring destFileRank);
public:
	int Score(); // Stefan-Mihai Moga
};

#endif
