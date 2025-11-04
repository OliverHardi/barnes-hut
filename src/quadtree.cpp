#include "quadtree.h"

Quadtree::Quadtree(float theta, float epsilon){
    this->t_sq = theta * theta;;
    this->e_sq = epsilon * epsilon;
    this->leaf_capacity = 8;
}

void Quadtree::clear(){
    this->nodes.clear();
    this->parents.clear();
}

void Quadtree::realloc(uint32_t numCircles){
    this->nodes.resize(numCircles * 4 - 3);
}

void Quadtree::subdivide(uint32_t node, std::vector<Circle>& circles, uint32_t start, uint32_t end){

    // uint32_t total = end - start;
    // if (total <= 1) return; // nothing

    glm::vec2 center = this->nodes[node].bounds.center;
    uint32_t split[5] = {start, 0, 0, 0, end};

    auto mid1 = std::partition(circles.begin() + start, circles.begin() + end,
        [&](const Circle& b) { return b.position.y < center.y; });
    split[2] = start + std::distance(circles.begin() + start, mid1);

    auto mid2 = std::partition(circles.begin() + start, circles.begin() + split[2],
        [&](const Circle& b) { return b.position.x < center.x; });
    split[1] = start + std::distance(circles.begin() + start, mid2);

    auto mid3 = std::partition(circles.begin() + split[2], circles.begin() + end,
        [&](const Circle& b) { return b.position.x < center.x; });
    split[3] = split[2] + std::distance(circles.begin() + split[2], mid3); 

    parents.push_back(node);
    uint32_t children = nodes.size();
    nodes[node].children = children;

    auto quadrants = nodes[node].bounds.subdivide();
    uint32_t nexts[4] = { children + 1, children + 2, children + 3, nodes[node].next };
    for(uint32_t i = 0; i < 4; i++){
        nodes.emplace_back(nexts[i], quadrants[i], split[i], split[i+1]);
    }

}

void Quadtree::propagate(){
    for(auto it = parents.rbegin(); it != parents.rend(); it++){
        uint32_t node = *it;
        uint32_t i = nodes[node].children;
        
        nodes[node].COM = nodes[i].COM + nodes[i + 1].COM + nodes[i + 2].COM + nodes[i + 3].COM;
        nodes[node].mass = nodes[i].mass + nodes[i + 1].mass + nodes[i + 2].mass + nodes[i + 3].mass;
    }
    for(auto& node : nodes){
        node.COM /= std::max(node.mass, 1e-6f);
    }
}

void Quadtree::build(std::vector<Circle>& circles){
    clear();

    AABB b = AABB::fromPoints(circles);
    nodes.emplace_back(0, b, 0, circles.size());

    uint32_t node = 0;
    // uint32_t i = 0;
    while(node < nodes.size()){
        // std::cout << "node: " << node << " with " << nodes[node].end - nodes[node].start << std::endl;
        uint32_t start = nodes[node].start;
        uint32_t end = nodes[node].end;

        if((end-start) > this->leaf_capacity){
            uint32_t prev_size = nodes.size();
            subdivide(node, circles, start, end);
        }else{
            // leaf
            for(uint32_t i = start; i < end; i++){
                nodes[node].COM += circles[i].position * circles[i].mass;
                nodes[node].mass += circles[i].mass;
            }
        }
        node++;
    }
    propagate();
}

glm::vec2 Quadtree::accelerate(const glm::vec2& pos, const std::vector<Circle>& circles, uint32_t index){
    glm::vec2 acc = glm::vec2(0.0f);
    uint32_t node = 0;

    while(true){
        Node& n = nodes[node];

        glm::vec2 delta = n.COM - pos;
        float d_sq = glm::dot(delta, delta);
        if(n.bounds.size * n.bounds.size < d_sq * this->t_sq){
            if(d_sq < 1e-10f){ continue; } // avoid singularity 
            float invDist = fast_invsqrt(d_sq + this->e_sq); // 1/sqrt(dist + softening)
            float invDist3 = invDist * invDist * invDist;
            acc += G * n.mass * invDist3 * delta;
            if(n.next == 0) { break; }
            node = n.next;
        }else if(n.isLeaf()){
            for(uint32_t i = n.start; i < n.end; i++){
                if(i == index) { continue; }
                glm::vec2 delta_i = circles[i].position - pos;
                float dist2 = glm::dot(delta_i, delta_i);
                if(dist2 < 1e-10f) { continue; } // avoid singularity
                float invDist = fast_invsqrt(dist2 + this->e_sq);
                float invDist3 = invDist * invDist * invDist;
                acc += G * circles[i].mass * invDist3 * delta_i;

            }
            if(n.next == 0) { break; }
            node = n.next;
        }else{
            node = n.children;
        }
    }
    return acc;
}

void Quadtree::draw(GLuint program, GLuint VAO){
    for(auto& node : nodes){
        if(node.isLeaf() && !node.isEmpty() || true){
            // draw box
            glm::vec2 minCorner = node.bounds.center - node.bounds.size;
            glm::vec2 maxCorner = node.bounds.center + node.bounds.size;
            // std::cout << "box: " << minCorner.x << ", " << minCorner.y << " - " << maxCorner.x << ", " << maxCorner.y << std::endl;

            glUniform2fv(glGetUniformLocation(program, "uMin"), 1, &minCorner[0]);
            glUniform2fv(glGetUniformLocation(program, "uMax"), 1, &maxCorner[0]);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }
    }
}

