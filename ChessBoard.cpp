// Bryan Liu (chl312), Dept. of Computing, Imperial College London
// ChessBoard.cpp - Implementation of ChessBoard (Info in ChessBoard.hpp)

#include "pch.h"
#include "ChessBoard.hpp"

using namespace std;

ChessBoard::ChessBoard()
{
	m_pParentWnd = nullptr;
	m_pColorStatic = nullptr;
	m_ctrlProgress = nullptr;
	_board = new Board();
	errorHandler = new ChessErrHandler();
	piecePlaceholder = new EmptyPiece(true);
	resetBoard();
}

ChessBoard::~ChessBoard()
{
	deepCleanBoard(_board);
	delete errorHandler;
	delete piecePlaceholder;
}

/* ChessBoard.submitMove():
   Verifies the move is valid, conduct the move and print on screen (stdout)
   Before any constructive checking:
	 - Is the game ended? (No point to continue if it is ended)
   Pre-move check:
	 - Is the given source and destination valid file and rank representations?
	 - Is the source an non-empty square?
	 - Is the piece on source square belongs to the player making the move?
	 - Is the piece's move on the current board valid based on its type?
   Try move: - In a sandbox to prevent messing up the original board
			 - Check: Does the move keeps current player's King in a safe
					  position (i.e. NOT in check)?
   Confirm the move on both the board and the piece
   Print the move on screen, and check if the game can continue
	 If yes, switch player and "wait" for next submitMove
	 If no, print statement and final represenation of board
   Finally do some nice memory management to prevent memory leakage
*/
void ChessBoard::submitMove(const TCHAR* fromSquare, const TCHAR* toSquare)
{
	wstring sourceFileRank(fromSquare);
	wstring destFileRank(toSquare);

	if (!gameCanContinue(sourceFileRank, destFileRank)) return;
	if (!sourceAndDestIsValid(sourceFileRank, destFileRank)) return;
	if (!sourceIsNotEmpty(sourceFileRank, _board)) return;

	Piece* piece = _board->at(sourceFileRank);
	if (!isCurrentPlayerPiece(_isWhiteTurn, piece, sourceFileRank)) return;

	int returnCode = piece->isValidMove(sourceFileRank, destFileRank, _board);
	if (!pieceMoveIsValid(returnCode, piece,
		sourceFileRank, destFileRank)) return;

	Board* sandboxBoard = cloneBoard(_board);
	Piece* capturedPiece = tryMoveAndReturnCaptured(sourceFileRank, destFileRank, sandboxBoard);
	if (!pieceMoveKeepsKingSafe(_isWhiteTurn, piece, sourceFileRank, destFileRank, sandboxBoard)) return;

	confirmMoveOnBoard(sourceFileRank, destFileRank, _board);

	if (!showMoveAndCheckIfGameCanContinue(piece, sourceFileRank,
		capturedPiece, destFileRank, _isWhiteTurn, _board))
	{
		endTheGame();
	}

	switchPlayers();

	delete capturedPiece;
	deepCleanBoard(sandboxBoard);
}

/* ChessBoard.resetBoard (): i.e. make a new game
   Rest the fields of the engine and insert appropriate pieces onto the board
*/
void ChessBoard::resetBoard()
{
	getANewBoard();
	beginAGame();
	makeWhiteGoesNext();
	makeGameNotInCheck();

	wstring fileRank(_T("A1"));
	for (TCHAR i = ChessInfo::MIN_FILE; i <= ChessInfo::MAX_FILE; i++)
	{
		fileRank.front() = i;

		fileRank.back() = ChessInfo::WHITE_INIT_PAWN_RANK;
		_board->insert({ fileRank, new Pawn(true) });

		fileRank.back() = ChessInfo::BLACK_INIT_PAWN_RANK;
		_board->insert({ fileRank, new Pawn(false) });
	}

	_board->insert({ wstring(_T("A1")), new Rook(true) });
	_board->insert({ wstring(_T("B1")), new Knight(true) });
	_board->insert({ wstring(_T("C1")), new Bishop(true) });
	_board->insert({ wstring(_T("D1")), new Queen(true) });
	_board->insert({ wstring(_T("E1")), new King(true) });
	_board->insert({ wstring(_T("F1")), new Bishop(true) });
	_board->insert({ wstring(_T("G1")), new Knight(true) });
	_board->insert({ wstring(_T("H1")), new Rook(true) });

	_board->insert({ wstring(_T("A8")), new Rook(false) });
	_board->insert({ wstring(_T("B8")), new Knight(false) });
	_board->insert({ wstring(_T("C8")), new Bishop(false) });
	_board->insert({ wstring(_T("D8")), new Queen(false) });
	_board->insert({ wstring(_T("E8")), new King(false) });
	_board->insert({ wstring(_T("F8")), new Bishop(false) });
	_board->insert({ wstring(_T("G8")), new Knight(false) });
	_board->insert({ wstring(_T("H8")), new Rook(false) });

	cout << "Let the game begin..." << endl;
	if (m_pColorStatic != nullptr)
	{
		// m_pColorStatic->SetBackgroundColor(RGB(0xEE, 0xE8, 0xAA));
		m_pColorStatic->SetWindowText(_T("“Every chess master was once a beginner.” – Irving Chernev"));
	}
}

