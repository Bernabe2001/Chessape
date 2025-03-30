#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <limits>
#include <utility>
#include <chrono>
#include <iomanip>
#include <cassert>
#include <math.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "../../chess-library/include/chess.hpp"

using namespace chess;
using namespace std;
using Clock = std::chrono::steady_clock;

// Debug macro: prints only when compiled with -DDEBUG
#ifdef DEBUG
    #define DEBUG_PRINT(x) (std::cerr << x << std::endl)
    // Global counter for nodes analyzed during search (each recursive call is counted)
    uint64_t nodesAnalyzed = 0;
#else
    #define DEBUG_PRINT(x) ((void)0)
#endif

constexpr short INFINITY_VAL = numeric_limits<short>::max();
constexpr int HASH_TABLE_SIZE = 2000000;

short R_FACTOR = 0.5;

bool EARLY_GAME = true;
bool MID_GAME = false;
bool END_GAME = false;

short EXPECTED_MOVES_LEFT = 60;

//-------------------------------------------------------------
// Evaluation namespace with piece values and piece–square tables
//-------------------------------------------------------------
namespace Evaluation {

    constexpr short PST[2][6][64] = {
        { // White tables
            {   // Pawn table × 1
                  0,   0,   0,   0,   0,   0,   0,   0,
                 90,  95,  95,  95,  95, 100,  95,  90,
                 95,  95,  95, 105, 105,  90,  90,  95,
                 90,  90, 100, 125, 125,  90,  90,  90,
                105, 105, 120, 140, 140, 120, 105, 105,
                125, 130, 140, 150, 150, 140, 130, 125,
                170, 180, 190, 210, 210, 190, 180, 170,
                  0,   0,   0,   0,   0,   0,   0,   0
            },
            {   // Knight table × 3
                240, 280, 280, 280, 280, 280, 260, 240,
                270, 290, 310, 315, 315, 310, 290, 270,
                280, 310, 315, 330, 330, 315, 310, 280,
                280, 315, 330, 335, 335, 330, 315, 280,
                280, 315, 330, 335, 335, 330, 315, 280,
                280, 310, 315, 330, 330, 315, 310, 280,
                270, 290, 310, 315, 315, 310, 290, 270,
                240, 280, 280, 280, 280, 280, 280, 240
            },
            {   // Bishop table × 3
                260, 280, 300, 290, 290, 300, 280, 260,
                280, 335, 300, 315, 315, 300, 335, 280,
                300, 300, 325, 325, 325, 325, 300, 300,
                315, 315, 325, 325, 325, 325, 315, 315,
                290, 325, 325, 325, 325, 325, 325, 290,
                300, 300, 325, 325, 325, 325, 300, 300,
                280, 335, 300, 315, 315, 300, 335, 280,
                260, 280, 300, 290, 290, 300, 280, 260
            },
            {   // Rook table × 5
                470, 470, 490, 520, 520, 500, 470, 470,
                460, 460, 480, 540, 540, 480, 460, 460,
                500, 500, 500, 540, 540, 500, 500, 500,
                500, 500, 500, 500, 500, 500, 500, 500,
                500, 500, 500, 500, 500, 500, 500, 500,
                500, 500, 500, 500, 500, 500, 500, 500,
                550, 550, 550, 570, 570, 550, 550, 550,
                510, 510, 510, 550, 550, 510, 510, 510
            },
            {   // Queen table × 9
                850, 870, 890, 900, 900, 890, 870, 850,
                870, 890, 900, 905, 905, 900, 890, 870,
                885, 905, 915, 915, 915, 915, 905, 885,
                905, 910, 910, 910, 910, 910, 910, 900,
                900, 910, 910, 910, 910, 910, 910, 905,
                900, 900, 910, 910, 910, 910, 900, 900,
                905, 905, 910, 910, 910, 910, 905, 905,
                900, 905, 910, 910, 910, 910, 905, 900
            },
            {   // King table × 0
                0,10,10, 0, 0, 0,20, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0
            }
        },
        { // Black tables
            {   // Pawn table × 1
                   0,   0,    0,    0,    0,     0,    0,    0,
                -170, -180, -190, -210, -210, -190, -180, -170,
                -125, -130, -140, -150, -150, -140, -130, -125,
                -105, -105, -120, -140, -140, -120, -105, -105,
                 -90,  -90, -100, -125, -125,  -90,  -90,  -90,
                 -95,  -95,  -95, -105, -105,  -90,  -90,  -95,
                 -90,  -95,  -95,  -95,  -95, -100,  -95,  -90,
                   0,    0,    0,    0,    0,    0,    0,    0
            },
            {   // Knight table × 3
                -240, -280, -280, -280, -280, -280, -260, -240,
                -270, -290, -310, -315, -315, -310, -290, -270,
                -280, -310, -315, -330, -330, -315, -310, -280,
                -280, -315, -330, -335, -335, -330, -315, -280,
                -280, -315, -330, -335, -335, -330, -315, -280,
                -280, -310, -315, -330, -330, -315, -310, -280,
                -270, -290, -310, -315, -315, -310, -290, -270,
                -240, -280, -280, -280, -280, -280, -280, -240
            },
            {   // Bishop table × 3
                -260, -280, -300, -290, -290, -300, -280, -260,
                -280, -335, -300, -315, -315, -300, -335, -280,
                -300, -300, -325, -325, -325, -325, -300, -300,
                -290, -325, -325, -325, -325, -325, -325, -290,
                -315, -315, -325, -325, -325, -325, -315, -315,
                -300, -300, -325, -325, -325, -325, -300, -300,
                -280, -335, -300, -315, -315, -300, -335, -280,
                -260, -280, -300, -290, -290, -300, -280, -260
            },
            {   // Rook table × 5
                -510, -510, -510, -550, -550, -510, -510, -510,
                -550, -550, -550, -570, -570, -550, -550, -550,
                -500, -500, -500, -500, -500, -500, -500, -500,
                -500, -500, -500, -500, -500, -500, -500, -500,
                -500, -500, -500, -500, -500, -500, -500, -500,
                -500, -500, -500, -540, -540, -500, -500, -500,
                -460, -460, -480, -540, -540, -480, -460, -460,
                -470, -470, -490, -520, -520, -500, -470, -470
            },
            {   // Queen table × 9
                -900, -905, -910, -910, -910, -910, -905, -900,
                -905, -905, -910, -910, -910, -910, -905, -905,
                -900, -900, -910, -910, -910, -910, -900, -900,
                -900, -910, -910, -910, -910, -910, -910, -905,
                -905, -910, -910, -910, -910, -910, -910, -900,
                -885, -905, -915, -915, -915, -915, -905, -885,
                -870, -890, -900, -905, -905, -900, -890, -870,
                -850, -870, -890, -900, -900, -890, -870, -850     
            },
            {   // King table × 0
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0,-10,-10,0,0, 0,-20,0
            }
        }
    };
}



