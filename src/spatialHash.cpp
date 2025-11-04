#include "spatialHash.h"

SpatialHash::SpatialHash(){
    this->tableSize = 0;
    
    this->table.resize(0);
    this->entries.resize(0);
}

uint32_t SpatialHash::hash(glm::ivec2 p){
    const uint32_t a = 73856093u;
    const uint32_t b = 19349663u;

    uint32_t rawHash = (p.x * a) ^ (p.y * b);

    return rawHash % this->tableSize;
    
    // alternative hash
    /*
    const uint32_t a = 73856093u;
    const uint32_t b = 19349663u;
    uint32_t ux = static_cast<uint32_t>(p.x);
    uint32_t uy = static_cast<uint32_t>(p.y);
    uint32_t raw = (ux * a) ^ (uy * b);

    return raw % this->tableSize;*/
}

void SpatialHash::resizeTables(uint32_t numCircles){
    this->tableSize = numCircles * 1;

    this->table.resize(this->tableSize + 1);
    this->entries.resize(numCircles);
}

void SpatialHash::rebuildTables(const std::vector<Circle>& circles){
    // clear table
    memset(this->table.data(), 0, this->table.size() * sizeof(uint32_t));

    // iterate through each circle's position and increment the hash table entry
    for(size_t i = 0; i < circles.size(); i++){
        glm::ivec2 cell = this->getCellPos(circles[i].position);
        uint32_t index = this->hash(cell);
        this->table[index]++;
    }
    // partial sum
    uint32_t sum = 0;
    for(size_t i = 0; i < this->table.size(); i++){
        uint32_t value = this->table[i];
        sum += value;
        this->table[i] = sum;
    }

    // populate entries
    for(size_t i = 0; i < circles.size(); i++){
        glm::ivec2 cell = this->getCellPos(circles[i].position);
        uint32_t index = this->hash(cell);
        this->table[index]--;
        this->entries[this->table[index]] = i;
    }

}

std::pair<uint32_t, uint32_t> SpatialHash::query(glm::ivec2 p){
    uint32_t i = this->hash(p);
    uint32_t start = this->table[i];
    uint32_t end = this->table[i + 1];
    return std::make_pair(start, end);
}


void SpatialHash::setCellSize(float cellSize){
    this->cellSize = cellSize;
    this->invCellSize = 1.0f / cellSize;
}

// floating point position -> integer cell coordinates
glm::ivec2 SpatialHash::getCellPos(glm::vec2 p){
    return glm::ivec2( glm::floor(p * this->invCellSize) );
}
