#include "nnue_eval.h"
#include "../../chess-library/include/chess.hpp"
#include <iostream>
#include <vector>

extern NNUE nnue_model;
std::vector<float> nnue_input_from_board(const chess::Board& board);

short evaluateBoardNNUE(const chess::Board& board) {
    std::cerr << "[DEBUG] Inside evaluateBoardNNUE" << std::endl;

    auto input = nnue_input_from_board(board);
    std::cerr << "[DEBUG] Input vector size: " << input.size() << std::endl;

    float score = nnue_model.evaluate(input);
    std::cerr << "[DEBUG] Score from NNUE: " << score << std::endl;

    return static_cast<short>(score);
}

