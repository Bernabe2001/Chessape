#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <limits>
#include <utility>
#include <chrono>
#include <iomanip>
#include <cassert>
#include "../../chess-library/include/chess.hpp"

using namespace chess;
using namespace std;
using Clock = std::chrono::steady_clock;

constexpr int INFINITY_VAL = numeric_limits<int>::max();
constexpr int HASH_TABLE_SIZE = 1000000;

#ifndef DEPTH
#define DEPTH 6
#endif

#ifndef ALPHA_BETA
#define ALPHA_BETA 1
#endif

#ifndef INCREMENTAL_EV
#define INCREMENTAL_EV 1
#endif

// Debug macro: prints only when compiled with -DDEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x) (std::cerr << x << std::endl)
#else
  #define DEBUG_PRINT(x) ((void)0)
#endif

//-------------------------------------------------------------
// Evaluation namespace with piece values and piece–square tables
//-------------------------------------------------------------
namespace Evaluation {

    constexpr int PST[2][6][64] = {
        { // White tables
            {   // Pawn table × 1
                100, 100, 100, 100, 100, 100, 100, 100,
                100, 100, 100, 100, 100, 100, 100, 100,
                105, 100, 100, 105, 105, 95 , 95 , 105,
                105, 105, 110, 130, 130, 100, 95 , 105,
                110, 110, 120, 140, 140, 120, 110, 110,
                130, 130, 140, 150, 150, 140, 130, 130,
                170, 180, 190, 200, 200, 190, 180, 170,
                100, 100, 100, 100, 100, 100, 100, 100
            },
            {   // Knight table × 3
                230, 260, 280, 280, 280, 280, 260, 230,
                260, 290, 310, 315, 315, 310, 290, 260,
                280, 310, 330, 340, 340, 330, 310, 280,
                280, 315, 340, 350, 350, 340, 315, 280,
                280, 315, 340, 350, 350, 340, 315, 280,
                280, 310, 330, 340, 340, 330, 310, 280,
                260, 290, 310, 315, 315, 310, 290, 260,
                230, 260, 280, 280, 280, 280, 260, 230
            },
            {   // Bishop table × 3
                260, 280, 300, 290, 290, 300, 280, 260,
                280, 330, 300, 315, 315, 300, 330, 280,
                300, 300, 330, 330, 330, 330, 300, 300,
                290, 315, 330, 330, 330, 330, 315, 290,
                290, 315, 330, 330, 330, 330, 315, 290,
                300, 300, 330, 330, 330, 330, 300, 300,
                280, 330, 300, 315, 315, 300, 330, 280,
                260, 280, 300, 290, 290, 300, 280, 260
            },
            {   // Rook table × 5
                450, 450, 500, 525, 525, 500, 450, 450,
                450, 450, 500, 540, 540, 500, 450, 450,
                500, 500, 500, 540, 540, 500, 500, 500,
                500, 500, 500, 500, 500, 500, 500, 500,
                500, 500, 500, 500, 500, 500, 500, 500,
                500, 500, 500, 500, 500, 500, 500, 500,
                500, 500, 500, 570, 570, 500, 500, 500,
                500, 500, 500, 550, 550, 500, 500, 500
            },
            {   // Queen table × 9
                810, 840, 870, 900, 900, 870, 840, 810,
                840, 900, 900, 900, 900, 900, 900, 840,
                870, 900, 920, 920, 920, 920, 900, 870,
                900, 900, 920, 940, 940, 920, 900, 900,
                900, 900, 920, 940, 940, 920, 900, 900,
                870, 900, 920, 920, 920, 920, 900, 870,
                840, 900, 900, 900, 900, 900, 900, 840,
                810, 840, 870, 900, 900, 870, 840, 810
            },
            {   // King table × 0
                0,50,25,0,0,25,50,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0
            }
        },
        { // Black tables
            {   // Pawn table × 1
                100, 100, 100, 100, 100, 100, 100, 100,
                170, 180, 190, 200, 200, 190, 180, 170,
                130, 130, 140, 150, 150, 140, 130, 130,
                110, 110, 120, 140, 140, 120, 110, 110,
                105, 105, 110, 130, 130, 100, 95 , 105,
                105, 100, 100, 105, 105, 95 , 95 , 105,
                100, 100, 100, 100, 100, 100, 100, 100, 
                100, 100, 100, 100, 100, 100, 100, 100
            },
            {   // Knight table × 3
                230, 260, 280, 280, 280, 280, 260, 230,
                260, 290, 310, 315, 315, 310, 290, 260,
                280, 310, 330, 340, 340, 330, 310, 280,
                280, 315, 340, 350, 350, 340, 315, 280,
                280, 315, 340, 350, 350, 340, 315, 280,
                280, 310, 330, 340, 340, 330, 310, 280,
                260, 290, 310, 315, 315, 310, 290, 260,
                230, 260, 280, 280, 280, 280, 260, 230
            },
            {   // Bishop table × 3
                260, 280, 300, 290, 290, 300, 280, 260,
                280, 330, 300, 315, 315, 300, 330, 280,
                300, 300, 330, 330, 330, 330, 300, 300,
                290, 315, 330, 330, 330, 330, 315, 290,
                290, 315, 330, 330, 330, 330, 315, 290,
                300, 300, 330, 330, 330, 330, 300, 300,
                280, 330, 300, 315, 315, 300, 330, 280,
                260, 280, 300, 290, 290, 300, 280, 260
            },
            {   // Rook table × 5
                500, 500, 500, 550, 550, 500, 500, 500,
                500, 500, 500, 570, 570, 500, 500, 500,
                500, 500, 500, 500, 500, 500, 500, 500,
                500, 500, 500, 500, 500, 500, 500, 500,
                500, 500, 500, 500, 500, 500, 500, 500,
                500, 500, 500, 540, 540, 500, 500, 500,
                450, 450, 500, 540, 540, 500, 450, 450,
                450, 450, 500, 525, 525, 500, 450, 450
            },
            {   // Queen table × 9
                810, 840, 870, 900, 900, 870, 840, 810,
                840, 900, 900, 900, 900, 900, 900, 840,
                870, 900, 920, 920, 920, 920, 900, 870,
                900, 900, 920, 940, 940, 920, 900, 900,
                900, 900, 920, 940, 940, 920, 900, 900,
                870, 900, 920, 920, 920, 920, 900, 870,
                840, 900, 900, 900, 900, 900, 900, 840,
                810, 840, 870, 900, 900, 870, 840, 810
            },
            {   // King table × 0
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,50,25,0,0,25,50,0
            }
        }
    };
}

