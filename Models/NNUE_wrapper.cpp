// nnue_wrapper.cpp
#include "nnue_wrapper.h"
#include "nnue_eval.h"  // Your NNUE class (you should have this from earlier)
#include <vector>

using namespace chess;

// Global NNUE instance
NNUE nnue_model("weights.txt");  // Load the trained weights file at start

std::vector<float> nnue_input_from_board(const Board &board) {
    std::vector<float> input(768, 0.0f);

    for (int square = 0; square < 64; ++square) {
        if (board.at(Square(square)) == Piece::NONE) continue;

        Piece piece = board.at(Square(square));
        int type_idx = -1;

        switch (piece.type()) {
            case PieceType::PAWN:   type_idx = 0; break;
            case PieceType::KNIGHT: type_idx = 1; break;
            case PieceType::BISHOP: type_idx = 2; break;
            case PieceType::ROOK:   type_idx = 3; break;
            case PieceType::QUEEN:  type_idx = 4; break;
            case PieceType::KING:   type_idx = 5; break;
            default: continue;
        }

        if (piece.color() == Color::BLACK)
            type_idx += 6;  // Offset for black pieces

        input[type_idx * 64 + square] = 1.0f;
    }

    return input;
}

short evaluateBoardNNUE(const Board &board) {
    std::vector<float> input = nnue_input_from_board(board);
    float score = nnue_model.evaluate(input);
    return static_cast<short>(score);  // output already in centipawns
}

