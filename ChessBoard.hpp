// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// ChessBoard.hpp - ChessBoard 
// (Representing implementation of a Chess game engine)

#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include <map>
#include <iostream>
#include <string>
#include <stdexcept>

#include "ChessErrHandler.hpp"
#include "ChessInfo.hpp"

#include "Piece.hpp"
#include "EmptyPiece.hpp"
#include "King.hpp"
#include "Queen.hpp"
#include "Bishop.hpp"
#include "Rook.hpp"
#include "Knight.hpp"
#include "Pawn.hpp"

using namespace std;

/* Improving clarity - the map of wstring (file & rank) to (ref. to) Piece would
					   be simply known as Board (as it represents a board)
*/
typedef map<wstring, Piece*> Board;

class CChessCtrl;

class ChessBoard {

	/* Contains knowledge of:
	   board - situation of chess board at that instant
	   errorHander - an error handler to handle invalid submitted moves
	   piecePlaceholder - a Null Piece (EmptyPiece) for those who might need it
	   boolean flags - obvious in function by their names, right?
	*/
public:
	CChessCtrl* m_pParentWnd;
	CStatic* m_pColorStatic;
	CProgressCtrl* m_ctrlProgress;
	Board* _board;
	bool IsWhiteTurn() { return _isWhiteTurn;  }
	bool HasGameEnded() { return _hasEnded;  }
private:
	ChessErrHandler* errorHandler;
	Piece* piecePlaceholder;

	bool _isWhiteTurn = true;
	bool _isInCheck = false;
	bool _hasEnded = false;

public:
	ChessBoard();
	~ChessBoard();

	void resetBoard();
	void submitMove(const TCHAR* fromSquare, const TCHAR* toSquare);

private:
	/* Pre-defined setter of the boolean flags, internal use only
	   (Hiding object fields from being changed by public methods directly)
	*/
	void getANewBoard();
	void makeGameInCheck();
	void makeGameNotInCheck();
	void beginAGame();
	void endTheGame();
	void makeWhiteGoesNext();

	/* N.B.: To reduce cohesion between methods and the class, class field are
			 passed in as arguments of individual methods, aiming to allow methods
			 to function elsewhere when moved to other classes/ programs
	*/
	/* Pre-move checking methods:
	   Multiple layers of check if input are valid move on board
	*/
	bool gameCanContinue(wstring sourceFileRank, wstring destFileRank);
	bool sourceAndDestIsValid(wstring sourceFileRank, wstring destFileRank);
	bool withinChessBoard(wstring fileRank);
	bool sourceIsNotEmpty(wstring sourceFileRank, Board* board);
	bool isCurrentPlayerPiece(bool isWhiteTurn, Piece* piece, wstring sourceFileRank);
	bool pieceMoveIsValid(int returnCode, Piece* piece, wstring sourceFileRank, wstring destFileRank);
	bool pieceMoveKeepsKingSafe(bool isWhiteTurn, Piece* piece, wstring sourceFileRank, wstring destFileRank, Board* sandboxBoard);

	// Responsible in calling the handler to print out *helpful* error messages
	void handleInvalidMove(int returnCode, Piece* piece, wstring sourceFileRank, wstring destFileRank);

	/* Pre-move/ post-move (pure) checking methods:
	   Checks if the given side's king is safe from rivalry on given board,
		 and if the given side have valid move based on given board
	*/
	bool kingIsSafeFromRivalry(bool isWhiteTurn, Board* board);
	bool playerHaveValidMove(bool isWhiteTurn, Board* board);
	wstring findPlayersKingFileRank(bool isWhiteTurn, Board* board);

	/* In-move methods that make side-effect on chess board:
	   Move the piece on source file and rank on a given board as in how one does
		 in real life (Method differs in if it returns the captured piece or not)
	*/
	Piece* tryMoveAndReturnCaptured(wstring sourceFileRank, wstring destFileRank, Board* board);
	void confirmMoveOnBoard(wstring sourceFileRank, wstring destFileRank, Board* board);

	// Doesn't require much explanation for the following three methods right?
	void switchPlayers();

	Board* cloneBoard(Board* board);
	void deepCleanBoard(Board* board);

	// Printing methods, in both text and graphics, on stdout
	bool showMoveAndCheckIfGameCanContinue(Piece* piece, wstring sourceFileRank, Piece* capturedPiece, wstring destFileRank, bool isWhiteTurn, Board* board);

	void printMove(Piece* piece, wstring sourceFileRank, wstring destFileRank);
	void printCapture(Piece* capturedPiece);
	void printCheck();
	void printCheckmate(bool isWhitePlayer);
	void printStalemate();

	void printBoard(Board* board);

public:
	DWORD m_nComputerThreadID;
	HANDLE m_hComputerThread;
	void ComputerPlayer();
};

#endif