//-------------------------------------------------------------
// getPieceSquareValue: returns the PST value for a given piece.
//-------------------------------------------------------------
inline short getPieceSquareValue(PieceType type, int colorId, int squareId) {
    int pieceIndex = static_cast<int>(type);             // Requires PieceType order matches PST
    return Evaluation::PST[colorId][pieceIndex][squareId]; 
}

//-------------------------------------------------------------
// evaluateBoard: computes a static evaluation of the board and actualices the phase of the game
//-------------------------------------------------------------
short evaluateBoard(Board &board) {
    short score = 0;
    short count = 0;
    for (int colorId: {0,1}) {
        for (auto type : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                           PieceType::ROOK, PieceType::QUEEN, PieceType::KING}) {
            Bitboard pieces = board.pieces(type, Color(colorId));
            while (!pieces.empty()){
                int sq = pieces.pop();
                score += getPieceSquareValue(type, colorId, sq);
                count ++;
            }
        }
    }
    if (count <= 12){
        EARLY_GAME = false;
        MID_GAME = false;
        END_GAME = true;
        EXPECTED_MOVES_LEFT = 25;
    }
    else if (count <= 29 || board.fullMoveNumber() >= 6){
        EARLY_GAME = false;
        MID_GAME = true;
        END_GAME = false;
        EXPECTED_MOVES_LEFT = 50;
    }
    else{
        EARLY_GAME = true;
        MID_GAME = false;
        END_GAME = false;
        EXPECTED_MOVES_LEFT = 60;
    }
    return score;
}

