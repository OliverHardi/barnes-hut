#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <chrono>
#include <cmath>

#include <sstream>
#include <iomanip>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


#include "utils.h"
#include "spatialHash.h"
#include "quadtree.h"
#include "threadPool.h"


const int circleCount = 45000;
const float circleRadius = 0.01f;

const float deltaTime = 0.25f;


std::vector<Circle> circles;


void updateCircles(Quadtree& quadtree, ThreadPool& pool){
    
    // for(uint32_t i = 0; i < circles.size(); i++){
    //     circles[i].velocity += quadtree.accelerate(circles[i].position, circles);
    //     // accelerate towards center
    //     circles[i].velocity += G * CENTER_MASS / glm::dot(circles[i].position, circles[i].position) * normalize(-circles[i].position);
    // }
    pool.parallel_for(0, circles.size(), [&](size_t i, size_t tid){
        circles[i].velocity += quadtree.accelerate(circles[i].position, circles, i) * deltaTime;
        // circles[i].velocity += G * CENTER_MASS / glm::dot(circles[i].position, circles[i].position) * normalize(-circles[i].position) * deltaTime;
    });
    

    // for(size_t i = 0; i < circles.size(); i++){
    //     circles[i].position += circles[i].velocity * deltaTime;
    // }
    pool.parallel_for(0, circles.size(), [&](size_t i, size_t tid){
        circles[i].position += circles[i].velocity * deltaTime;
        if(glm::dot(circles[i].position, circles[i].position) > 5000.0f){
            // circles[i].position = -390.0f * normalize(circles[i].position);
            // circles[i].velocity *= glm::clamp(circles[i].velocity*0.5f, -1.0f, 1.0f);
            circles[i].position = 10.0f * randomPosInCircle();
            // circles[i].velocity = calcTangential(circles[i].position);
            circles[i].velocity = glm::vec3(0.0f);
        }
    });
}

// generates contact pairs
inline bool narrowphase(uint32_t i, uint32_t j, Contact& contact){
    glm::vec2 delta = circles[i].position - circles[j].position;
    float dist2 = glm::dot(delta, delta);
    float radii = 2.0f * circleRadius;
    if( dist2 < radii * radii && dist2 > 1e-10f){
        float dist = sqrt(dist2);

        contact.i = i;
        contact.j = j;
        contact.delta = delta * fast_invsqrt(dist2);
        contact.overlap = radii - dist;

        glm::vec2 relativeVelocity = circles[i].velocity - circles[j].velocity;
        contact.impulse = glm::dot(relativeVelocity, delta);

        return true;
    }
    return false;
}

void collideCircles(SpatialHash& hashTable, ThreadPool& pool){

    std::vector<std::vector<Contact>> threadContacts(pool.size());

    pool.parallel_for(0, circles.size(), [&](size_t i, size_t tid){
        auto& localContacts = threadContacts[tid];
        glm::ivec2 cell = hashTable.getCellPos(circles[i].position);
        for(int x = -1; x <= 1; x++){
            for(int y = -1; y <= 1; y++){
                glm::ivec2 neighborCell = cell + glm::ivec2(x, y);
                auto [start, end] = hashTable.query(neighborCell);

                for(uint32_t j = start; j < end; j++){
                    uint32_t index = hashTable.getEntry(j);
                    if(i >= index) { continue; }    // avoid double check and self check

                    glm::ivec2 entryCell = hashTable.getCellPos(circles[index].position);
                    if (entryCell != neighborCell) { continue; }
                    Contact c;
                    if( narrowphase(i, index, c) ){
                        localContacts.emplace_back(c);
                    }
                }

            }
        }
    });

    for (auto& buf : threadContacts) {
        for (auto& c : buf) {
            // move circles apart
            circles[c.i].position += c.delta * c.overlap * 0.5f;
            circles[c.j].position -= c.delta * c.overlap * 0.5f;

            // adjust velocities
            glm::vec2 dv = c.delta * c.impulse * 0.5f;
            circles[c.i].velocity -= dv;
            circles[c.j].velocity += dv;
        }
    }



}