//-------------------------------------------------------------
// getPieceSquareValue: Returns the PST value for a given piece.
//-------------------------------------------------------------
inline int getPieceSquareValue(PieceType type, int colorIdx, int square) {
    int pieceIndex = static_cast<int>(type); // Requires PieceType order matches PST
    return Evaluation::PST[colorIdx][pieceIndex][square];
}

//-------------------------------------------------------------
// Evaluation Function: Computes a static evaluation of the board.
//-------------------------------------------------------------
int evaluateBoard(Board &board) {
    int score = 0;
    // Loop over both colors.
    for (int colorIdx : {0,1}) {
        int multiplier = (colorIdx == 0) ? 1 : -1;
        // Evaluate Pawn, Knight, Bishop, Rook, and Queen.
        for (auto type : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                           PieceType::ROOK, PieceType::QUEEN, PieceType::KING}) {
            Bitboard pieces = board.pieces(type, Color(colorIdx));
            while (!pieces.empty()){
                int sq = pieces.pop();
                score += multiplier * getPieceSquareValue(type, colorIdx, sq);
            }
        }
    }
    return score;
}


int evaluateMove(const Board& board, const Move& move) {
    int delta = 0;
    int movingColorIdx = board.sideToMove();
    int colorSign = (movingColorIdx == 0 ? 1 : -1);
    int oppColorIdx = (movingColorIdx == 0 ? 1 : 0);
    uint16_t type = move.typeOf();
    int from_ind = move.from().index();
    int to_ind = move.to().index();

    if (type == Move::CASTLING) delta += colorSign * 100;
    else if (type == Move::ENPASSANT) {
        int pawnValue = getPieceSquareValue(PieceType::PAWN, movingColorIdx, from_ind);
        delta -= colorSign * pawnValue;  // Remove our pawn from source

        int capturedSq_ind = (movingColorIdx == 0 ? to_ind - 8 : to_ind + 8);
        
        int capturedPawnValue = getPieceSquareValue(PieceType::PAWN, oppColorIdx, capturedSq_ind);
        delta += colorSign * capturedPawnValue;  // Remove opponent's pawn (negative becomes positive)

        int newSquareValue = getPieceSquareValue(PieceType::PAWN, movingColorIdx, to_ind);
        delta += colorSign * newSquareValue;  // Add our pawn to destination
    } 
    else if (type == Move::PROMOTION){
        PieceType capture_type = board.at(Square(to_ind)).type();

        delta -= colorSign * getPieceSquareValue(PieceType::PAWN, movingColorIdx, from_ind);
        delta += colorSign * getPieceSquareValue(move.promotionType(), movingColorIdx, to_ind);
        
        if (capture_type != PieceType::NONE){
            delta += colorSign * getPieceSquareValue(capture_type, oppColorIdx, to_ind);
        }
    }
    else { 
        PieceType pieceType = board.at(Square(from_ind)).type();

        delta -= colorSign * getPieceSquareValue(pieceType, movingColorIdx, from_ind);
        delta += colorSign * getPieceSquareValue(pieceType, movingColorIdx, to_ind);

        PieceType capture_type = board.at(Square(to_ind)).type();
        if (capture_type != PieceType::NONE){
            delta += colorSign * getPieceSquareValue(capture_type, oppColorIdx, to_ind);
        }
    }
    return delta;
}


