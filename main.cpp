// «Көлеңке» — қараңғыдағы платформер. C++17, OpenGL 3.3 core, GLFW + GLEW + GLM
// Түстер: тек ақ, қара, сұр
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

// ---------------- Деректер (1–2-апта) ----------------
struct Rect  { float x, y, w, h; };
struct Theme { glm::vec3 bg, plat, edge, spike, player, exit, drifter; };
// Mover: синус бойынша тербелетін объект (платформа немесе «елес»)
struct Mover { float x0, y0, dx, dy, speed, w, h; float x = 0, y = 0, px = 0, py = 0; };
struct Spike { float x, y; int n; };             // n — қатардағы тікенек саны
struct Level {
    const char* name; float width, radius; glm::vec2 start; Rect exit;
    std::vector<Rect>  plats;
    std::vector<Spike> spikes;
    std::vector<Mover> movers;                   // жылжымалы платформалар
    std::vector<Mover> drifters;                 // елестер (w = радиус)
};
struct Player {
    float x = 0, y = 0, vx = 0, vy = 0, w = 22, h = 22;
    bool ground = false; float coyote = 0, jumpBuf = 0; int dir = 1, onMover = -1;
};

const float W = 960, H = 540, STEP = 1.0f / 60.0f;
const float GRAV = 1800, JUMP = 620, SPEED = 260;
const Theme T{ {0.02f, 0.02f, 0.02f}, {0.35f, 0.35f, 0.35f}, {0.75f, 0.75f, 0.75f},
               {0.95f, 0.95f, 0.95f}, {1, 1, 1}, {0.85f, 0.85f, 0.85f}, {0.6f, 0.6f, 0.6f} };

const std::vector<Level> LEVELS = {
    { "Алғашқы қадам", 1600, 230, {60, 40}, {1520, 40, 30, 50},
      { {0, 0, 500, 40}, {620, 0, 400, 40}, {1120, 0, 480, 40}, {300, 120, 120, 20} },
      { {800, 40, 3} }, {}, {} },
    { "Тікенектер", 1900, 200, {60, 40}, {1820, 40, 30, 50},
      { {0, 0, 1900, 40}, {680, 100, 140, 20}, {1130, 100, 160, 20}, {1500, 100, 100, 20} },
      { {350, 40, 4}, {700, 40, 5}, {1150, 40, 6}, {1620, 40, 4} }, {}, {} },
    { "Қозғалыс", 2000, 180, {60, 40}, {1920, 40, 30, 50},
      { {0, 0, 400, 40}, {1420, 90, 100, 20}, {1600, 0, 400, 40} },
      { {1750, 40, 3} },
      { {470, 60, 120, 0, 1.2f, 120, 20}, {820, 110, 0, 40, 1.5f, 120, 20},
        {1150, 80, 100, 0, 1.0f, 120, 20} }, {} },
    { "Елестер", 2200, 160, {60, 40}, {2120, 40, 30, 50},
      { {0, 0, 2200, 40}, {300, 100, 120, 20}, {520, 180, 120, 20},
        {740, 260, 120, 20}, {1000, 260, 200, 20} },
      { {900, 40, 4}, {1220, 40, 3} }, {},
      { {690, 230, 0, 50, 2.0f, 14, 0}, {1100, 330, 90, 0, 1.5f, 14, 0},
        {1550, 70, 180, 0, 1.2f, 16, 0}, {1880, 90, 0, 40, 2.4f, 16, 0} } },
    { "Соңғы жарық", 2400, 130, {60, 40}, {2340, 40, 30, 50},
      { {0, 0, 300, 40}, {560, 0, 500, 40}, {1100, 100, 100, 20},
        {1240, 0, 320, 40}, {1820, 0, 580, 40} },
      { {650, 40, 3}, {850, 40, 3}, {1400, 40, 4}, {2120, 40, 3} },
      { {380, 60, 60, 0, 1.4f, 100, 20}, {1630, 70, 0, 50, 1.6f, 110, 20} },
      { {760, 110, 0, 50, 2.0f, 14, 0}, {1980, 70, 90, 0, 2.2f, 16, 0},
        {2270, 140, 0, 70, 1.8f, 14, 0} } },
};

