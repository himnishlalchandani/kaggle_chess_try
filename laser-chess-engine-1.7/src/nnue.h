#ifndef NNUE_H
#define NNUE_H

#include <array>
#include <algorithm>
#include "board.h"

namespace NNUE {
    constexpr int INPUT_SIZE = 768;  // 64 squares * 12 piece types
    constexpr int HIDDEN_SIZE = 32;  // Small hidden layer
    constexpr int OUTPUT_SIZE = 1;
    constexpr int EMPTY = -1;  // Added EMPTY constant

    class Accumulator {
    public:
        std::array<int16_t, HIDDEN_SIZE> values;  // Fixed array declaration
        void clear();
    };

    class Network {
    public:
        Network();
        int evaluate(const Board& board);
        void refresh(const Board& board);
    private:
        void update_accumulator(int piece, int square, bool add);
        
        // Fixed constexpr clamp function
        static constexpr int16_t clamp(int x) {
            return (x < -32768) ? -32768 : (x > 32767 ? 32767 : x);
        }
        
        Accumulator accumulator;
    };
}