//-------------------------------------------------------------
// evaluateCapture: Computes the static evaluation of a capture assuming you lose that piece.
//-------------------------------------------------------------
short evaluateCapture(const Board& board, const Move& move){
    short delta = 0;
    int movingColorIdx = board.sideToMove();
    int oppColorIdx = (movingColorIdx == 0 ? 1 : 0);
    uint16_t mov_type = move.typeOf();
    int from_ind = move.from().index();
    int to_ind = move.to().index();

    if (mov_type == Move::ENPASSANT){
        int capturedSq_ind = (movingColorIdx == 0 ? to_ind - 8 : to_ind + 8);

        delta -= getPieceSquareValue(PieceType::PAWN, movingColorIdx, from_ind);         // Remove our pawn from source
        delta -= getPieceSquareValue(PieceType::PAWN, oppColorIdx, capturedSq_ind);      // Remove opponent's pawn
    } 
    else if (mov_type == Move::PROMOTION){
        delta -= getPieceSquareValue(PieceType::PAWN, movingColorIdx, from_ind);         // Remove pawn
        
        PieceType capture_type = board.at(Square(to_ind)).type();
        delta -= getPieceSquareValue(capture_type, oppColorIdx, to_ind);                 // Remove captured piece
    }
    else{ 
        PieceType pieceType = board.at(Square(from_ind)).type();
        delta -= getPieceSquareValue(pieceType, movingColorIdx, from_ind);               // Remove piece from source

        PieceType capture_type = board.at(Square(to_ind)).type();
        delta -= getPieceSquareValue(capture_type, oppColorIdx, to_ind);             //Remove captured piece
    }
    return delta;
}

//-------------------------------------------------------------
// evaluateMove: Computes a static evaluation of a move.
//-------------------------------------------------------------
short evaluateMove(const Board& board, const Move& move){
    short delta = 0;
    int movingColorIdx = board.sideToMove();
    int oppColorIdx = (movingColorIdx == 0 ? 1 : 0);
    uint16_t mov_type = move.typeOf();
    int from_ind = move.from().index();
    int to_ind = move.to().index();

    if (mov_type == Move::CASTLING){
        delta -= getPieceSquareValue(PieceType::KING, movingColorIdx, from_ind);
        delta += getPieceSquareValue(PieceType::KING, movingColorIdx, to_ind);

        int aux = (from_ind < to_ind ? -1 : 1);                                           
        int rook_initial;
        if (aux == -1) rook_initial = (movingColorIdx == 0 ? 7 : 63);                    // Short castle
        else rook_initial = (movingColorIdx == 0 ? 0 : 56);                              // Long castle

        delta -= getPieceSquareValue(PieceType::KING, movingColorIdx, rook_initial);
        delta += getPieceSquareValue(PieceType::KING, movingColorIdx, to_ind+aux);
    }
    else if (mov_type == Move::ENPASSANT){
        int capturedSq_ind = (movingColorIdx == 0 ? to_ind - 8 : to_ind + 8);

        delta -= getPieceSquareValue(PieceType::PAWN, movingColorIdx, from_ind);         // Remove our pawn from source
        delta -= getPieceSquareValue(PieceType::PAWN, oppColorIdx, capturedSq_ind);      // Remove opponent's pawn
        delta += getPieceSquareValue(PieceType::PAWN, movingColorIdx, to_ind);           // Include our pawn in destination
    } 
    else if (mov_type == Move::PROMOTION){
        delta -= getPieceSquareValue(PieceType::PAWN, movingColorIdx, from_ind);         // Remove pawn
        delta += getPieceSquareValue(move.promotionType(), movingColorIdx, to_ind);      // Include new piece
        
        PieceType capture_type = board.at(Square(to_ind)).type();
        if (capture_type != PieceType::NONE){
            delta -= getPieceSquareValue(capture_type, oppColorIdx, to_ind);             // Remove captured piece
        }
    }
    else{ 
        PieceType pieceType = board.at(Square(from_ind)).type();
        delta -= getPieceSquareValue(pieceType, movingColorIdx, from_ind);               // Remove piece from source
        delta += getPieceSquareValue(pieceType, movingColorIdx, to_ind);                 // Include piece in destination

        PieceType capture_type = board.at(Square(to_ind)).type();
        if (capture_type != PieceType::NONE){
            delta -= getPieceSquareValue(capture_type, oppColorIdx, to_ind);             //Remove captured piece
        }
    }
    return delta;
}


