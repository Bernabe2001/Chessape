#pragma once
#include <vector>
#include <string>

class NNUE {
public:
    NNUE(const std::string& weight_file);
    float evaluate(const std::vector<float>& input);  // Input: 768-element vector

private:
    std::vector<std::vector<float>> weights1, weights2, weights3;
    std::vector<float> bias1, bias2, bias3;

    std::vector<float> relu(const std::vector<float>& x);
    std::vector<float> linear(const std::vector<float>& input,
                              const std::vector<std::vector<float>>& weights,
                              const std::vector<float>& bias);
};

