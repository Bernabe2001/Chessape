#include <iostream>
#include "nnue_eval.h"

int main() {
    NNUE net("weights.txt");

    std::vector<float> input(768, 0.0f);
    // Set ones for the starting position:
    int pieces[32][2] = {
        {0, 8}, {0, 9}, {0,10}, {0,11}, {0,12}, {0,13}, {0,14}, {0,15},  // Black pawns
        {6, 0}, {7, 1}, {8, 2}, {9, 3}, {10, 4}, {11, 5}, {8, 6}, {7, 7}, // Black pieces
        {0, 48}, {0,49}, {0,50}, {0,51}, {0,52}, {0,53}, {0,54}, {0,55}, // White pawns
        {0,56}, {1,57}, {2,58}, {3,59}, {4,60}, {5,61}, {2,62}, {1,63}   // White pieces
    };
    for (auto& p : pieces) {
        input[p[0] * 64 + p[1]] = 1.0f;
    }

    float score = net.evaluate(input);
    std::cout << "NNUE evaluation of starting position: " << score << " centipawns\n";
    return 0;
}


