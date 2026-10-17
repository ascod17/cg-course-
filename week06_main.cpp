#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH  = 1280;
const int HEIGHT = 720;

const float PI           = 3.14159265f;
const float GROUND_TOP   = -0.7f;
const float BALL_RADIUS  = 0.1f;
const float SQUARE_HALF  = 0.12f;
const float JUMP_HEIGHT  = 0.9f;
const float BOUNCE_TIME  = 1.2f;
const int   SEGMENTS     = 48;
const float PASS_TIME    = 3.0f * BOUNCE_TIME;
const float GRAVITY      = 8.0f * JUMP_HEIGHT / (BOUNCE_TIME * BOUNCE_TIME);
const float JUMP_SPEED   = 4.0f * JUMP_HEIGHT / BOUNCE_TIME;

float aspect = (float)WIDTH / HEIGHT;

struct Mesh {
    unsigned int vao = 0, vbo = 0, ebo = 0;
    int count = 0;
};

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform vec2 uOffset;
uniform vec2 uScale;
uniform float uAspect;
void main() {
    vec2 p = aPos * uScale + uOffset;
    p.x /= uAspect;
    gl_Position = vec4(p, 0.0, 1.0);
}
)";

const char* fragmentSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() { FragColor = vec4(uColor, 1.0); }
)";

unsigned int compileShader(GLenum type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cerr << "Шейдер қатесі:\n" << log << "\n";
    }
    return s;
}

Mesh makeMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    Mesh m;
    m.count = (int)indices.size();
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return m;
}

Mesh makeQuad() {
    std::vector<float> v = {
         1.0f,  1.0f,
         1.0f, -1.0f,
        -1.0f, -1.0f,
        -1.0f,  1.0f
    };
    std::vector<unsigned int> i = { 0, 1, 3, 1, 2, 3 };
    return makeMesh(v, i);
}

Mesh makeCircle() {
    std::vector<float> v = { 0.0f, 0.0f };
    std::vector<unsigned int> idx;
    for (int k = 0; k < SEGMENTS; ++k) {
        float a = 2.0f * PI * k / SEGMENTS;
        v.push_back(std::cos(a));
        v.push_back(std::sin(a));
    }
    for (int k = 0; k < SEGMENTS; ++k) {
        idx.push_back(0);
        idx.push_back(1 + k);
        idx.push_back(1 + (k + 1) % SEGMENTS);
    }
    return makeMesh(v, idx);
}

void freeMesh(Mesh& m) {
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    glDeleteBuffers(1, &m.ebo);
}

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
    if (height > 0) aspect = (float)width / height;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    if (!glfwInit()) { std::cerr << "GLFW іске қосылмады\n"; return -1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Компьютерлік графика", nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    Mesh quad   = makeQuad();
    Mesh circle = makeCircle();

    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale  = glGetUniformLocation(shader, "uScale");
    int locAspect = glGetUniformLocation(shader, "uAspect");
    int locColor  = glGetUniformLocation(shader, "uColor");

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        float t = (float)glfwGetTime();

        float tau = std::fmod(t, BOUNCE_TIME);
        float ballY = GROUND_TOP + BALL_RADIUS + JUMP_SPEED * tau - 0.5f * GRAVITY * tau * tau;

        float reach = std::max(aspect + SQUARE_HALF, 2.2f);
        float phase = std::fmod(t, PASS_TIME);
        float squareX = -reach + 2.0f * reach * phase / PASS_TIME;
        float squareY = GROUND_TOP + SQUARE_HALF;

        glClearColor(0.08f, 0.10f, 0.20f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
        glUniform1f(locAspect, aspect);

        glBindVertexArray(quad.vao);
        glUniform3f(locColor, 0.25f, 0.30f, 0.38f);
        glUniform2f(locScale, 3.0f, 0.3f);
        glUniform2f(locOffset, 0.0f, GROUND_TOP - 0.3f);
        glDrawElements(GL_TRIANGLES, quad.count, GL_UNSIGNED_INT, 0);

        glUniform3f(locColor, 0.20f, 0.85f, 0.95f);
        glUniform2f(locScale, SQUARE_HALF, SQUARE_HALF);
        glUniform2f(locOffset, squareX, squareY);
        glDrawElements(GL_TRIANGLES, quad.count, GL_UNSIGNED_INT, 0);

        glBindVertexArray(circle.vao);
        glUniform3f(locColor, 1.0f, 0.65f, 0.1f);
        glUniform2f(locScale, BALL_RADIUS, BALL_RADIUS);
        glUniform2f(locOffset, 0.0f, ballY);
        glDrawElements(GL_TRIANGLES, circle.count, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    freeMesh(quad);
    freeMesh(circle);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
