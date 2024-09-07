// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// Bishop.hpp - Bishop extending Piece

/* Represent Implementation of a Bishop piece in chess
   Including concrete implementation of clone(), isValidMove(), toString()
	 and toGraphics(), inherited from abstract superclass Piece
*/

#ifndef BISHOP_H
#define BISHOP_H

#include <map>
#include <string>

#include "Piece.hpp"

class Bishop : public Piece {

public:
	Bishop(bool isWhitePlayer);
	~Bishop();
	Bishop* clone();

	int isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board);
	wstring toString();
	wstring toGraphics();
public:
	int Score(); // Stefan-Mihai Moga
};

#endif