short black(Board &board, short depth, short alpha, short beta, Move &bestMove, short currentEval, vector<uint8_t> &positionCounts, short min_depth,  short max_depth);
short white(Board &board, short depth, short alpha, short beta, Move &bestMove, short currentEval, vector<uint8_t> &positionCounts, short min_depth,  short max_depth) {
    #ifdef DEBUG
    ++nodesAnalyzed; // Debug: count node analyzed for white search
    #endif

    // Terminal condition 1: 50 moves rule
    if (board.isHalfMoveDraw()){
        GameResult result = board.getHalfMoveDrawType().second;
        if (result == GameResult::LOSE) return -INFINITY_VAL;
        else return 0;
    }

    // Terminal condition 2 : triple repetition
    uint64_t zobrist_w = board.zobrist() % HASH_TABLE_SIZE;
    uint8_t rep = positionCounts[zobrist_w];
    if (rep == 2) return 0;

    Movelist unordered_moves;
    movegen::legalmoves(unordered_moves, board);

    // Terminal condition 3: no moves allowed (received checkmate or stalemate)
    if (unordered_moves.empty()){
        GameResult result = board.isGameOver().second;
        if (result == GameResult::LOSE) return -INFINITY_VAL;
        else return 0; 
    }

    bestMove = unordered_moves[0];

    // Terminal condition 4: max depth reached
    if (depth == max_depth) return currentEval+10;

    bool in_check = board.inCheck();

    // Null move pruning (inactive in endgames)
    if (!END_GAME && currentEval - 100 > beta && !in_check && min_depth - depth > 2) {   
        board.makeNullMove();
        Movelist moves_aux;                                               
        movegen::legalmoves(moves_aux, board);
        if (!moves_aux.empty()){
            short new_min_depth = min_depth - depth - 2;
            short new_max_depth = min_depth - depth;
            short null_move_score = black(board, 0, beta-1, beta, bestMove, currentEval, positionCounts, new_min_depth, new_max_depth); // Give turn away, small window for efficiency
            if (null_move_score >= beta){
                board.unmakeNullMove();
                return null_move_score;                                                                                                 // Return null_move score
            }
        }
        bestMove = unordered_moves[0];
        board.unmakeNullMove();
    }

    // Move ordering
    Movelist queen_promotion;
    Movelist check_and_capture;
    Movelist check;
    Movelist good_capture;
    Movelist bad_capture;
    Movelist quiet;

    for (const auto& move : unordered_moves){ 
        board.makeMove(move);
        bool is_check = board.inCheck();
        board.unmakeMove(move);
    
        bool is_capture = board.isCapture(move) && move.typeOf() != Move::CASTLING;
        bool is_queen_promotion = (move.typeOf() == Move::PROMOTION && move.promotionType() == PieceType::QUEEN);
    
        if (is_queen_promotion) queen_promotion.add(move);
        else if (is_check && is_capture) check_and_capture.add(move);
        else if (is_capture) {
            if (evaluateCapture(board, move) > 100) good_capture.add(move);
            else bad_capture.add(move);
        }
        else if (is_check) check.add(move);
        else quiet.add(move);
    }

    Movelist moves;
    for (const auto& m : queen_promotion) moves.add(m);
    for (const auto& m : check_and_capture) moves.add(m);
    for (const auto& m : good_capture) moves.add(m);
    if (depth < min_depth + 3){
        for (const auto& m : check) moves.add(m);
    }
    for (const auto& m : bad_capture) moves.add(m);

    short best = -INFINITY_VAL;
    
    if (depth < min_depth || in_check) {
        for (const auto& m : quiet) moves.add(m); // If in check or min_depth not reached, analyze all movements
    }
    else { // Not in check, min_depth reached 
        if (currentEval-10 >= beta) return currentEval-10;
        else best = currentEval-10; // Standing pat
    }

    Move dummy;
    for (auto &move : moves) {
        short evalDelta = 0;

        // Calculate the change in evaluation caused by the move
        evalDelta = evaluateMove(board, move);
        currentEval += evalDelta;

        board.makeMove(move);
        positionCounts[zobrist_w] += 1;
        short score = black(board, depth + 1, alpha, beta, dummy, currentEval, positionCounts, min_depth, max_depth);
        positionCounts[zobrist_w] -= 1;
        board.unmakeMove(move);

        // Restore the evaluation to its previous state
        currentEval -= evalDelta;

        if (score > best) {
            best = score;
            bestMove = move;
        }
        alpha = max(alpha, score);
        if (alpha >= beta) {
            break;
        }
    }
    return best;
}