// ---------------- Ойын күйі ----------------
GLFWwindow* win = nullptr;
GLuint prog = 0, vao = 0, vbo = 0;
GLint  locT, locDark, locPlayer, locExit, locRadius, locAlpha;   // uniform орындары (кэш)
std::vector<float> verts;                                        // x, y, r, g, b
glm::mat4 proj;
Level  lv;                 // ағымдағы деңгейдің жұмыс көшірмесі
Player pl;
int   cur = 0, deaths = 0;
float camX = 0, timeT = 0, reveal = 0, winT = 0;
bool  victory = false;

// ---------------- Шейдерлер (3-апта) ----------------
const char* VS = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uTransform;
out vec3 vColor;
out vec2 vWorld;
void main() {
    vColor = aColor;
    vWorld = aPos;                       // әлем координатасы (жарық есебі үшін)
    gl_Position = uTransform * vec4(aPos, 0.0, 1.0);
})";

const char* FS = R"(#version 330 core
in vec3 vColor;
in vec2 vWorld;
uniform int   uDark;                     // 1 — қараңғылық қабатын сызу
uniform vec2  uPlayer, uExit;
uniform float uRadius, uAlpha;
out vec4 FragColor;
void main() {
    if (uDark == 1) {
        float lp  = 1.0 - smoothstep(uRadius * 0.35, uRadius, distance(vWorld, uPlayer));
        float le  = 1.0 - smoothstep(10.0, 70.0, distance(vWorld, uExit));
        float lit = max(lp, le * 0.8);
        FragColor = vec4(0.0, 0.0, 0.0, uAlpha * (1.0 - lit));
    } else {
        FragColor = vec4(vColor, 1.0);
    }
})";

GLuint compileStage(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    char log[1024];
    if (!ok) { glGetShaderInfoLog(s, 1024, nullptr, log); fprintf(stderr, "Шейдер компиляция қатесі:\n%s\n", log); }
    return s;
}

GLuint makeShader() {
    GLuint vs = compileStage(GL_VERTEX_SHADER, VS);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, FS);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    char log[1024];
    if (!ok) { glGetProgramInfoLog(p, 1024, nullptr, log); fprintf(stderr, "Шейдер link қатесі:\n%s\n", log); }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

// ---------------- Буфер және сызу (2-апта) ----------------
void flush() {                            // бір VAO/VBO, бір glDrawArrays
    if (verts.empty()) return;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 5));
    verts.clear();
}
void vertex(float x, float y, glm::vec3 c) { verts.insert(verts.end(), {x, y, c.r, c.g, c.b}); }
void tri(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec3 col) {
    vertex(a.x, a.y, col); vertex(b.x, b.y, col); vertex(c.x, c.y, col);
}
void rect(Rect r, glm::vec3 c) {
    tri({r.x, r.y}, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, c);
    tri({r.x, r.y}, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}, c);
}
void circle(float cx, float cy, float r, glm::vec3 c, int seg = 24) {
    for (int i = 0; i < seg; i++) {
        float a0 = 6.2831853f * i / seg, a1 = 6.2831853f * (i + 1) / seg;
        tri({cx, cy}, {cx + r * std::cos(a0), cy + r * std::sin(a0)},
                      {cx + r * std::cos(a1), cy + r * std::sin(a1)}, c);
    }
}
void ring(float cx, float cy, float r, float th, glm::vec3 c, int seg = 48) {
    for (int i = 0; i < seg; i++) {
        float a0 = 6.2831853f * i / seg, a1 = 6.2831853f * (i + 1) / seg;
        glm::vec2 i0{cx + (r - th) * std::cos(a0), cy + (r - th) * std::sin(a0)};
        glm::vec2 i1{cx + (r - th) * std::cos(a1), cy + (r - th) * std::sin(a1)};
        glm::vec2 o0{cx + r * std::cos(a0), cy + r * std::sin(a0)};
        glm::vec2 o1{cx + r * std::cos(a1), cy + r * std::sin(a1)};
        tri(i0, o0, o1, c); tri(i0, o1, i1, c);
    }
}

