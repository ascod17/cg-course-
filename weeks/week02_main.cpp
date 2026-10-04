// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — 2-АПТА: үшбұрыш (VBO + VAO)
//  Фон 1-аптадағыдай ауысып тұрады. Төртбұрыштарды диагональ бойынша
//  қиып алған сары-қызғылт үшбұрыштар торы. Пробел — ақ фон. Esc — шығу.
// =====================================================================
#include <glad/gl.h>      // glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH  = 1280;
const int HEIGHT = 720;

struct Color { float r, g, b; };

// a мен b арасындағы түсті араластыру (k = 0..1)
Color mix(Color a, Color b, float k) {
    return { a.r + (b.r - a.r) * k, a.g + (b.g - a.g) * k, a.b + (b.b - a.b) * k };
}

// Уақытқа байланысты көк -> фиолет -> голубой -> көк циклі
Color backgroundColor(float t) {
    const Color blue   = {0.05f, 0.10f, 0.90f};
    const Color violet = {0.55f, 0.15f, 0.85f};
    const Color cyan   = {0.35f, 0.80f, 1.00f};

    float phase = std::fmod(t * 0.4f, 3.0f);       // 0..3, әр 1 бірлік = бір түс
    int   seg   = (int)phase;                      // 0, 1 немесе 2
    float k     = phase - seg;                     // 0..1 — сегмент ішіндегі орны
    k = k * k * (3.0f - 2.0f * k);                 // жұмсақ ауысу (smoothstep)

    if (seg == 0) return mix(blue,   violet, k);
    if (seg == 1) return mix(violet, cyan,   k);
    return               mix(cyan,   blue,   k);
}


// ---------------------------------------------------------------------
//  Шейдерлер (әзірге ең қарапайым; 3-аптада жақсартамыз)
// ---------------------------------------------------------------------
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() { gl_Position = vec4(aPos, 1.0); }
)";

const char* fragmentSrc = R"(
#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(1.0, 0.65, 0.1, 1.0); }   // сары-қызғылт
)";

// Шейдерді компиляциялап, қатесі болса консольге шығарады
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

// Тор: cols x rows ұяшық. Әр ұяшық — төртбұрыш, оны диагональмен қиып,
// жоғарғы-сол жақ үшбұрышын аламыз (суреттегідей).
std::vector<float> makeGrid(int cols, int rows) {
    std::vector<float> v;
    const float left = -0.85f, right = 0.85f, top = 0.85f, bottom = -0.85f;
    const float cw = (right - left) / cols;
    const float ch = (top - bottom) / rows;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float x0 = left + c * cw, x1 = x0 + cw;
            float y1 = top - r * ch,  y0 = y1 - ch;
            // жоғарғы-сол, төменгі-сол, жоғарғы-оң
            float tri[] = { x0, y1, 0.0f,   x0, y0, 0.0f,   x1, y1, 0.0f };
            v.insert(v.end(), tri, tri + 9);
        }
    }
    return v;
}

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

bool spaceHeld = false;

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    spaceHeld = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
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

    // --- 2-АПТА: вершина деректері мен буферлер ---
    const int COLS = 3, ROWS = 4;
    std::vector<float> vertices = makeGrid(COLS, ROWS);
    int vertexCount = (int)vertices.size() / 3;

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- шейдер программасы ---
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        if (spaceHeld) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);              // толық ақ
        } else {
            Color c = backgroundColor((float)glfwGetTime());
            glClearColor(c.r, c.g, c.b, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
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
