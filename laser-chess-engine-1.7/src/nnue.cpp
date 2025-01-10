#include "nnue.h"
#include <algorithm>
#include <cmath>

namespace NNUE {
    // Weight tables - designed to capture basic chess principles
    static const int16_t PIECE_VALUES[12] = {
        100,   // White pawn
        320,   // White knight
        330,   // White bishop
        500,   // White rook
        900,   // White queen
        20000, // White king
        -100,  // Black pawn
        -320,  // Black knight
        -330,  // Black bishop
        -500,  // Black rook
        -900,  // Black queen
        -20000 // Black king
    };

    static const int16_t POSITION_WEIGHTS[64] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        5,  10, 10, -20,-20, 10, 10, 5,
        5,  -5, -10, 0,  0, -10,-5,  5,
        0,  0,  0,  20, 20, 0,  0,  0,
        5,  5,  10, 25, 25, 10, 5,  5,
        10, 10, 20, 30, 30, 20, 10, 10,
        50, 50, 50, 50, 50, 50, 50, 50,
        0,  0,  0,  0,  0,  0,  0,  0
    };

    // These weights are derived from piece values and position weights
    static int16_t input_weights[INPUT_SIZE][HIDDEN_SIZE];
    static int16_t output_weights[HIDDEN_SIZE];

    void init_weights() {
        // Initialize input weights based on piece-square combinations
        for (int piece = 0; piece < 12; piece++) {
            for (int square = 0; square < 64; square++) {
                for (int hidden = 0; hidden < HIDDEN_SIZE; hidden++) {
                    int base_weight = PIECE_VALUES[piece] / 100;
                    int pos_weight = POSITION_WEIGHTS[square];
                    
                    // Create different patterns for each hidden neuron
                    int pattern = (hidden * 7 + square * 13 + piece * 17) % 32;
                    int weight = base_weight * (pattern - 16) / 16;
                    
                    // Add positional consideration
                    weight += pos_weight * (piece < 6 ? 1 : -1);
                    
                    input_weights[piece * 64 + square][hidden] = weight;
                }
            }
        }

        // Initialize output weights with a pattern
        for (int i = 0; i < HIDDEN_SIZE; i++) {
            output_weights[i] = ((i % 2) * 2 - 1) * (16 + (i / 2));
        }
    }

    void Accumulator::clear() {
        std::fill(values.begin(), values.end(), 0);
    }

    Network::Network() {
        static bool weights_initialized = false;
        if (!weights_initialized) {
            init_weights();
            weights_initialized = true;
        }
        accumulator.clear();
    }

    void Network::refresh(const Board& board) {
        accumulator.clear();
        
        // Update accumulator based on current board position
        for (int square = 0; square < 64; square++) {
            int piece = board.getPieces(square);
            if (piece != EMPTY) {
                update_accumulator(piece, square, true);
            }
        }
    }

    void Network::update_accumulator(int piece, int square, bool add) {
        if (piece == EMPTY) return;
        
        int feature_index = piece * 64 + square;
        int16_t multiplier = add ? 1 : -1;
        
        for (int i = 0; i < HIDDEN_SIZE; i++) {
            accumulator.values[i] = clamp(accumulator.values[i] + 
                input_weights[feature_index][i] * multiplier);
        }
    }

    int Network::evaluate(const Board& board) {
        // Ensure accumulator is up to date
        refresh(board);
        
        // Calculate final output
        int32_t sum = 0;
        for (int i = 0; i < HIDDEN_SIZE; i++) {
            sum += accumulator.values[i] * output_weights[i];
        }
        
        // Scale output to centipawns and ensure it's within reasonable bounds
        return clamp(sum / 1024);
    }
}

// Global network instance
NNUE::Network network;