// ---------------- Матрицалар (6-апта) ----------------
void use(const glm::mat4& m) {            // алдымен жиналғанды сызып, сосын матрицаны ауыстырамыз
    flush();
    glUniformMatrix4fv(locT, 1, GL_FALSE, glm::value_ptr(m));
}

// ---------------- Терезе тақырыбы (1-апта) ----------------
void setState(const char* msg) {
    char buf[256];
    if (victory)
        snprintf(buf, sizeof buf, "Көлеңке | ЖЕҢІС! Өлім саны: %d | %s", deaths, msg);
    else
        snprintf(buf, sizeof buf, "Көлеңке | Деңгей %d/%d: %s | Өлім: %d | %s",
                 cur + 1, (int)LEVELS.size(), lv.name, deaths, msg);
    glfwSetWindowTitle(win, buf);
}

// ---------------- Физика ----------------
Rect box() { return {pl.x, pl.y, pl.w, pl.h}; }
Rect moverRect(const Mover& m) { return {m.x, m.y, m.w, m.h}; }
bool overlap(Rect a, Rect b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}
bool circleHitsRect(float cx, float cy, float r, Rect b) {
    float nx = std::clamp(cx, b.x, b.x + b.w), ny = std::clamp(cy, b.y, b.y + b.h);
    return (cx - nx) * (cx - nx) + (cy - ny) * (cy - ny) < r * r;
}

void updateMovers() {
    for (auto* list : {&lv.movers, &lv.drifters})
        for (auto& m : *list) {
            m.px = m.x; m.py = m.y;
            float s = std::sin(timeT * m.speed);
            m.x = m.x0 + m.dx * s;
            m.y = m.y0 + m.dy * s;
        }
}

void resetLevel() {
    lv = LEVELS[cur]; pl = Player();
    pl.x = lv.start.x; pl.y = lv.start.y;
    reveal = 1.0f;                        // деңгей басында карта бір сәт көрінеді
    updateMovers(); updateMovers();       // px == x болуы үшін екі рет
    setState("A/D — жүру, Space — секіру, R — қайта");
}

void kill() {
    deaths++; resetLevel();
    setState("Қараңғылық сені жұтты... Қайта көр");
}

void advance() {
    if (cur + 1 < (int)LEVELS.size()) {
        cur++; resetLevel();
        setState("Есік ашылды! Жарық азая түседі...");
    } else {
        victory = true; winT = 0;
        setState("Enter — қайта ойнау, Esc — шығу");
    }
}

void solve(Rect s, bool xAxis, int moverIdx) {
    Rect b = box();
    if (!overlap(b, s)) return;
    if (xAxis) {
        bool fromLeft = pl.vx > 0 || (pl.vx == 0 && b.x + b.w / 2 < s.x + s.w / 2);
        pl.x = fromLeft ? s.x - pl.w : s.x + s.w;
    } else if (pl.vy <= 0) {              // жоғарыдан қонды
        pl.y = s.y + s.h; pl.vy = 0; pl.ground = true; pl.onMover = moverIdx;
    } else {                              // басымен соқты
        pl.y = s.y - pl.h; pl.vy = 0;
    }
}

void collide(bool xAxis) {
    for (auto& p : lv.plats) solve(p, xAxis, -1);
    for (int i = 0; i < (int)lv.movers.size(); i++) solve(moverRect(lv.movers[i]), xAxis, i);
}

bool down(int k) { return glfwGetKey(win, k) == GLFW_PRESS; }

