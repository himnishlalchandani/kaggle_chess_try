#ifndef NNUE_H
#define NNUE_H

#include <cstdint>
#include <array>
#include "board.h"

namespace NNUE {
    constexpr int INPUT_SIZE = 768;  // 64 squares * 12 piece types
    constexpr int HIDDEN_SIZE = 32;  // Small hidden layer
    constexpr int OUTPUT_SIZE = 1;

    class Accumulator {
    public:
        std::array<int16_t, HIDDEN_SIZE> values;
        void clear();
    };

    class Network {
    public:
        Network();
        int evaluate(const Board& board);
        void refresh(const Board& board);

    private:
        void update_accumulator(int piece, int square, bool add);
        static constexpr int16_t clamp(int x) {
            return std::max(-32768, std::min(32767, x));
        }
        Accumulator accumulator;
    };
}

extern NNUE::Network network;

#endif // NNUE_H