int black(Board &board, int depth, int alpha, int beta, Move &bestMove, int &currentEval, vector<uint8_t> &positionCounts);
int white(Board &board, int depth, int alpha, int beta, Move &bestMove, int &currentEval, vector<uint8_t> &positionCounts) {
    // Terminal condition 1
    if (board.isHalfMoveDraw()) {
        GameResult result = board.getHalfMoveDrawType().second;
        if (result == GameResult::LOSE) return -INFINITY_VAL;
        else return 0;
    }

    // Terminal condition 2
    uint64_t zobrist_w = board.zobrist() % HASH_TABLE_SIZE;
    uint8_t rep = positionCounts[zobrist_w];
    if (rep == 2) {
        return 0;
    }

    Movelist moves;
    movegen::legalmoves(moves, board);
    // Terminal condition 3
    if (moves.empty()) {
        GameResult result = board.isGameOver().second;
        if (result == GameResult::LOSE) return -INFINITY_VAL;
        else if (result == GameResult::DRAW) return 0;
        else return INFINITY_VAL;
    }

    // Terminal condition 4
    if (depth == 0) {
        #if INCREMENTAL_EV == 1
            return currentEval;
        #else
            return evaluateBoard(board);
        #endif
    }

    Move dummy;
    int best = -INFINITY_VAL;
    bestMove = moves[0];
    for (auto &move : moves) {
        int evalDelta = 0;
        #if INCREMENTAL_EV == 1
            // Calculate the change in evaluation caused by the move.
            evalDelta = evaluateMove(board, move);
            currentEval += evalDelta;
        #endif
        board.makeMove(move);
        positionCounts[zobrist_w] += 1;
        int score = black(board, depth - 1, alpha, beta, dummy, currentEval, positionCounts);
        positionCounts[zobrist_w] -= 1;
        board.unmakeMove(move);
        #if INCREMENTAL_EV == 1
            // Restore the evaluation to its previous state.
            currentEval -= evalDelta;
        #endif
        if (score > best) {
            best = score;
            bestMove = move;
        }
        alpha = max(alpha, score);
        #if ALPHA_BETA == 1
            if (alpha >= beta) {
                break;
            }
        #endif
    }
    return best;
}