short black(Board &board, short depth, short alpha, short beta, Move &bestMove, short currentEval, vector<uint8_t> &positionCounts, short min_depth,  short max_depth){
    #ifdef DEBUG
        ++nodesAnalyzed; // Debug: count node analyzed for black search
    #endif
    
    // Terminal condition 1: 50 moves rule
    if (board.isHalfMoveDraw()){
        GameResult result = board.getHalfMoveDrawType().second;
        if (result == GameResult::LOSE) return INFINITY_VAL;
        else return 0;
    }

    // Terminal condition 2 : triple repetition
    uint64_t zobrist_w = board.zobrist() % HASH_TABLE_SIZE;
    uint8_t rep = positionCounts[zobrist_w];
    if (rep == 2) return 0;

    Movelist unordered_moves;
    movegen::legalmoves(unordered_moves, board);

    // Terminal condition 3: no moves allowed (received checkmate or stalemate)
    if (unordered_moves.empty()){
        GameResult result = board.isGameOver().second;
        if (result == GameResult::LOSE) return INFINITY_VAL;
        else return 0; 
    }

    bestMove = unordered_moves[0];

    if (depth == max_depth) return currentEval - 10; //Max depth

    bool in_check = board.inCheck();

    // Null move pruning (inactive in endgames)
    if (!END_GAME && currentEval + 100 < alpha && !in_check && min_depth - depth > 2) {
        board.makeNullMove();
        Movelist moves_aux;                                               
        movegen::legalmoves(moves_aux, board);
        if (!moves_aux.empty()){
            short new_min_depth = min_depth - depth - 2;
            short new_max_depth = min_depth - depth;
            short null_move_score = white(board, 0, alpha, alpha+1, bestMove, currentEval, positionCounts, new_min_depth, new_max_depth); // Give turn away, small window for efficiency
            if (null_move_score <= alpha){
                board.unmakeNullMove();
                return null_move_score;                                                                       // Return null_move score
            }
        }
        bestMove = unordered_moves[0];
        board.unmakeNullMove();
    }

    // Move ordering
    Movelist queen_promotion;
    Movelist check_and_capture;
    Movelist check;
    Movelist good_capture;
    Movelist bad_capture;
    Movelist quiet;

    for (const auto& move : unordered_moves){ 
        board.makeMove(move);
        bool is_check = board.inCheck();
        board.unmakeMove(move);
    
        bool is_capture = board.isCapture(move) && move.typeOf() != Move::CASTLING;
        bool is_queen_promotion = (move.typeOf() == Move::PROMOTION && move.promotionType() == PieceType::QUEEN);
    
        if (is_queen_promotion) queen_promotion.add(move);
        else if (is_check && is_capture) check_and_capture.add(move);
        else if (is_capture) {
            if (evaluateCapture(board, move) < -100) good_capture.add(move);
            else bad_capture.add(move);
        }
        else if (is_check) check.add(move);
        else quiet.add(move);
    }

    Movelist moves;
    for (const auto& m : queen_promotion) moves.add(m);
    for (const auto& m : check_and_capture) moves.add(m);
    for (const auto& m : good_capture) moves.add(m);
    if (depth < min_depth + 3) for (const auto& m : check) moves.add(m);
    for (const auto& m : bad_capture) moves.add(m);

    short best = +INFINITY_VAL;
    
    if (depth < min_depth || in_check) {
        for (const auto& m : quiet) moves.add(m); // If in check or min_depth not reached, analyze all movements
    }
    else { // Not in check, min_depth reached 
        if (currentEval+10 <= alpha) return currentEval+10;
        else best = currentEval+10; // Standing pat
    }

    Move dummy;
    for (auto &move : moves) {
        short evalDelta = 0;

        // Calculate the change in evaluation caused by the move
        evalDelta = evaluateMove(board, move);
        currentEval += evalDelta;

        board.makeMove(move);
        positionCounts[zobrist_w] += 1;
        short score = white(board, depth + 1, alpha, beta, dummy, currentEval, positionCounts, min_depth, max_depth);
        positionCounts[zobrist_w] -= 1;
        board.unmakeMove(move);

        // Restore the evaluation to its previous state
        currentEval -= evalDelta;

        if (score < best) {
            best = score;
            bestMove = move;
        }
        beta = min(beta, score);
        if (alpha >= beta) {
            break;
        }
    }
    return best;
}




