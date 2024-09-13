// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// Queen.hpp - Queen extending Piece

/* Represent Implementation of a Queen piece in chess
   Including concrete implementation of clone(), isValidMove(), toString()
	 and toGraphics(), inherited from abstract superclass Piece
*/

#ifndef QUEEN_H
#define QUEEN_H

#include <map>
#include <string>

#include "Piece.hpp"

class Queen : public Piece {

public:
	Queen(bool isWhitePlayer);
	~Queen();
	Queen* clone();

	int isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board);
	wstring toString();
	wstring toGraphics();
public:
	int Score(); // Stefan-Mihai Moga
};

#endif
