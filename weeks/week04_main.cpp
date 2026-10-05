#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH  = 1280;
const int HEIGHT = 720;

struct Color { float r, g, b; };

Color mix(Color a, Color b, float k) {
    return { a.r + (b.r - a.r) * k, a.g + (b.g - a.g) * k, a.b + (b.b - a.b) * k };
}

Color backgroundColor(float t) {
    const Color blue   = {0.05f, 0.10f, 0.90f};
    const Color violet = {0.55f, 0.15f, 0.85f};
    const Color cyan   = {0.35f, 0.80f, 1.00f};

    float phase = std::fmod(t * 0.4f, 3.0f);
    int   seg   = (int)phase;
    float k     = phase - seg;
    k = k * k * (3.0f - 2.0f * k);

    if (seg == 0) return mix(blue,   violet, k);
    if (seg == 1) return mix(violet, cyan,   k);
    return               mix(cyan,   blue,   k);
}

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec2 uOffset;
uniform float uScale;
void main() { gl_Position = vec4(aPos.xy * uScale + uOffset, aPos.z, 1.0); }
)";

const char* fragmentSrc = R"(
#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(1.0, 0.65, 0.1, 1.0); }
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

std::vector<float> makePyramid(int n) {
    std::vector<float> v;
    const float half = 0.4f;
    const float top = 0.4f, bottom = -0.4f;
    const float rowH = (top - bottom) / n;
    const float cellW = 2.0f * half / n;
    const float shrink = 0.92f;

    for (int r = 0; r < n; ++r) {
        float yTop = top - r * rowH;
        float yBot = yTop - rowH;
        float fb = (float)(r + 1) / n;
        float xLeftBottom = -half * fb;
        for (int j = 0; j <= r; ++j) {
            float xl = xLeftBottom + j * cellW;
            float xr = xl + cellW;
            float xa = (xl + xr) * 0.5f;

            float cx = (xl + xr + xa) / 3.0f;
            float cy = (yBot + yBot + yTop) / 3.0f;
            auto sx = [&](float x) { return cx + (x - cx) * shrink; };
            auto sy = [&](float y) { return cy + (y - cy) * shrink; };

            float tri[] = { sx(xl), sy(yBot), 0.0f,
                            sx(xr), sy(yBot), 0.0f,
                            sx(xa), sy(yTop), 0.0f };
            v.insert(v.end(), tri, tri + 9);
        }
    }
    return v;
}

int uploadPyramid(unsigned int vbo, int n) {
    std::vector<float> v = makePyramid(n);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_DYNAMIC_DRAW);
    return (int)v.size() / 3;
}

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

bool spaceHeld = false;
float speed = 1.5f;
int  rowsN = 5;
bool rowsChanged = false;

const int MAX_ROWS = 30;

void onKey(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    if (key == GLFW_KEY_UP   && rowsN < MAX_ROWS) { ++rowsN; rowsChanged = true; }
    if (key == GLFW_KEY_DOWN && rowsN > 1)        { --rowsN; rowsChanged = true; }
}

void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    spaceHeld = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) speed += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) speed -= 2.0f * dt;
}

int main() {

    std::cout << "Пирамида қатарының саны n (1.." << MAX_ROWS << "), мысалы 5: ";
    int input = 0;
    if (std::cin >> input && input >= 1 && input <= MAX_ROWS) {
        rowsN = input;
    } else {
        std::cout << "Дұрыс сан емес, n = 5 алынды.\n";
        rowsN = 5;
    }

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
    glfwSetKeyCallback(window, onKey);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    int vertexCount = uploadPyramid(vbo, rowsN);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

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
    float lastFrame = (float)glfwGetTime();
    float angle = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - lastFrame;
        lastFrame = now;
        angle += speed * dt;

        processInput(window, dt);

        if (rowsChanged) {
            glBindVertexArray(vao);
            vertexCount = uploadPyramid(vbo, rowsN);
            rowsChanged = false;
            std::cout << "n = " << rowsN << "\n";
        }

        if (spaceHeld) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            Color c = backgroundColor((float)glfwGetTime());
            glClearColor(c.r, c.g, c.b, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
        glUniform2f(locOffset, std::cos(angle) * 0.4f, std::sin(angle) * 0.4f);
        glUniform1f(locScale, 0.75f + 0.25f * std::sin(now * 2.0f));
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
