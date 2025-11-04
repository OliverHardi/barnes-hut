#ifndef SPATIALHASH_H
#define SPATIALHASH_H

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <cstdint>

#include "utils.h"

/*
have a table size (can be the number of cirles or larger, to decrease hash collisions)

create an array of uint (0) of size table size
00000
feed the position of each circle into a hash and increment the value in the array
01020
iterate through the array and replace each value with the partial sum up to that point
01133

create a new blank array of size number of circles
for each particle:
    hash the position to get the index
    deiterate the value in the original array at that index
    then use the value to index into the new array and store the pointer to the particle

usage:
hash position to get index:
get value in array1 at that index and the next value as well
*/

class SpatialHash {
    public:
        SpatialHash();

        uint32_t tableSize;


        void resizeTables(uint32_t numCircles);
        
        void rebuildTables(const std::vector<Circle>& circles);

        void setCellSize(float cellSize);
        
        glm::ivec2 getCellPos(glm::vec2 p);

        std::pair<uint32_t, uint32_t> query(glm::ivec2 p);

        inline uint32_t getEntry(uint32_t i){
            return this->entries[i];
        }

    private:

        uint32_t hash(glm::ivec2 v);

        std::vector<uint32_t> table;
        std::vector<uint32_t> entries;

        float cellSize;
        float invCellSize;

};

#endif