// The pre-defined setters (6 methods)
/* getANewBoard () assumption:
   board contains non-null pointer to a Board, thus can be deep cleaned
*/
void ChessBoard::getANewBoard()
{
	deepCleanBoard(this->_board);
	_board = new Board;
}

void ChessBoard::makeGameInCheck()
{
	_isInCheck = true;
}

void ChessBoard::makeGameNotInCheck()
{
	_isInCheck = false;
}

void ChessBoard::beginAGame()
{
	_hasEnded = false;
}

void ChessBoard::endTheGame()
{
	_hasEnded = true;
}

void ChessBoard::makeWhiteGoesNext()
{
	_isWhiteTurn = true;
}

/* The following methods check certain conditions required for a valid chess
	 move and gives out an error to user (and return false) if it is not met
   N.B: A kind request not to judge on "if condition then false else true"
		It is, at the end, an over-literal program illustration
*/

// gameCanContine (): check if the game is not ended
bool ChessBoard::gameCanContinue(wstring sourceFileRank, wstring destFileRank)
{
	if (_hasEnded)
	{
		handleInvalidMove(ChessErrHandler::GAME_HAS_ENDED,
			piecePlaceholder, sourceFileRank, destFileRank);
		return false;
	}
	return true;
}

/* sourceAndDestIsValid (): check if given source and dest's file and rank
							represenation is valid (wstring with 2 chars)
							and are within chess board
*/
bool ChessBoard::sourceAndDestIsValid(wstring sourceFileRank, wstring destFileRank)
{
	if (sourceFileRank.length() != ChessInfo::FILERANK_LENGTH ||
		destFileRank.length() != ChessInfo::FILERANK_LENGTH)
	{
		handleInvalidMove(ChessErrHandler::INVALID_FILE_RANK,
			piecePlaceholder, sourceFileRank, destFileRank);
		return false;
	}

	if (!withinChessBoard(sourceFileRank))
	{
		handleInvalidMove(ChessErrHandler::SOURCE_OUTOF_BOUND,
			piecePlaceholder, sourceFileRank, destFileRank);
		return false;
	}

	if (!withinChessBoard(destFileRank))
	{
		handleInvalidMove(ChessErrHandler::DEST_OUTOF_BOUND,
			piecePlaceholder, sourceFileRank, destFileRank);
		return false;
	}
	return true;
}

/* withinChesBoard (): helper method
   check if file is in range 'A'-'H', rank is in range '1'-'8'
*/
bool ChessBoard::withinChessBoard(wstring fileRank)
{
	TCHAR file = fileRank.at(ChessInfo::FILE_INDEX);
	TCHAR rank = fileRank.at(ChessInfo::RANK_INDEX);
	return ChessInfo::MIN_FILE <= file && file <= ChessInfo::MAX_FILE &&
		ChessInfo::MIN_RANK <= rank && rank <= ChessInfo::MAX_RANK;
}

/* sourceIsNotEmpty ():
   check if source is not an empty square (i.e. there is a piece to move)
*/
bool ChessBoard::sourceIsNotEmpty(wstring sourceFileRank, Board* board)
{
	try
	{
		board->at(sourceFileRank);
	}
	catch (const std::out_of_range& err)
	{
		UNREFERENCED_PARAMETER(err);
		handleInvalidMove(ChessErrHandler::MOVED_EMPTY_PIECE,
			piecePlaceholder, sourceFileRank, sourceFileRank);
		return false;
	}
	return true;
}

