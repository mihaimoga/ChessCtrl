// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// Pawn.hpp - Pawn extending Piece

/* Represent Implementation of a Pawn piece in chess
   Including concrete implementation of clone(), isValidMove(), toString()
	 and toGraphics(), inherited from abstract superclass Piece
   Private method isAdjacentFile() used in logic of validing move of a Pawn
*/

#ifndef PAWN_H
#define PAWN_H

#include <map>
#include <string>

#include "Piece.hpp"

class Pawn : public Piece {

public:
	Pawn(bool isWhitePlayer);
	~Pawn();
	Pawn* clone();

	int isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board);
	wstring toString();
	wstring toGraphics();

private:
	bool isAdjacentFile(wstring sourceFileRank, wstring destFileRank);
public:
	int Score(); // Stefan-Mihai Moga
};

#endif