void updatePlayer(float dt) {
    if (pl.onMover >= 0) {                // жылжымалы платформа ойыншыны бірге алып жүреді
        const Mover& m = lv.movers[pl.onMover];
        pl.x += m.x - m.px; pl.y += m.y - m.py;
    }
    float in = 0;
    if (down(GLFW_KEY_A) || down(GLFW_KEY_LEFT))  in -= 1;
    if (down(GLFW_KEY_D) || down(GLFW_KEY_RIGHT)) in += 1;
    if (in != 0) pl.dir = in > 0 ? 1 : -1;
    pl.vx = in * SPEED;
    pl.vy -= GRAV * dt;

    // coyote time: жиектен түскен соң 0.1 с ішінде әлі де секіруге болады
    pl.coyote = pl.ground ? 0.1f : pl.coyote - dt;
    pl.jumpBuf -= dt;
    if (pl.jumpBuf > 0 && pl.coyote > 0) { pl.vy = JUMP; pl.jumpBuf = 0; pl.coyote = 0; }
    bool held = down(GLFW_KEY_SPACE) || down(GLFW_KEY_W) || down(GLFW_KEY_UP);
    if (!held && pl.vy > 0) pl.vy -= GRAV * dt;   // қысқа басу — аласа секіру

    pl.x += pl.vx * dt; collide(true);
    pl.y += pl.vy * dt; pl.ground = false; pl.onMover = -1; collide(false);
    pl.x = std::clamp(pl.x, 0.0f, lv.width - pl.w);
}

void checkCollisions() {
    Rect b = box();
    for (auto& s : lv.spikes)
        if (overlap(b, {s.x + 3, s.y, s.n * 20.0f - 6, 12})) { kill(); return; }
    for (auto& d : lv.drifters)
        if (circleHitsRect(d.x, d.y, d.w, b)) { kill(); return; }
    if (pl.y < -60) { kill(); return; }
    if (overlap(b, lv.exit)) advance();
}

void step(float dt) {                     // бір физикалық қадам (1/60 с)
    timeT += dt;
    if (victory) { winT += dt; return; }
    reveal = std::max(0.0f, reveal - dt * 0.8f);
    updateMovers(); updatePlayer(dt); checkCollisions();
    camX = std::clamp(pl.x + pl.w / 2 - W / 2, 0.0f, std::max(0.0f, lv.width - W));
}