int main(){

    std::cout << "opening window..." << std::endl;
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "nbody sim", nullptr, nullptr);

    if(!window){
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    for(size_t i = 0; i < circleCount; i++){
        circles.emplace_back(
            10.0f * randomPosInCircle(),
            256.0f
        );
    }
    // createPlanets(200, circles);

    ThreadPool pool;    // create threadpool

    SpatialHash hashTable; // create hash table for collisions
    
    hashTable.setCellSize(circleRadius * 2.0f);
    hashTable.resizeTables(circles.size());

    Quadtree quadtree; // create barnes-hut quadtree
    
    quadtree.realloc(circles.size());    
    
    std::string vertexShaderCode = loadShaderSource("shaders/v_circle.glsl");
    std::string fragmentShaderCode = loadShaderSource("shaders/f_circle.glsl");
    GLuint program = createProgram(
        vertexShaderCode.c_str(),
        fragmentShaderCode.c_str()
    );

    glUseProgram(program);

    GLint radiusLoc = glGetUniformLocation(program, "uRadius");
    glUniform1f(radiusLoc, circleRadius);

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, circles.size() * sizeof(Circle), circles.data(), GL_DYNAMIC_DRAW);

    // Position attribute (offset 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Circle), (void*)offsetof(Circle, position));
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 1);

    glBindVertexArray(0); // unbind

    // debug line program for drawing quadtree

    std::string debugVertCode = loadShaderSource("shaders/v_line.glsl");
    std::string debugFragCode = loadShaderSource("shaders/f_line.glsl");
    GLuint lineProgram = createProgram(
        debugVertCode.c_str(),
        debugFragCode.c_str()
    );

    GLuint lineVAO, lineVBO;
    {
        glm::vec2 boxVerts[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        glGenVertexArrays(1, &lineVAO);
        glBindVertexArray(lineVAO);

        glGenBuffers(1, &lineVBO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(boxVerts), boxVerts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }
    

    while(!glfwWindowShouldClose(window)){          // main loop
        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        using clock = std::chrono::high_resolution_clock;
        // update
        auto t0 = clock::now();
        quadtree.build(circles);

        auto t1 = clock::now();
        updateCircles(quadtree, pool);
        auto t2 = clock::now();

        hashTable.rebuildTables(circles);
        auto t3 = clock::now();

        collideCircles(hashTable, pool);

        auto t4 = clock::now();

        // draw boxes: (uncomment to debug/visualize quadtree)
        glUseProgram(lineProgram);
        glBindVertexArray(lineVAO);

        glUniform2f(glGetUniformLocation(lineProgram, "uCameraPos"), cameraPos.x, cameraPos.y);
        glUniform1f(glGetUniformLocation(lineProgram, "uZoom"), zoom);
        quadtree.draw(lineProgram, lineVAO);

        // draw circles
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        Circle* ptr = (Circle*)glMapBufferRange(
            GL_ARRAY_BUFFER,
            0,
            circles.size() * sizeof(Circle),
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
        );
        memcpy(ptr, circles.data(), circles.size() * sizeof(Circle));
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glUseProgram(program);
        glUniform2f(glGetUniformLocation(program, "uCameraPos"), cameraPos.x, cameraPos.y);
        glUniform1f(glGetUniformLocation(program, "uZoom"), zoom);
        glBindVertexArray(VAO);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, circles.size());


        

        auto t5 = clock::now();
        
        // display benchmarking data
        float buildMS  = std::chrono::duration<double, std::milli>(t1 - t0).count();
        float updateMS  = std::chrono::duration<double, std::milli>(t2 - t1).count();
        float tableMS   = std::chrono::duration<double, std::milli>(t3 - t2).count();
        float collideMS = std::chrono::duration<double, std::milli>(t4 - t3).count();
        float drawMS   = std::chrono::duration<double, std::milli>(t5 - t4).count();
        float totalMS   = std::chrono::duration<double, std::milli>(t5 - t0).count();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "build: " << buildMS 
            << " ms | update circles: " << updateMS 
            << " ms | build table: " << tableMS 
            << " ms | collide: " << collideMS 
            << " ms | draw: " << drawMS
            << " ms | total: " << totalMS << " ms";
        std::string title = oss.str();
        glfwSetWindowTitle(window, title.c_str());


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}