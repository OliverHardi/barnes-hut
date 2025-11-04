#ifndef UTILS_H
#define UTILS_H

// #define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h> 
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <cmath>
#include <arm_neon.h>

const float G  = 6.6743e-11f;

const float CENTER_MASS = 1e7f;
const float SOFTENING_FACTOR = 1e-4f;



glm::vec2 calcTangential(const glm::vec2& p);

glm::vec2 randomPosInCircle();

inline float fast_invsqrt(float x){ // hardware accelerated 1/sqrt(x)
    float32x2_t v = vdup_n_f32(x);
    float32x2_t y = vrsqrte_f32(v);               // approximate 1/sqrt
    y = vmul_f32(y, vrsqrts_f32(vmul_f32(y, y), v)); // 1 iteration to improve precision
    return vget_lane_f32(y, 0);
}

struct Circle{
    glm::vec2 position;
    glm::vec2 velocity;
    float mass;

    Circle(glm::vec2 pos = glm::vec2(0.0f), float m = 1.0f) 
        : position(pos), velocity(calcTangential(pos)), mass(m) {}
    Circle(glm::vec2 pos, glm::vec2 vel, float m = 1.0f)
        : position(pos), velocity(vel), mass(m) {}
};

struct Contact{
    uint32_t i, j;
    glm::vec2 delta;
    float overlap;
    float impulse;
};

glm::vec2 goldenRatioPoint(float i);
void planetAt(glm::vec2 p, size_t size, std::vector<Circle>& circles);
void createPlanets(size_t n, std::vector<Circle>& circles);

GLuint compileShader(GLenum type, const char* src);

GLuint createProgram(const char* vsSrc, const char* fsSrc);

std::string loadShaderSource(const char* path);

float getRandom();

float easeCurve(float t);

extern glm::vec2 cameraPos;
extern float zoom;

extern double lastX, lastY;

// mouse down
extern bool mouseDown;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
// camera pos
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

// zoom
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);



#endif