int black(Board &board, int depth, int alpha, int beta, Move &bestMove, int &currentEval, vector<uint8_t> &positionCounts) {
    // Terminal condition 1 (Missing Threefold Rep)
    if (board.isHalfMoveDraw()) {
        GameResult result = board.getHalfMoveDrawType().second;
        if (result == GameResult::LOSE) return INFINITY_VAL;
        else return 0;
    }

    // Terminal condition 2
    uint64_t zobrist_b = board.zobrist() % HASH_TABLE_SIZE;
    uint8_t rep = positionCounts[zobrist_b];
    if (rep == 2) return 0;

    Movelist moves;
    movegen::legalmoves(moves, board);
    // Terminal condition 3
    if (moves.empty()) {
        GameResult result = board.isGameOver().second;
        if (result == GameResult::LOSE) return INFINITY_VAL;
        else if (result == GameResult::DRAW) return 0;
        else return -INFINITY_VAL;
    }

    // Terminal condition 4
    if (depth == 0) {
        #if INCREMENTAL_EV == 1
            return currentEval;
        #else
            return evaluateBoard(board);
        #endif
    }

    Move dummy;
    int best = INFINITY_VAL;
    bestMove = moves[0];
    for (auto &move : moves) {
        int evalDelta = 0;
        #if INCREMENTAL_EV == 1
            // Calculate the change in evaluation caused by the move.
            evalDelta = evaluateMove(board, move);
            currentEval += evalDelta;
        #endif
        board.makeMove(move);
        positionCounts[zobrist_b] += 1;
        int score = white(board, depth - 1, alpha, beta, dummy, currentEval, positionCounts);
        positionCounts[zobrist_b] -= 1;
        board.unmakeMove(move);
        #if INCREMENTAL_EV == 1
            // Restore the evaluation to its previous state.
            currentEval -= evalDelta;
        #endif
        if (score < best) {
            best = score;
            bestMove = move;
        }
        beta = min(beta, score);
        #if ALPHA_BETA == 1
            if (alpha >= beta) {
                break;
            }
        #endif
    }
    return best;
}

//-------------------------------------------------------------
// UCI Command Handler: Processes UCI protocol commands.
//-------------------------------------------------------------
void handleUCI() {
    Board board; // Our current board state.
    string command;
    vector<uint8_t> positionCounts(HASH_TABLE_SIZE,0);
    while (getline(cin, command)) {
        if (command == "uci") {
            cout << "id name Chessape_1.0" << endl;
            cout << "id author Bernabé Iturralde Jara" << endl;
            cout << "uciok" << endl;
        } else if (command == "isready") {
            cout << "readyok" << endl;
        } else if (command.rfind("position", 0) == 0) {
            istringstream ss(command);
            string token;
            ss >> token; // "position"
            ss >> token; // "startpos" or "fen"
            if (token == "startpos") {
                board = Board();
                DEBUG_PRINT("[DEBUG] Position set to startpos.");
            } else if (token == "fen") {
                string fen_str, fen_token;
                for (int i = 0; i < 6; i++) {
                    if (!(ss >> fen_token)) {
                        DEBUG_PRINT("[DEBUG] Error: incomplete FEN string.");
                        break;
                    }
                    if (i > 0) fen_str += " ";
                    fen_str += fen_token;
                }
                board = Board(fen_str);
                DEBUG_PRINT("[DEBUG] Position set to FEN: " + fen_str);
            }
            // Process additional moves (if any).
            while (ss >> token) {
                if (token == "moves") continue;
                DEBUG_PRINT("[DEBUG] Applying move: " + token);
                Move move = uci::uciToMove(board, token);
                board.makeMove(move);
            }
            DEBUG_PRINT("[DEBUG] Final FEN: " + board.getFen());
            DEBUG_PRINT("[DEBUG] sideToMove: " + string((board.sideToMove() == Color::WHITE) ? "WHITE" : "BLACK"));
        } else if (command.rfind("go", 0) == 0) {
            DEBUG_PRINT("[DEBUG] Starting search on FEN: " + board.getFen());
            
            Move bestMove;
            int currentEval = evaluateBoard(board);
            int score;
            int zobrist = board.zobrist() % HASH_TABLE_SIZE;
            if (board.sideToMove() == Color(0)){
                score = white(board, DEPTH, -INFINITY_VAL, INFINITY_VAL, bestMove, currentEval, positionCounts);
            }
            else score = black(board, DEPTH, -INFINITY_VAL, INFINITY_VAL, bestMove, currentEval, positionCounts);
            positionCounts[zobrist] += 1;
            DEBUG_PRINT("[DEBUG] Best score: " + to_string(score) + ", Best move: " + uci::moveToUci(bestMove));
            cout << "bestmove " << uci::moveToUci(bestMove) << endl;
        } else if (command == "quit") {
            break;
        }
        else if (command == "ucinewgame") {
            positionCounts = vector<uint8_t> (HASH_TABLE_SIZE,0);
        }
    }
}






