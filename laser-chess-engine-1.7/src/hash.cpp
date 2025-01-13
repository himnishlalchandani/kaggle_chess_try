/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An
    Laser is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    Laser is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with Laser.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstring>
#include "hash.h"

Hash::Hash(uint64_t MB) {
    init(MB);  // Changed to use the passed MB value instead of hardcoded 1
}

Hash::~Hash() {
    free(table);
}

void Hash::init(uint64_t MB) {
    // Convert to bytes (5MB = 5,242,880 bytes)
    uint64_t bytes = MB << 20;
    
    // Calculate how many array slots we can use
    // Each HashNode is 32 bytes (2 * 16 bytes for each HashEntry)
    uint64_t maxSize = bytes / sizeof(HashNode);
    
    // Find largest power of 2 that fits
    size = 1;
    while (size <= maxSize)
        size <<= 1;
    size >>= 1;
    
    // Allocate aligned memory for better cache performance
    #ifdef _MSC_VER
        table = (HashNode*)_aligned_malloc(size * sizeof(HashNode), 64);
    #else
        table = (HashNode*)aligned_alloc(64, size * sizeof(HashNode));
    #endif
    
    clear();
}

void Hash::add(Board &b, int score, Move move, int eval, int depth, uint8_t nodeType) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h & (size-1);
    HashNode* node = table + index;

    // A more recent update to the same position should always be chosen
    if (node->slot1.zobristKey == h) {
        node->slot1.setEntry(b, score, move, eval, depth, nodeType, age);
        return;
    }
    if (node->slot2.zobristKey == h) {
        node->slot2.setEntry(b, score, move, eval, depth, nodeType, age);
        return;
    }

    // Replacement strategy
    HashEntry* toReplace = &node->slot1;
    int score1 = 16 * ((int)((uint8_t)(age - (node->slot1.ageNodeType >> 2))))
                 + depth - node->slot1.depth;
    int score2 = 16 * ((int)((uint8_t)(age - (node->slot2.ageNodeType >> 2))))
                 + depth - node->slot2.depth;

    if (score1 < score2)
        toReplace = &node->slot2;

    // The node must be from a newer search space or a sufficiently high depth
    if (score1 >= -2 || score2 >= -2)
        toReplace->setEntry(b, score, move, eval, depth, nodeType, age);
}

HashEntry* Hash::get(Board &b) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h & (size-1);
    HashNode* node = table + index;

    if (node->slot1.zobristKey == h)
        return &node->slot1;
    if (node->slot2.zobristKey == h)
        return &node->slot2;
    return nullptr;
}

uint64_t Hash::getSize() const {
    return (2 * size);  // Return total number of entries (2 per node)
}

void Hash::setSize(uint64_t MB) {
    #ifdef _MSC_VER
        _aligned_free(table);
    #else
        free(table);
    #endif
    init(MB);
}

void Hash::incrementAge() {
    age++;
}

void Hash::clear() {
    std::memset(table, 0, size * sizeof(HashNode));
    age = 0;
}

int Hash::estimateHashfull() const {
    int used = 0;
    // Sample first 500 nodes for hash fullness estimate
    for (int i = 0; i < 500; i++) {
        used += ((table + i)->slot1.ageNodeType >> 2) == age;
        used += ((table + i)->slot2.ageNodeType >> 2) == age;
    }
    return used;
}
