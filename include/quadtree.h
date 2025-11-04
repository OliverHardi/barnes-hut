#ifndef QUADTREE_H
#define QUADTREE_H

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <cstdint>
#include "utils.h"

struct AABB{
    glm::vec2 center;
    float size;

    AABB(): center(0.0f), size(0.0f) {}
    AABB(glm::vec2& c, float s): center(c), size(s) {}

    static AABB fromPoints( const std::vector<Circle>& circles ){
        glm::vec2 minp(FLT_MAX);
        glm::vec2 maxp(-FLT_MAX);
        for(const auto& c : circles){
            minp = glm::min(minp, c.position);
            maxp = glm::max(maxp, c.position);
        }
        glm::vec2 center = (minp + maxp) * 0.5f;
        glm::vec2 diff = maxp - minp;
        float halfSize = glm::max(diff.x, diff.y) * 0.5f;
        return AABB(center, halfSize);
    }
    AABB intoQuadrant(uint32_t quadrant) const {
        AABB b = *this;
        b.size *= 0.5f;
        b.center.x += ((quadrant & 1) - 0.5f) * b.size * 2.0f;
        b.center.y += (((quadrant >> 1) & 1) - 0.5f) * b.size * 2.0f;
        return b;
    }

    std::array<AABB, 4> subdivide() const {
        return { intoQuadrant(0), intoQuadrant(1), intoQuadrant(2), intoQuadrant(3) };
    }
};

struct Node{
    uint32_t children;
    uint32_t next;
    
    glm::vec2 COM;
    float mass;

    AABB bounds;
    uint32_t start;
    uint32_t end;

    Node()
        : children(0), next(0), COM(0.0f), mass(0.0f), bounds(AABB()), start(0), end(0) {}

    Node(size_t next_, const AABB& q, size_t s, size_t e)
        : children(0), next(next_), COM(0.0f), mass(0.0f), bounds(q), start(s), end(e) {}

    bool isLeaf() const { return children == 0; }
    bool isBranch() const { return children != 0; }
    bool isEmpty() const { return mass == 0.0f; }
};

class Quadtree{

    public:

        uint32_t leaf_capacity;

        Quadtree(float theta = 0.9f, float epsilon = 1e-4f);

        void realloc(uint32_t numCircles);

        void build(std::vector<Circle>& circles);

        glm::vec2 accelerate(const glm::vec2& pos, const std::vector<Circle>& circles, uint32_t index);

        void draw(GLuint program, GLuint VAO);

    private:
        float t_sq;
        float e_sq;

        std::vector<Node> nodes;
        std::vector<uint32_t> parents;

        void clear();

        void subdivide(uint32_t node, std::vector<Circle>& circles, uint32_t start, uint32_t end);

        void propagate();
};

#endif

/*

1
2



*/