#ifdef TEST

void test_evaluateBoard() {
    std::cout << "Testing evaluateBoard..." << std::endl;
    Board board;
    int score;

    // Test 1: Empty board
    board = Board("8/8/8/8/8/8/8/8 w - - 0 1");
    score = evaluateBoard(board);
    assert(score == 0);

    // Test 2: Single white pawn on a2
    board = Board("8/8/8/8/8/8/P7/8 w - - 0 1");
    score = evaluateBoard(board);
    assert(score == 100);

    // Test 3: Single black pawn on a7
    board = Board("8/p7/8/8/8/8/8/8 w - - 0 1");
    score = evaluateBoard(board);
    assert(score == -100);

    // Test 4: White knight on b1
    board = Board("8/8/8/8/8/8/8/N7 w - - 0 1");
    score = evaluateBoard(board);
    assert(score == 230);

    // Test 5: Black bishop on c8
    board = Board("2b5/8/8/8/8/8/8/8 w - - 0 1");
    score = evaluateBoard(board);
    assert(score == -300);

    // Test 6: White queen on d1
    board = Board("8/8/8/8/8/8/8/3Q4 w - - 0 1");
    score = evaluateBoard(board);
    assert(score == 900);

    // Test 7: Black rook on h8
    board = Board("7r/8/8/8/8/8/8/8 w - - 0 1");
    score = evaluateBoard(board);
    assert(score == -450);

    // Test 8: White pawn on e4
    board = Board("8/8/8/8/4P3/8/8/8 w - - 0 1");
    score = evaluateBoard(board);
    assert(score == 120);

    // Test 9: Black knight on g8
    board = Board("6n1/8/8/8/8/8/8/8 w - - 0 1");
    score = evaluateBoard(board);
    assert(score == -260);

    // Test 10: Initial position
    board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    score = evaluateBoard(board);
    assert(score == 0);

    std::cout << "All evaluateBoard tests passed!" << std::endl;
}

