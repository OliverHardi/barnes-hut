#include "utils.h"

glm::vec2 goldenRatioPoint(float i){
    static float phi = (1.0f + sqrt(5.0f)) * M_PI;
    return glm::vec2( cos(phi * i), sin(phi * i) );
}

void planetAt(glm::vec2 p, size_t size, std::vector<Circle>& circles){
    glm::vec2 tangent = calcTangential(p);
    for(size_t i = 0; i < size; i++){
        float r = 0.011f * sqrt(float(i));
        // float r = 0.001f * float(i);
        glm::vec2 offset = goldenRatioPoint(float(i)) * r;
        // circles.emplace_back(p + offset, 1.0f);
        circles.emplace_back(p + offset, tangent, 1.0f);
    }
}

void createPlanets(size_t n, std::vector<Circle>& circles){
    // for(size_t i = 0; i < n; i++){
    //     float r = getRandom() * 7.0f + 1.0f;
    //     float theta = getRandom() * 2.0f * M_PI;
    //     glm::vec2 p = glm::vec2( cos(theta) * r, sin(theta) * r );
    //     // circles.emplace_back(p, 100.0f);
    //     planetAt(p, 100, circles);
    // }
    // planetAt(glm::vec2(-0.5, 0.0f), 10, circles);
    // planetAt(glm::vec2(0.5, 0.0f), 10, circles);

    // circles.emplace_back(glm::vec2(-0.5f, 0.0f), glm::vec2(0.0f, 1.0f), 1.0f);
    // for(size_t i = 0; i < 10000; i++){
    //     circles.emplace_back(glm::vec2(-10.0f, 0.0f) + 10.0f * randomPosInCircle(), glm::vec2(0.0f), 1.0f);
    // }
    // circles.emplace_back(glm::vec2(0.5f, 0.0f), glm::vec2(0.0f, -1.0f), 1.0f);
    // for(size_t i = 0; i < 10000; i++){
    //         circles.emplace_back(glm::vec2(10.0f, 0.0f) + 10.0f * randomPosInCircle(), glm::vec2(0.0f), 1.0f);
    //     }
    }

glm::vec2 calcTangential(const glm::vec2& p) {
    float r = glm::length(p);
    glm::vec2 dir = p/r;
    float mag = sqrt(G * CENTER_MASS / r);

    return mag * glm::vec2(-dir.y, dir.x);
}

glm::vec2 randomPosInCircle(){
    float r = easeCurve(getRandom());
    float theta = getRandom() * 2.0f * 3.14159265359f;
    return glm::vec2(r * cos(theta), r * sin(theta));
}



GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader error:\n" << log << std::endl;
    }
    return shader;
}

GLuint createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

std::string loadShaderSource(const char* path){
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

float getRandom(){
    return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}

float easeCurve(float t){
    t = sqrt(t); // uniform distribution
    // float k = 2.0f - 4.0f * t;
    // return exp(-k * k);
    #define N 10.0f
    float mult = std::atan(-0.5f * N) * -2.0f;
    return tan(mult * (t - 0.5f)) / N + 0.5f;
}

glm::vec2 cameraPos(0.0f, 0.0f);
float zoom = 1.0f;
double lastX = 0.0, lastY = 0.0;
bool mouseDown = false;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            glfwGetCursorPos(window, &lastX, &lastY);
            mouseDown = true;
        } else if (action == GLFW_RELEASE) {
            mouseDown = false;
        }
    }
}
// camera pos
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (mouseDown) {
        double dx = xpos - lastX;
        double dy = ypos - lastY;

        // Adjust sensitivity
        float sensitivity = 1.0f/400.0f;
        cameraPos.x -= dx * sensitivity / zoom;
        cameraPos.y += dy * sensitivity / zoom;

        lastX = xpos;
        lastY = ypos;
    }
}

// zoom
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    float zoomSpeed = 0.025f;
    zoom += yoffset * zoomSpeed * zoom;

    if (zoom < 0.05f) zoom = 0.05f; // prevent negative or zero zoom
}