struct SearchParameters {
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    int movestogo = 0;
    int depth = 0;
    int nodes = 0;
    int mate = 0;
    int movetime = 0;
    bool infinite = false;
    vector<string> searchmoves;
    bool ponder = false;
};

//-------------------------------------------------------------
// UCI Command Handler: Processes UCI protocol commands.
//-------------------------------------------------------------

class UCIHandler {
    private:
        Board board;
        atomic<bool> searching{false};
        thread search_thread;
        mutex board_mutex;
        SearchParameters search_params;
        vector<uint8_t> positionCounts = vector<uint8_t>(HASH_TABLE_SIZE, 0);
        bool uciChess960 = false;
        
        // Engine options
        unordered_map<string, string> options;
        
    public:
        // Destructor to join any running search thread
        ~UCIHandler() {
            if (search_thread.joinable()) {
                search_thread.join();
            }
        }
        
        void run() {
            string line;
            while (getline(cin, line)) {
                process_command(line);
            }
        }
        
    private:
        void process_command(const string& input) {
            istringstream iss(input);
            string token;
            iss >> token;
        
            if (token == "uci") {
                handle_uci();
            } else if (token == "isready") {
                cout << "readyok" << endl;
            } else if (token == "setoption") {
                handle_setoption(iss);
            } else if (token == "ucinewgame") {
                handle_ucinewgame();
            } else if (token == "position") {
                handle_position(iss);
            } else if (token == "go") {
                handle_go(iss);
            } else if (token == "stop") {
                handle_stop();
            } else if (token == "quit") {
                handle_quit();
            } else if (token == "debug") {
                // Handle debug mode
            }
            // Add other command handlers as needed
        }
        