void test_evaluateMove() {
    std::cout << "Testing evaluateMove..." << std::endl;
    Board board;
    Move move;
    int delta;

    // Test 1: Black pawn e2 to e4
    board = Board("8/8/8/8/8/8/4P3/8 w - - 0 1");
    move = uci::uciToMove(board, "e2e4");
    delta = evaluateMove(board, move);
    assert(delta == 20);

    // Test 2: White captures black pawn
    board = Board("8/8/8/3p4/4P3/8/8/8 w - - 0 1");
    move = uci::uciToMove(board, "e4d5");
    delta = evaluateMove(board, move);
    assert(delta == 130);

    // Test 3: Black pawn e7 to e5
    board = Board("8/4P3/8/8/8/8/8/8 b - - 0 1");
    move = uci::uciToMove(board, "e7e5");
    delta = evaluateMove(board, move);
    assert(delta == -20);

    // Test 4: Knight move
    board = Board("8/8/8/8/8/8/8/N7 w - - 0 1");
    move = uci::uciToMove(board, "a1b3");
    delta = evaluateMove(board, move);
    assert(delta == 80);

    // Test 5: En passant
    board = Board("7k/8/8/pP6/8/8/8/7K w - a6 0 1");
    move = Move::make<Move::ENPASSANT>(Square(33), Square(40));
    delta = evaluateMove(board, move);
    assert(delta == 125);

    // Test 6: Castling
    board = Board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    move = uci::uciToMove(board, "e1g1");
    delta = evaluateMove(board, move);
    assert(delta == 100);

    // Test 7: Promotion
    board = Board("8/P7/8/8/8/8/8/8 w - - 0 1");
    move = uci::uciToMove(board, "a7a8q");
    delta = evaluateMove(board, move);
    assert(delta == 640);

    // Test 8: Black promotion with capture
    board = Board("8/8/8/8/8/8/6p1/7B b - - 0 1");
    move = uci::uciToMove(board, "g2h1q");
    delta = evaluateMove(board, move);
    assert(delta == -890);

    // Test 9: King move (no PST change)
    board = Board("8/8/8/8/8/8/8/4K3 w - - 0 1");
    move = uci::uciToMove(board, "e1e2");
    delta = evaluateMove(board, move);
    assert(delta == 0);

    // Test 10: Checkmate move evaluation
    board = Board("3k4/3Q4/3K4/8/8/8/8/8 w - - 0 1");
    move = uci::uciToMove(board, "d7d8");
    delta = evaluateMove(board, move);
    assert(delta == 0);

    std::cout << "All evaluateMove tests passed!" << std::endl;
}

void test_minimax() {
    std::cout << "Testing minimax..." << std::endl;
    Board board;
    Move bestMove;
    vector<uint8_t> positionCounts(HASH_TABLE_SIZE, 0);
    int currentEval, score;

    // Test 1: Checkmate in 1 (white)
    board = Board("3k4/7Q/3K4/8/8/8/8/8 w - - 0 1");
    currentEval = evaluateBoard(board);
    score = white(board, 1, -INFINITY_VAL, INFINITY_VAL, bestMove, currentEval, positionCounts);
    assert(score == INFINITY_VAL);

    // Test 2: Avoid checkmate (black)
    board = Board("7k/5ppp/8/6Q1/8/5K2/8/6R1 b - - 0 1");
    currentEval = evaluateBoard(board);
    score = black(board, 2, -INFINITY_VAL, INFINITY_VAL, bestMove, currentEval, positionCounts);
    assert(uci::moveToUci(bestMove) == "g7g6"); // Block mate

    // Test 3: Capture highest value piece (white)
    board = Board("4k3/8/1n3r2/8/3B4/8/8/4K3 w - - 0 1");
    currentEval = evaluateBoard(board);
    score = white(board, 2, -INFINITY_VAL, INFINITY_VAL, bestMove, currentEval, positionCounts);
    assert(uci::moveToUci(bestMove) == "d4f6"); // Capture rook

    // Test 4: Promote to queen (white)
    board = Board("2k5/P7/8/8/8/8/8/3K4 w - - 0 1");
    currentEval = evaluateBoard(board);
    score = white(board, 2, -INFINITY_VAL, INFINITY_VAL, bestMove, currentEval, positionCounts);
    assert(uci::moveToUci(bestMove) == "a7a8q");

    // Test 5: Threefold repetition (draw)
    board = Board("rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    positionCounts[board.zobrist() % HASH_TABLE_SIZE] = 2;
    currentEval = evaluateBoard(board);
    score = white(board, 0, -INFINITY_VAL, INFINITY_VAL, bestMove, currentEval, positionCounts);
    assert(score == 0);

    std::cout << "All minimax tests passed!" << std::endl;
}

int main() {
    test_evaluateBoard();
    test_evaluateMove();
    test_minimax();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}

#else

int main() {
    handleUCI();
    return 0;
}

#endif

