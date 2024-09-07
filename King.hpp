// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// King.hpp - King extending Piece

/* Represent Implementation of a King piece in chess
   Including concrete implementation of clone(), isValidMove(), toString()
	 and toGraphics(), inherited from abstract superclass Piece
   Private method isAdjacentSquare() used in logic of validing move of a King
*/

#ifndef KING_H
#define KING_H

#include <map>
#include <string>

#include "Piece.hpp"

class King : public Piece {

public:
	King(bool isWhitePlayer);
	~King();
	King* clone();

	int isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board);
	wstring toString();
	wstring toGraphics();

private:
	bool isAdjacentSquare(wstring sourceFileRank, wstring destFileRank);
public:
	int Score(); // Stefan-Mihai Moga
};

#endif
