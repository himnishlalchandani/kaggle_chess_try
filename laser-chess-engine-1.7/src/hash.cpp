#include "hash.h"

// Structure packing to minimize memory waste
#pragma pack(push, 1)
struct HashEntry {
    uint64_t zobristKey;  // 8 bytes
    Move move;           // Assuming Move is 4 bytes (32 bits)
    int16_t score;      // 2 bytes (reduced from int)
    int16_t eval;       // 2 bytes (reduced from int)
    uint8_t depth;      // 1 byte (reduced from int)
    uint8_t ageNodeType;// 1 byte (age and node type combined)
};

struct HashNode {
    HashEntry slot1;
    HashEntry slot2;
};
#pragma pack(pop)

Hash::Hash(uint64_t MB) {
    init(MB);
}

Hash::~Hash() {
    free(table);
}

void Hash::init(uint64_t MB) {
    // Convert to bytes (5MB = 5,242,880 bytes)
    uint64_t bytes = MB << 20;
    
    // Calculate maximum number of HashNode entries
    // Each HashNode contains 2 HashEntry structures
    // Size of each HashEntry = 18 bytes
    // Size of HashNode = 36 bytes (2 * 18)
    uint64_t maxSize = bytes / sizeof(HashNode);
    
    // Find the largest power of 2 that fits
    size = 1;
    while (size <= maxSize)
        size <<= 1;
    size >>= 1;
    
    // Allocate the table
    table = (HashNode*)aligned_alloc(64, size * sizeof(HashNode));
    clear();
}

void Hash::add(Board &b, int score, Move move, int eval, int depth, uint8_t nodeType) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h & (size-1);
    HashNode *node = table + index;
    
    // Clamp values to fit in smaller data types
    int16_t clampedScore = std::clamp(score, INT16_MIN, INT16_MAX);
    int16_t clampedEval = std::clamp(eval, INT16_MIN, INT16_MAX);
    uint8_t clampedDepth = std::min(depth, 255);

    if (node->slot1.zobristKey == b.getZobristKey()) {
        setEntry(&node->slot1, b, clampedScore, move, clampedEval, clampedDepth, nodeType);
    }
    else if (node->slot2.zobristKey == b.getZobristKey()) {
        setEntry(&node->slot2, b, clampedScore, move, clampedEval, clampedDepth, nodeType);
    }
    else {
        // Replacement strategy based on age and depth
        HashEntry *toReplace = &node->slot1;
        uint8_t age1 = node->slot1.ageNodeType >> 2;
        uint8_t age2 = node->slot2.ageNodeType >> 2;
        
        if ((age - age1) < (age - age2) || 
            ((age - age1) == (age - age2) && node->slot1.depth < node->slot2.depth)) {
            toReplace = &node->slot2;
        }
        
        setEntry(toReplace, b, clampedScore, move, clampedEval, clampedDepth, nodeType);
    }
}

private:
void Hash::setEntry(HashEntry* entry, Board &b, int16_t score, Move move, 
                   int16_t eval, uint8_t depth, uint8_t nodeType) {
    entry->zobristKey = b.getZobristKey();
    entry->move = move;
    entry->score = score;
    entry->eval = eval;
    entry->depth = depth;
    entry->ageNodeType = (age << 2) | (nodeType & 0x3);
}