        void handle_uci() {
            cout << "id name Chessape_1.2" << endl;
            cout << "id author Bernabé Iturralde Jara" << endl;
            cout << "option name UCI_Chess960 type check default false" << endl;
            cout << "uciok" << endl;
        }
        

	void handle_setoption(istringstream& iss) {
    		string word, name, value;
    		while (iss >> word) {
        		if (word == "name") {
            			iss >> name;
        		} else if (word == "value") {
            		iss >> value;
        		}
    		}
    		if (name == "UCI_Chess960") {
        		uciChess960 = (value == "true" || value == "1");
        		cerr << "UCI_Chess960 set to " << (uciChess960 ? "true" : "false") << endl;
    		}
	}
        
        void handle_ucinewgame() {
            // Reset board and any game-specific state
            lock_guard<mutex> lock(board_mutex);
            board = Board(); // Reset to initial position
            positionCounts = vector<uint8_t>(HASH_TABLE_SIZE, 0);
        }
        
        void handle_position(istringstream& iss) {
            lock_guard<mutex> lock(board_mutex);
            string token;
            iss >> token;
            
            if (token == "startpos") {
                board = Board();
                iss >> token; // Consume "moves" if present
            } else if (token == "fen") {
                string fen;
                while (iss >> token && token != "moves") {
                    fen += token + " ";
                }
                board = Board(fen);
            }
        
            // Process moves
            while (iss >> token) {
                DEBUG_PRINT("[DEBUG] Applying move: " + token);
                Move move = uci::uciToMove(board, token);
                board.makeMove(move);
            }
            DEBUG_PRINT("[DEBUG] Final FEN: " + board.getFen());
            DEBUG_PRINT("[DEBUG] sideToMove: " + string((board.sideToMove() == Color::WHITE) ? "WHITE" : "BLACK"));
        }
        
