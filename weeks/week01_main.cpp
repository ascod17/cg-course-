#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

const int WIDTH = 1280;
const int HEIGHT = 720;

struct Color {
    float r, g, b;
};

Color mix(Color a, Color b, float k) {
    return {
        a.r + (b.r - a.r) * k,
        a.g + (b.g - a.g) * k,
        a.b + (b.b - a.b) * k
    };
}

Color backgroundColor(float t) {
    const Color blue = {0.05f, 0.10f, 0.90f};
    const Color violet = {0.55f, 0.15f, 0.85f};
    const Color cyan = {0.35f, 0.80f, 1.00f};

    float phase = std::fmod(t * 0.4f, 3.0f);
    int seg = (int)phase;
    float k = phase - seg;
    k = k * k * (3.0f - 2.0f * k);

    if (seg == 0) return mix(blue, violet, k);
    if (seg == 1) return mix(violet, cyan, k);
    return mix(cyan, blue, k);
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
    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "Компьютерлік графика",
        nullptr,
        nullptr
    );

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

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        if (spaceHeld) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            Color c = backgroundColor((float)glfwGetTime());
            glClearColor(c.r, c.g, c.b, 1.0f);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
