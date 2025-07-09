#include "../../chess-library/include/chess.hpp"
#include <vector>

using namespace chess;

std::vector<float> nnue_input_from_board(const Board& board) {
    std::vector<float> input(768, 0.0f);

    for (int square = 0; square < 64; ++square) {
        Piece piece = board.at(Square(square));
        if (piece == Piece::NONE) continue;

        int type = static_cast<int>(piece.type());  // 0=Pawn, 1=Knight, ..., 5=King
        int color_offset = (piece.color() == Color::BLACK) ? 6 : 0;

        int idx = color_offset + type;  // Total index 0–11
        input[idx * 64 + square] = 1.0f;
    }

    return input;
}