/* isCurrentPlayerPiece ():
   check if the piece at source belongs to the player in turn
*/
bool ChessBoard::isCurrentPlayerPiece(bool isWhiteTurn, Piece* piece, wstring sourceFileRank)
{

	bool isCurrentPlayerPiece = isWhiteTurn == piece->isWhitePlayer();
	if (!isCurrentPlayerPiece)
	{
		handleInvalidMove(ChessErrHandler::NOT_OWNER_TURN, piece,
			sourceFileRank, sourceFileRank);
	}
	return isCurrentPlayerPiece;
}

/* pieceMoveIsValid ():
   check if the return code for validating piece move is zero
*/
bool ChessBoard::pieceMoveIsValid(int returnCode, Piece* piece, wstring sourceFileRank, wstring destFileRank)
{

	if (returnCode != ChessErrHandler::CHESS_NO_ERROR)
	{
		handleInvalidMove(returnCode, piece, sourceFileRank, destFileRank);
		return false;
	}
	return true;
}

/* pieceMoveKeepsKingSafe ():
   check if given side's King is safe under the given board
   (in ChessBoard it would be a sandbox),
   if not give out an error to user and return false
*/
bool ChessBoard::pieceMoveKeepsKingSafe(bool isWhiteTurn, Piece* piece, wstring sourceFileRank, wstring destFileRank, Board* board)
{

	if (!kingIsSafeFromRivalry(isWhiteTurn, board))
	{
		handleInvalidMove(ChessErrHandler::ALLOW_KING_IN_CHECK, piece,
			sourceFileRank, destFileRank);
		return false;
	}
	return true;
}

/* handleInvalidMove ():
   Calls the error handler to print the error,
   passing the information given by the calling functions
*/
void ChessBoard::handleInvalidMove
(int errorCode, Piece* piece, wstring sourceFileRank, wstring destFileRank)
{
	errorHandler->printErr(errorCode, piece, sourceFileRank, destFileRank);
}

/* kingIsSafeFromRivalry ():
   pre-cond.: board valid, existing representation of a Board
   Return true if none of the Pieces of the other side can capture the given
	 side's King (i.e. has a valid move to the King's file and rank)
   Method: Check all pieces of the other side
		   For each piece check if its move to the given side's King is valid
		   (if true, the given side's King is not safe)
*/
bool ChessBoard::kingIsSafeFromRivalry(bool isWhiteTurn, Board* board)
{
	wstring kingFileRank = findPlayersKingFileRank(isWhiteTurn, board);

	for (Board::const_iterator it = board->cbegin();
		it != board->cend(); it++) {
		wstring challengingFileRank = it->first;
		Piece* challengingPiece = it->second;
		if (challengingPiece->isWhitePlayer() != isWhiteTurn &&
			challengingPiece->isValidMove(challengingFileRank,
				kingFileRank, board) == ChessErrHandler::CHESS_NO_ERROR)
		{
			return false;
		}
	}
	return true;
}

/* playerHaveValidMove ():
   pre-cond.: board valid, existing representation of a Board
   Return true if the given side have a valid move on the given board
   Method: Check all pieces of the given side
		   For each piece check all its valid move
		   For each valid move check if given side's King is safe from attack
		   (if true, there is at least one valid move for the given side)
*/
bool ChessBoard::playerHaveValidMove(bool isWhiteTurn, Board* board)
{
	for (Board::const_iterator it = board->cbegin(); it != board->cend(); ++it)
	{
		wstring possibleSource = it->first;
		Piece* possiblePiece = it->second;

		if (possiblePiece->isWhitePlayer() == isWhiteTurn)
		{
			for (TCHAR i = ChessInfo::MIN_FILE; i <= ChessInfo::MAX_FILE; i++)
			{
				for (TCHAR j = ChessInfo::MIN_RANK; j <= ChessInfo::MAX_RANK; j++)
				{

					wstring possibleDest({ i, j });
					if (possiblePiece->isValidMove(possibleSource,
						possibleDest, board) == ChessErrHandler::CHESS_NO_ERROR)
					{
						Board* sandboxBoard = cloneBoard(board);
						Piece* captured = tryMoveAndReturnCaptured(possibleSource, possibleDest, sandboxBoard);
						bool haveValidMove = kingIsSafeFromRivalry(isWhiteTurn, sandboxBoard);

						delete captured;
						deepCleanBoard(sandboxBoard);

						if (haveValidMove) return true;
					}
				}
			}
		}
	}
	return false;
}