        void handle_go(istringstream& iss) {
            // Join any previous search thread if it is joinable
            if (search_thread.joinable()) {
                search_thread.join();
            }
            
            string token;
            // Update only provided parameters; others persist from previous command.
            while (iss >> token) {
                if (token == "wtime") {
                    iss >> search_params.wtime;
                } else if (token == "btime") {
                    iss >> search_params.btime;
                } else if (token == "winc") {
                    iss >> search_params.winc;
                } else if (token == "binc") {
                    iss >> search_params.binc;
                } else if (token == "movestogo") {
                    iss >> search_params.movestogo;
                } else if (token == "depth") {
                    iss >> search_params.depth;
                } else if (token == "nodes") {
                    iss >> search_params.nodes;
                } else if (token == "mate") {
                    iss >> search_params.mate;
                } else if (token == "movetime") {
                    iss >> search_params.movetime;
                } else if (token == "infinite") {
                    search_params.infinite = true;
                } else if (token == "searchmoves") {
                    search_params.searchmoves.clear();
                    string move;
                    while (iss >> move) {
                        search_params.searchmoves.push_back(move);
                    }
                } else if (token == "ponder") {
                    search_params.ponder = true;
                }
            }
            
            if (searching) { // Already searching, ignore new command.
                return;
            }
            
            searching = true;
            search_thread = thread(&UCIHandler::start_search, this);
        }
        
        
        void start_search() {
            lock_guard<mutex> lock(board_mutex);
            DEBUG_PRINT("[DEBUG] Starting search on FEN: " + board.getFen());
            auto go_beg = Clock::now();
            
            // Get initial time values from current search parameters
            int my_time = (board.sideToMove() == Color::WHITE) ? search_params.wtime : search_params.btime;
            int my_inc = (board.sideToMove() == Color::WHITE) ? search_params.winc : search_params.binc;
            
            // Calculate allocated time for this move
            int moveTime = my_inc + (my_time / EXPECTED_MOVES_LEFT);
        
            DEBUG_PRINT("[DEBUG] Initial time: " + to_string(my_time) + "ms");
            DEBUG_PRINT("[DEBUG] Estimated move time: " + to_string(moveTime) + "ms");
        
            Move bestMove;
            short currentEval = evaluateBoard(board);
            short score;
            uint64_t zobrist = board.zobrist() % HASH_TABLE_SIZE;
        
            int currentMinDepth = 5;
            int currentMaxDepth = 10;
            long elapsed = 0;
        
            do {
                #ifdef DEBUG
                nodesAnalyzed = 0;
                auto searchStart = Clock::now();
                #endif
        
                if (board.sideToMove() == Color::WHITE) {
                    score = white(board, 0, -INFINITY_VAL, INFINITY_VAL, bestMove,
                                  currentEval, positionCounts, currentMinDepth, currentMaxDepth);
                } else {
                    score = black(board, 0, -INFINITY_VAL, INFINITY_VAL, bestMove,
                                  currentEval, positionCounts, currentMinDepth, currentMaxDepth);
                }
        
                #ifdef DEBUG
                auto searchEnd = Clock::now();
                elapsed = chrono::duration_cast<chrono::milliseconds>(searchEnd - searchStart).count();
                DEBUG_PRINT("[DEBUG] Depth [" + to_string(currentMinDepth) + ", " + 
                            to_string(currentMaxDepth) + "] - Nodes analyzed: " + to_string(nodesAnalyzed));
                DEBUG_PRINT("[DEBUG] Time of execution: " + to_string(elapsed) + "ms");
                if (elapsed > 0) DEBUG_PRINT("[DEBUG] Speed: " + to_string(nodesAnalyzed / elapsed) + "knps");
                DEBUG_PRINT("[DEBUG] Move: " + uci::moveToUci(bestMove));
                DEBUG_PRINT("[DEBUG] Score: " + to_string(score));
                #endif
        
                // Calculate remaining time
                auto now = Clock::now();
                long total_elapsed = chrono::duration_cast<chrono::milliseconds>(now - go_beg).count();
                int remaining_time = my_time - total_elapsed;
        
                DEBUG_PRINT("[DEBUG] Total elapsed: " + to_string(total_elapsed) + "ms");
                DEBUG_PRINT("[DEBUG] Remaining time: " + to_string(remaining_time) + "ms");
        
                bool terminalScore = false;
                if (score == INFINITY_VAL || score == -INFINITY_VAL) {
                    terminalScore = true;
                }
        
                // Time management logic
                if (!terminalScore) {
                    if (total_elapsed < moveTime / 5) {
                        currentMinDepth += 1;
                        currentMaxDepth += 2;
                        DEBUG_PRINT("[DEBUG] Increasing depth to MIN_DEPTH=" +
                                  to_string(currentMinDepth) + ", MAX_DEPTH=" + to_string(currentMaxDepth));
                    }
                    else break;
                } 
                else break;
            } while (true);
        
            auto go_end = Clock::now();
            long total_elapsed = chrono::duration_cast<chrono::milliseconds>(go_end - go_beg).count();
            if (board.sideToMove() == Color::WHITE)
                search_params.wtime += search_params.winc - total_elapsed;
            else
                search_params.btime += search_params.binc - total_elapsed;
            
            // Update position count for threefold repetition
            positionCounts[zobrist] += 1;
        
            searching = false;
            
            // Send final best move
            cout << "bestmove " << uci::moveToUci(bestMove) << endl;
        }
        
        void send_info(const string& message) {
            cout << "info " << message << endl;
        }
        
        void handle_stop() {
            if (searching) {
                searching = false;
                if (search_thread.joinable()) {
                    search_thread.join();
                }
            }
        }
        
        void handle_quit() {
            handle_stop();
            exit(0);
        }
    };
    
int main() {
    UCIHandler handler;
    handler.run();
    return 0;
}