// ---------------- Сурет салу ----------------
void drawPlatform(Rect r) {
    rect(r, T.plat);
    rect({r.x, r.y + r.h - 4, r.w, 4}, T.edge);       // ашық сұр үстіңгі жиек
}
void drawSpikes() {
    for (auto& s : lv.spikes)
        for (int i = 0; i < s.n; i++) {
            float x = s.x + i * 20;
            tri({x, s.y}, {x + 20, s.y}, {x + 10, s.y + 18}, T.spike);
        }
}
void drawDrifters() {
    float pulse = 1 + 0.15f * std::sin(timeT * 6);
    for (auto& d : lv.drifters) {
        circle(d.x, d.y, d.w * pulse, T.drifter);
        circle(d.x, d.y, d.w * 0.45f, T.bg);           // ортасы қара — «көз»
    }
}
void drawExit() {
    Rect e = lv.exit;
    rect({e.x - 4, e.y, e.w + 8, e.h + 4}, T.exit);
    rect(e, T.bg);
    float g = 0.5f + 0.5f * std::sin(timeT * 3);       // жыпылықтайтын шам
    rect({e.x + e.w / 2 - 3, e.y + e.h / 2 - 3, 6, 6}, glm::vec3(g));
}
void drawPlayer() {
    rect(box(), T.player);
    float ex = pl.x + (pl.dir > 0 ? 10 : 3);           // көздер қозғалыс бағытына қарайды
    rect({ex, pl.y + 12, 3, 5}, T.bg);
    rect({ex + 6, pl.y + 12, 3, 5}, T.bg);
}
void drawDarkness() {                     // 4-апта: uniform-дар арқылы қараңғылық
    flush();
    glUniform1i(locDark, 1);
    glUniform2f(locPlayer, pl.x + pl.w / 2, pl.y + pl.h / 2);
    glUniform2f(locExit, lv.exit.x + lv.exit.w / 2, lv.exit.y + lv.exit.h / 2);
    float flick = 1 + 0.03f * std::sin(timeT * 13);    // шам сәл дірілдейді
    glUniform1f(locRadius, lv.radius * flick + reveal * reveal * 1200);
    glUniform1f(locAlpha, 0.97f);
    rect({camX - 10, -10, W + 20, H + 20}, glm::vec3(0));
    flush();
    glUniform1i(locDark, 0);
}
void drawVictory() {
    use(proj);
    for (int i = 0; i < 6; i++) {
        float r = std::fmod(winT * 120 + i * 80, 480.0f);
        ring(W / 2, H / 2, r + 6, 4, glm::vec3(1 - r / 480));
    }
    rect({W / 2 - 11, H / 2 - 11, 22, 22}, T.player);
    flush();
}
void render() {
    glClearColor(T.bg.r, T.bg.g, T.bg.b, 1); glClear(GL_COLOR_BUFFER_BIT);
    if (victory) { drawVictory(); return; }
    use(proj * glm::translate(glm::mat4(1.0f), glm::vec3(-camX, 0, 0)));   // камера
    for (auto& p : lv.plats)  drawPlatform(p);
    for (auto& m : lv.movers) drawPlatform(moverRect(m));
    drawSpikes(); drawDrifters(); drawExit(); drawPlayer();
    drawDarkness();
}

// ---------------- Терезе оқиғалары (1-апта) ----------------
void onKey(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, 1);
    if (key == GLFW_KEY_SPACE || key == GLFW_KEY_W || key == GLFW_KEY_UP) pl.jumpBuf = 0.12f;
    if (key == GLFW_KEY_R && !victory) resetLevel();
    if (key == GLFW_KEY_ENTER && victory) { victory = false; cur = 0; deaths = 0; resetLevel(); }
}
void onResize(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

int main() {
    if (!glfwInit()) { fprintf(stderr, "GLFW іске қосылмады\n"); return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    win = glfwCreateWindow((int)W, (int)H, "Көлеңке", nullptr, nullptr);
    if (!win) { fprintf(stderr, "Терезе ашылмады\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win); glfwSwapInterval(1);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { fprintf(stderr, "GLEW іске қосылмады\n"); return -1; }
    glfwSetKeyCallback(win, onKey); glfwSetFramebufferSizeCallback(win, onResize);
    int fw, fh; glfwGetFramebufferSize(win, &fw, &fh); glViewport(0, 0, fw, fh);

    prog = makeShader(); glUseProgram(prog);
    locT      = glGetUniformLocation(prog, "uTransform");   // циклге дейін кэштейміз
    locDark   = glGetUniformLocation(prog, "uDark");
    locPlayer = glGetUniformLocation(prog, "uPlayer");
    locExit   = glGetUniformLocation(prog, "uExit");
    locRadius = glGetUniformLocation(prog, "uRadius");
    locAlpha  = glGetUniformLocation(prog, "uAlpha");
    glUniform1i(locDark, 0);

    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);                   // позиция
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float))); // түс
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    proj = glm::ortho(0.0f, W, 0.0f, H);                  // экран координаттары
    resetLevel();

    double last = glfwGetTime(); float acc = 0;
    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        acc += std::min((float)(now - last), 0.25f); last = now;
        while (acc >= STEP) { step(STEP); acc -= STEP; }   // тұрақты қадаммен физика
        render(); glfwSwapBuffers(win); glfwPollEvents();
    }
    glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); glDeleteProgram(prog);
    glfwTerminate(); return 0;
}