/* findPlayersKingFileRank ()
   pre-cond.: board valid, existing representation of a Board
   Iterate and return the location (file & rank) of the given side's King
*/
wstring ChessBoard::findPlayersKingFileRank(bool isWhiteTurn, Board* board)
{
	for (Board::const_iterator it = board->cbegin(); it != board->cend(); it++)
	{
		wstring possibleFileRank = it->first;
		Piece* possiblePiece = it->second;
		if (possiblePiece->isWhitePlayer() == isWhiteTurn &&
			possiblePiece->isKing() == true)
		{
			return possibleFileRank;
		}
	}
	return _T("");
}

/* tryMoveAndReturnCaptured ():
   pre-cond.: sourceFileRank, destFileRank valid file & rank represenation
			  board valid, existing rep. of a Board (expt. to be a sandbox)
   Given a board, conduct move from source to destination & get the captured
   Case 1 (try block) - Another piece in destination (conduct capture)
   Case 2 (catch block) - No piece exists in destination (conduct move)
   Return the piece if in case 1, NULL otherwise
*/
Piece* ChessBoard::tryMoveAndReturnCaptured(wstring sourceFileRank, wstring destFileRank, Board* board)
{

	Piece* sourcePiece = board->at(sourceFileRank);
	Piece* destPiece;

	try
	{
		destPiece = board->at(destFileRank);
		board->at(destFileRank) = sourcePiece;
	}
	catch (const std::out_of_range& err)
	{
		UNREFERENCED_PARAMETER(err);
		destPiece = NULL;
		board->insert({ destFileRank, sourcePiece });
	}
	board->erase(sourceFileRank);

	return destPiece;
}

/* confirmMoveOnBoard ():
   pre-cond.: sourceFileRank, destFileRank valid file & rank represenation
			  board valid, existing rep. of a Board (expected to be a real one)
   Given a board, conduct the move from source to destination
   Case 1 (try block) - Another piece in destination (conduct capture)
   Case 2 (catch block) - No piece exists in destination (conduct move)
*/
void ChessBoard::confirmMoveOnBoard(wstring sourceFileRank, wstring destFileRank, Board* board)
{
	Piece* movingPiece = board->at(sourceFileRank);
	try
	{
		Piece* destPiece = board->at(destFileRank);
		delete destPiece;
		board->at(destFileRank) = movingPiece;
	}
	catch (const std::out_of_range& err) {
		UNREFERENCED_PARAMETER(err);
		board->insert({ destFileRank, movingPiece });
	}

	movingPiece->confirmMove();
	board->erase(sourceFileRank);
	makeGameNotInCheck();
}

/* Valid move report statement structure:
   A B? C?
   ((D|E)
   F)?

   A - "(name of piece) moves from (source) to (destination)"
   B - "taking (name of captured piece)" (if there is a captured piece)
   C - ", Check!" (if the other side is now in check)
   D - "Stalemate." (if other side have no valid move while not in check)
   E - "Checkmate! (loser side) loses." (ditto, but while in check)
   F - "(unicode visualisation of chess board)"

   Also return if the game can continue or not (i.e. no checkmate/stalemate)
*/
bool ChessBoard::showMoveAndCheckIfGameCanContinue(Piece* piece,
	wstring sourceFileRank, Piece* capturedPiece, wstring destFileRank,
	bool isWhiteTurn, Board* board)
{
	printMove(piece, sourceFileRank, destFileRank);
	if (capturedPiece != NULL) {
		printCapture(capturedPiece);
	}
	if (!kingIsSafeFromRivalry(!isWhiteTurn, board)) {
		printCheck();
		makeGameInCheck();
	}
	cout << endl;

	if (!playerHaveValidMove(!isWhiteTurn, board)) {
		if (_isInCheck) {
			printCheckmate(!isWhiteTurn);
		}
		else {
			printStalemate();
		}
		printBoard(board);
		return false;
	}
	return true;
}

// Have a guess what it does :)
void ChessBoard::switchPlayers()
{
	_isWhiteTurn = !(_isWhiteTurn);
}

