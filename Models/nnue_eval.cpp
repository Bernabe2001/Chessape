#include "nnue_eval.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>


NNUE::NNUE(const std::string& file) {
    std::ifstream in(file);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open weights file: " + file);
    }

    std::string line;
    int total_lines = 0;

    auto read_matrix = [&](int rows, int cols, std::vector<std::vector<float>>& matrix) {
        matrix.resize(rows, std::vector<float>(cols));
        for (int i = 0; i < rows; ++i) {
            if (!std::getline(in, line)) {
                std::cerr << "Unexpected EOF reading matrix row " << i << "\n";
                throw std::runtime_error("Unexpected EOF");
            }
            total_lines++;
            std::istringstream ss(line);
            for (int j = 0; j < cols; ++j) {
                float val;
                if (!(ss >> val)) {
                    std::cerr << "Parse error at matrix[" << i << "][" << j << "]\n";
                    throw std::runtime_error("Bad weight value");
                }
                matrix[i][j] = val;
            }
        }
    };

    auto read_bias = [&](int size, std::vector<float>& bias) {
        if (!std::getline(in, line)) {
            std::cerr << "Unexpected EOF reading bias\n";
            throw std::runtime_error("Unexpected EOF");
        }
        total_lines++;
        std::istringstream ss(line);
        float val;
        bias.resize(size);
        for (int i = 0; i < size; ++i) {
            if (!(ss >> val)) {
                std::cerr << "Parse error at bias[" << i << "]\n";
                throw std::runtime_error("Bad bias value");
            }
            bias[i] = val;
        }
    };

    // Read all layers in correct order
    read_matrix(512, 768, weights1);
    read_bias(512, bias1);

    read_matrix(256, 512, weights2);
    read_bias(256, bias2);

    read_matrix(1, 256, weights3);
    read_bias(1, bias3);

    std::cout << "[DEBUG] Loaded weights successfully, total lines: " << total_lines << "\n";
}

std::vector<float> NNUE::relu(const std::vector<float>& x) {
    std::vector<float> out(x.size());
    for (size_t i = 0; i < x.size(); ++i)
        out[i] = std::max(0.0f, x[i]);
    return out;
}

std::vector<float> NNUE::linear(const std::vector<float>& input,
                                const std::vector<std::vector<float>>& weights,
                                const std::vector<float>& bias) {
    std::vector<float> out(bias);
    for (size_t i = 0; i < weights.size(); ++i)
        for (size_t j = 0; j < input.size(); ++j)
            out[i] += weights[i][j] * input[j];
    return out;
}

float NNUE::evaluate(const std::vector<float>& input) {
    assert(input.size() == 768);

    auto x1 = relu(linear(input, weights1, bias1));
    auto x2 = relu(linear(x1, weights2, bias2));
    auto out = linear(x2, weights3, bias3);
    return out[0];  // Centipawn score
}

