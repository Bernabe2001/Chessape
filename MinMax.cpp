#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include "chess-library/include/chess.hpp"

using namespace chess;
using namespace std;

constexpr int INFINITY = numeric_limits<int>::max();

// Helper function to extract and remove the least significant bit from a bitboard
Square popLeastSignificantBit(Bitboard &bb) {
    return static_cast<Square>(bb.pop()); // Use the return value of pop()
}

// Basic evaluation function (material count only, adjusted for checkmate priority)
int evaluate(const Board &board) {
    // Check for game-ending conditions
    auto [reason, result] = board.isGameOver();
    if (reason == GameResultReason::CHECKMATE) {
        return (board.sideToMove() == Color::WHITE ? -INFINITY : INFINITY);
    }
    if (reason == GameResultReason::STALEMATE) {
        return 0; // Stalemate is neutral
    }

    int score = 0;

    for (auto color : {Color::WHITE, Color::BLACK}) {
        int multiplier = (color == Color::WHITE) ? 1 : -1;

        for (auto type : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN, PieceType::KING}) {
            Bitboard pieces = board.pieces(type, color);

            while (!pieces.empty()) {
                Square sq = popLeastSignificantBit(pieces);

                switch (static_cast<int>(type)) {
                    case static_cast<int>(PieceType::PAWN):
                        score += multiplier * 1;
                        break;
                    case static_cast<int>(PieceType::KNIGHT):
                    case static_cast<int>(PieceType::BISHOP):
                        score += multiplier * 3;
                        break;
                    case static_cast<int>(PieceType::ROOK):
                        score += multiplier * 5;
                        break;
                    case static_cast<int>(PieceType::QUEEN):
                        score += multiplier * 9;
                        break;
                    case static_cast<int>(PieceType::KING):
                        score += multiplier * 100;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return score;
}

// Minimax with Alpha-Beta Pruning
int minimax(Board &board, int depth, bool isMaximizingPlayer, int alpha, int beta) {
    auto [reason, result] = board.isGameOver();
    if (depth == 0 || reason != GameResultReason::NONE) {
        return evaluate(board);
    }

    Movelist moves;
    movegen::legalmoves(moves, board);

    if (isMaximizingPlayer) {
        int maxEval = -INFINITY;
        for (const auto &move : moves) {
            board.makeMove(move);
            int eval = minimax(board, depth - 1, false, alpha, beta);
            board.unmakeMove(move);
            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);
            if (beta <= alpha) {
                break;
            }
        }
        return maxEval;
    } else {
        int minEval = INFINITY;
        for (const auto &move : moves) {
            board.makeMove(move);
            int eval = minimax(board, depth - 1, true, alpha, beta);
            board.unmakeMove(move);
            minEval = min(minEval, eval);
            beta = min(beta, eval);
            if (beta <= alpha) {
                break;
            }
        }
        return minEval;
    }
}

// Find the best move
pair<Move, int> find_best_move_with_evaluation(Board &board, int depth) {
    Move bestMove;
    int bestValue = -INFINITY;
    int alpha = -INFINITY, beta = INFINITY;

    Movelist moves;
    movegen::legalmoves(moves, board);

    for (const auto &move : moves) {
        board.makeMove(move);
        int moveValue = minimax(board, depth - 1, false, alpha, beta);
        board.unmakeMove(move);

        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
        }
    }

    return {bestMove, bestValue};
}

int main() {
    ifstream inputFile("input.txt");
    if (!inputFile) {
        cerr << "Error: Unable to open input.txt" << endl;
        return 1;
    }

    string fen;
    getline(inputFile, fen);
    inputFile.close();

    Board board(fen); // Initialize board with input from file

    cout << "Input Board:\n";
    cout << board.getFen() << endl;

    if (auto [reason, result] = board.isGameOver(); reason != GameResultReason::NONE) {
        cout << "The game is already over." << endl;
        return 0;
    }

    int depth = 4; // Set depth for minimax
    auto [bestMove, evaluation] = find_best_move_with_evaluation(board, depth);

    cout << "Best Move: " << uci::moveToUci(bestMove) << endl;
    cout << "Evaluation: " << evaluation << endl;

    return 0;
}