// Deep clone the given board - including all the Piece within the map
Board* ChessBoard::cloneBoard(Board* board)
{

	Board* clone = new Board;
	for (Board::iterator it = board->begin(); it != board->end(); it++)
	{
		clone->insert({ it->first, it->second->clone() });
	}
	return clone;
}

/* deepCleanBoard ():
   deep clean the given board, free all containing Piece's memory and itself
*/
void ChessBoard::deepCleanBoard(Board* board)
{

	for (Board::iterator it = board->begin(); it != board->end(); it++)
	{
		delete it->second;
	}
	delete board;
}

/* print(Move|Capture|Check|Checkmate|Stalemate) ():
   Print in stdout with structured as specified in comments for
	 showMoveAndCheckIfGameCanContinue ()
*/
void ChessBoard::printMove(Piece* movingPiece, wstring sourceFileRank, wstring destFileRank)
{

	/* cout << movingPiece->toString() << " moves from "
		<< sourceFileRank << " to " << destFileRank; */
	wstring strStatus = movingPiece->toString();
	strStatus += _T(" moves from ");
	strStatus += sourceFileRank;
	strStatus += _T(" to ");
	strStatus += destFileRank;
	if (m_pColorStatic != nullptr)
	{
		m_pColorStatic->SetWindowText(strStatus.c_str());
	}
}

void ChessBoard::printCapture(Piece* capturedPiece)
{
	/*cout << " taking " << capturedPiece->toString(); */
	wstring strStatus = _T(" taking ");
	strStatus += capturedPiece->toString();
	if (m_pColorStatic != nullptr)
	{
		CString strBuffer;
		m_pColorStatic->GetWindowText(strBuffer);
		strBuffer += strStatus.c_str();
		m_pColorStatic->SetWindowText(strBuffer);
	}
}

void ChessBoard::printCheck()
{
	/* cout << ", Check!"; */
	wstring strStatus = _T(", Check!");
	if (m_pColorStatic != nullptr)
	{
		CString strBuffer;
		m_pColorStatic->GetWindowText(strBuffer);
		strBuffer += strStatus.c_str();
		m_pColorStatic->SetWindowText(strBuffer);
	}
	MessageBeep(MB_ICONEXCLAMATION);
}

void ChessBoard::printCheckmate(bool isWhiteTurn)
{
	/* cout << "Checkmate! " << (isWhiteTurn ? "White" : "Black")
		<< " loses." << endl; */
	wstring strStatus = _T(" Checkmate! ");
	strStatus += (isWhiteTurn ? _T("White") : _T("Black"));
	strStatus += _T(" loses.");
	if (m_pColorStatic != nullptr)
	{
		CString strBuffer;
		m_pColorStatic->GetWindowText(strBuffer);
		strBuffer += strStatus.c_str();
		m_pColorStatic->SetWindowText(strBuffer);
	}
	MessageBeep(MB_ICONEXCLAMATION);
}

void ChessBoard::printStalemate()
{
	/* cout << "Stalemate." << endl; */
	wstring strStatus = _T(" Stalemate.");
	if (m_pColorStatic != nullptr)
	{
		CString strBuffer;
		m_pColorStatic->GetWindowText(strBuffer);
		strBuffer += strStatus.c_str();
		m_pColorStatic->SetWindowText(strBuffer);
	}
	MessageBeep(MB_ICONEXCLAMATION);
}

// Printing the state of the chessboard in unicode (graphic) representation
void ChessBoard::printBoard(Board* board)
{
	UNREFERENCED_PARAMETER(board);
	/* cout << endl << "  +--+--+--+--+--+--+--+--+" << endl;
	for (TCHAR i = ChessInfo::MAX_RANK; i >= ChessInfo::MIN_RANK; i--)
	{
		cout << i << " ";
		for (TCHAR j = ChessInfo::MIN_FILE; j <= ChessInfo::MAX_FILE; j++)
		{
			cout << "|";
			wstring fileRank({ j, i });
			try
			{
				Piece* piece = board->at(fileRank);
				cout << piece->toGraphics() << " ";
			}
			catch (const std::out_of_range& err)
			{
				UNREFERENCED_PARAMETER(err);
				cout << "  ";
			}
		}
		cout << "|" << endl << "  +--+--+--+--+--+--+--+--+" << endl;
	}
	cout << "    A  B  C  D  E  F  G  H" << endl; */
}
