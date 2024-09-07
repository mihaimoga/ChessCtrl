// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// Piece.hpp - (abstract class) Piece

/* Represent Implementation of a general piece in chess
   Including general, common methods a piece using to validate its own move
	and hook methods clone(), isValidMove(), toString() and toGraphics()
*/

#ifndef PIECES_H
#define PIECES_H

#include <map>
#include <string>
#include <cmath>
#include <stdexcept>

#include "ChessInfo.hpp"

using namespace std;

class Piece {

protected:
	bool _isWhitePlayer;
	bool _isKing = false;
	bool isFirstMove = true;

public:
	Piece(bool isWhitePlayer);
	virtual ~Piece() = 0;

	// Piece.clone(): return a different object with same information of itself
	virtual Piece* clone() = 0;

	void confirmMove();
	bool isWhitePlayer();
	bool isKing();

	/* Piece.isValidMove()
	   Pre-cond.: sourceFileRank, destFileRank are valid file & rank rep.s
				  board is a reference to an existing map implementation of
					current chess board situation
	   Post-cond.: return 0 if the move is valid
				   respective error code (defined in ChessErrHandler class)
					 for error reporting otherwise
	   N.B.: It is decided to include check for source = dest in each
			   concrete implementation of Piece since there is subtlety in
			   method call structure after so-called refactoring and for some
			   piece it might be "valid" for source = dest (say EmptyPiece)
			 Similar for checking friendlies in destination (Pawn has
			   different check and error reporting logic)
	*/
	virtual int isValidMove(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board) = 0;

	wstring playerToString();

	// Piece.toString() Post-cond.: return the wstring rep. of the piece
	virtual wstring toString() = 0;

	// Piece.toGraphics() Post-cond: return graphical rep. of the piece
	virtual wstring toGraphics() = 0;

protected:
	bool isFriendly(Piece* that);

	bool isSameFile(wstring sourceFileRank, wstring destFileRank);
	bool isSameRank(wstring sourceFileRank, wstring destFileRank);
	bool isSameDiagonal(wstring sourceFileRank, wstring destFileRank);

	bool noVerticalObstruction(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board);
	bool noHorizontalObstruction(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board);
	bool noDiagonalObstruction(wstring sourceFileRank, wstring destFileRank, map<wstring, Piece*>* board);

	bool destExistFriendlyPiece(wstring destFileRank, map<wstring, Piece*>* board);

public:
	virtual int Score() = 0; // Stefan-Mihai Moga
};

#include "ChessErrHandler.hpp"

#endif
