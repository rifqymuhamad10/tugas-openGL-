/**
 * Hierarchical Modeling - Rumah dengan Halaman
 * =============================================
 * Setup: GLFW + GLAD (Modern OpenGL 3.3 Core)
 *
 * Hierarki:
 *   Dunia
 *   ├── Halaman (tanah hijau)
 *   └── Rumah (parent)
 *       ├── Dinding
 *       ├── Atap
 *       ├── Pintu
 *       └── Jendela (kiri & kanan)
 *
 * Kontrol:
 *   A/D     - Geser rumah kiri/kanan
 *   W/S     - Geser rumah atas/bawah
 *   Q/E     - Rotasi rumah
 *   Z/X     - Scale rumah (perbesar/perkecil)
 *   R       - Reset
 *   ESC     - Keluar
 *
 * Kompilasi (MinGW, dari folder root project):
 *   g++ src/glad.c src/main.cpp -o main.exe
 *     -Iinclude -Llib
 *     -lglfw3dll -lopengl32 -lgdi32
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cmath>

// ============================================================
//  SHADER SOURCE
// ============================================================
const char* VS = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform mat4 uModel;
uniform mat4 uProjection;
void main() {
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
}
)";

const char* FS = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

// ============================================================
//  GLOBALS - transformasi rumah
// ============================================================
float houseX    = 0.0f;
float houseY    = 0.0f;
float houseRot  = 0.0f;
float houseScale= 1.0f;

// ============================================================
//  SHADER HELPERS
// ============================================================
GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader error: " << log << "\n";
    }
    return s;
}

GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER,   VS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FS);
    GLuint p  = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

// ============================================================
//  MATH - matrix 4x4 (column-major, seperti OpenGL)
// ============================================================
using Mat4 = float[16];

void identity(Mat4 m) {
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0]=m[5]=m[10]=m[15]=1.0f;
}

// ortho projection: kiri,kanan,bawah,atas
void ortho(Mat4 m, float l, float r, float b, float t) {
    identity(m);
    m[0]  =  2.0f/(r-l);
    m[5]  =  2.0f/(t-b);
    m[10] = -1.0f;
    m[12] = -(r+l)/(r-l);
    m[13] = -(t+b)/(t-b);
}

// Terapkan translate ke matrix
void translate(Mat4 m, float tx, float ty) {
    m[12] += tx * m[0] + ty * m[4];
    m[13] += tx * m[1] + ty * m[5];
}

// Buat matrix model: translate + rotate + scale
void makeModel(Mat4 m, float tx, float ty, float rot, float sc) {
    float c = std::cos(rot), s = std::sin(rot);
    // Column-major:
    m[0]  =  sc*c;  m[4] = -sc*s;  m[8]  = 0; m[12] = tx;
    m[1]  =  sc*s;  m[5] =  sc*c;  m[9]  = 0; m[13] = ty;
    m[2]  =  0;     m[6] =  0;     m[10] = 1; m[14] = 0;
    m[3]  =  0;     m[7] =  0;     m[11] = 0; m[15] = 1;
}

// Multiply dua mat4
void mul(Mat4 out, const Mat4 a, const Mat4 b) {
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++) {
            out[col*4+row] = 0;
            for (int k = 0; k < 4; k++)
                out[col*4+row] += a[k*4+row] * b[col*4+k];
        }
}

// ============================================================
//  PRIMITIVE RENDERER
// ============================================================
GLuint gVAO, gVBO, gProg;

void initRenderer() {
    // Quad unit (dua segitiga), vertex akan diisi tiap draw call
    glGenVertexArrays(1, &gVAO);
    glGenBuffers(1, &gVBO);

    glBindVertexArray(gVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    gProg = createProgram();
}

/**
 * drawRect - menggambar persegi panjang
 * @param proj      : projection matrix
 * @param parentModel: matrix parent (matrix stack!)
 * @param lx,ly     : offset lokal dari parent
 * @param w,h       : lebar & tinggi
 * @param r,g,b     : warna
 * @param rotLocal  : rotasi lokal tambahan
 * @return matrix model objek ini (bisa dipakai sebagai parent child)
 */
void drawRect(const Mat4 proj,
              const Mat4 parentModel,
              float lx, float ly,
              float w,  float h,
              float r,  float g, float b,
              float rotLocal = 0.0f,
              float* outModel = nullptr)
{
    // Buat local transform
    Mat4 local;
    makeModel(local, lx, ly, rotLocal, 1.0f);

    // Kalikan dengan parent → ini inti "matrix stack"
    Mat4 combined;
    mul(combined, parentModel, local);

    // Simpan ke outModel jika perlu dipakai child
    if (outModel) {
        for (int i = 0; i < 16; i++) outModel[i] = combined[i];
    }

    // Vertex quad (dua segitiga, berpusat di origin lokal)
    float hw = w / 2.0f, hh = h / 2.0f;
    float verts[] = {
        -hw, -hh,   hw, -hh,   hw,  hh,
        -hw, -hh,   hw,  hh,  -hw,  hh
    };

    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glUseProgram(gProg);
    glUniformMatrix4fv(glGetUniformLocation(gProg,"uProjection"), 1, GL_FALSE, proj);
    glUniformMatrix4fv(glGetUniformLocation(gProg,"uModel"),      1, GL_FALSE, combined);
    glUniform3f(glGetUniformLocation(gProg,"uColor"), r, g, b);

    glBindVertexArray(gVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

/**
 * drawTriangle - menggambar segitiga (untuk atap)
 * Pivot di tengah bawah segitiga
 */
void drawTriangle(const Mat4 proj,
                  const Mat4 parentModel,
                  float lx, float ly,
                  float w,  float h,
                  float r,  float g, float b)
{
    Mat4 local;
    makeModel(local, lx, ly, 0.0f, 1.0f);

    Mat4 combined;
    mul(combined, parentModel, local);

    float hw = w / 2.0f;
    float verts[] = {
        -hw, 0.0f,   hw, 0.0f,   0.0f, h
    };

    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glUseProgram(gProg);
    glUniformMatrix4fv(glGetUniformLocation(gProg,"uProjection"), 1, GL_FALSE, proj);
    glUniformMatrix4fv(glGetUniformLocation(gProg,"uModel"),      1, GL_FALSE, combined);
    glUniform3f(glGetUniformLocation(gProg,"uColor"), r, g, b);

    glBindVertexArray(gVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

// ============================================================
//  SCENE RENDER
// ============================================================
void render(int width, int height) {
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f); // langit biru muda
    glClear(GL_COLOR_BUFFER_BIT);

    // Aspect ratio → koordinat dunia: -4..4 horizontal, -3..3 vertical
    float aspect = (float)width / height;
    float viewH  = 3.0f;
    float viewW  = viewH * aspect;

    Mat4 proj;
    ortho(proj, -viewW, viewW, -viewH, viewH);

    // ── IDENTITY matrix sebagai "dunia" (root) ──
    Mat4 world; identity(world);

    // ==============================================
    //  [1] HALAMAN (child dari dunia, statis)
    // ==============================================
    // Tanah hijau
    drawRect(proj, world,  0.0f, -1.8f, viewW*2, 2.4f, 0.3f, 0.7f, 0.3f);

    // Jalan setapak (batu abu)
    drawRect(proj, world,  0.0f, -2.0f, 0.4f, 1.5f, 0.6f, 0.6f, 0.6f);

    // Pohon kiri
    // - batang
    drawRect(proj, world, -2.8f, -1.0f, 0.15f, 0.6f, 0.5f, 0.3f, 0.1f);
    // - daun (segitiga susun)
    drawTriangle(proj, world, -2.8f, -0.7f, 0.9f, 0.6f, 0.1f, 0.5f, 0.1f);
    drawTriangle(proj, world, -2.8f, -0.3f, 0.7f, 0.5f, 0.15f,0.6f,0.15f);
    drawTriangle(proj, world, -2.8f,  0.05f,0.5f, 0.4f, 0.2f, 0.7f, 0.2f);

    // Pohon kanan
    drawRect(proj, world,  2.8f, -1.0f, 0.15f, 0.6f, 0.5f, 0.3f, 0.1f);
    drawTriangle(proj, world,  2.8f, -0.7f, 0.9f, 0.6f, 0.1f, 0.5f, 0.1f);
    drawTriangle(proj, world,  2.8f, -0.3f, 0.7f, 0.5f, 0.15f,0.6f,0.15f);
    drawTriangle(proj, world,  2.8f,  0.05f,0.5f, 0.4f, 0.2f, 0.7f, 0.2f);

    // ==============================================
    //  [2] RUMAH - matrix parent dari semua bagian
    //      Transformasi rumah dikontrol user (A/D/W/S/Q/E/Z/X)
    // ==============================================
    Mat4 houseModel;
    makeModel(houseModel, houseX, houseY, houseRot * 3.14159f/180.0f, houseScale);

    // -- Dinding utama (pivot rumah di tengah bawah dinding) --
    Mat4 wallModel;
    drawRect(proj, houseModel,
             0.0f, 0.5f,        // offset lokal dari pivot rumah
             2.0f, 1.0f,        // lebar x tinggi
             0.9f, 0.8f, 0.6f, // warna krem
             0.0f, wallModel);  // simpan wallModel untuk anak2nya

    // -- Atap (child dari rumah, bukan child dinding) --
    drawTriangle(proj, houseModel,
                 0.0f, 1.0f,        // duduk di atas dinding
                 2.2f, 0.7f,        // lebar sedikit lebih dari dinding
                 0.7f, 0.2f, 0.1f); // warna merah bata

    // -- Pintu (child dari dinding) --
    //    Posisi relatif terhadap tengah dinding
    drawRect(proj, wallModel,
             0.0f, -0.2f,       // tengah bawah dinding
             0.3f, 0.6f,        // lebar pintu
             0.5f, 0.3f, 0.1f); // coklat tua

    // Kenop pintu
    drawRect(proj, wallModel,
             0.1f, -0.2f,
             0.05f, 0.05f,
             0.9f, 0.8f, 0.1f); // kuning emas

    // -- Jendela kiri (child dari dinding) --
    drawRect(proj, wallModel,
            -0.6f, 0.3f,
             0.35f, 0.3f,
             0.6f, 0.85f, 1.0f); // biru muda (kaca)
    // palang jendela kiri - horizontal
    drawRect(proj, wallModel,
            -0.6f, 0.3f,
             0.35f, 0.03f,
             0.8f, 0.8f, 0.8f);
    // palang jendela kiri - vertikal
    drawRect(proj, wallModel,
            -0.6f, 0.3f,
             0.03f, 0.3f,
             0.8f, 0.8f, 0.8f);

    // -- Jendela kanan (child dari dinding) --
    drawRect(proj, wallModel,
             0.6f, 0.3f,
             0.35f, 0.3f,
             0.6f, 0.85f, 1.0f);
    drawRect(proj, wallModel,
             0.6f, 0.3f,
             0.35f, 0.03f,
             0.8f, 0.8f, 0.8f);
    drawRect(proj, wallModel,
             0.6f, 0.3f,
             0.03f, 0.3f,
             0.8f, 0.8f, 0.8f);

    // -- Cerobong asap (child dari rumah) --
    drawRect(proj, houseModel,
             0.5f, 1.55f,
             0.2f, 0.4f,
             0.5f, 0.4f, 0.4f);

    // -- Pondasi (child dari rumah) --
    drawRect(proj, houseModel,
             0.0f, -0.05f,
             2.1f, 0.12f,
             0.5f, 0.5f, 0.5f);
}

// ============================================================
//  INPUT
// ============================================================
void keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    const float STEP = 0.1f, ROT = 5.0f, SC = 0.05f;
    switch (key) {
        case GLFW_KEY_A:      houseX    -= STEP; break;
        case GLFW_KEY_D:      houseX    += STEP; break;
        case GLFW_KEY_W:      houseY    += STEP; break;
        case GLFW_KEY_S:      houseY    -= STEP; break;
        case GLFW_KEY_Q:      houseRot  += ROT;  break;
        case GLFW_KEY_E:      houseRot  -= ROT;  break;
        case GLFW_KEY_Z:      houseScale+= SC;   break;
        case GLFW_KEY_X:      houseScale = std::max(0.1f, houseScale - SC); break;
        case GLFW_KEY_R:
            houseX = houseY = houseRot = 0.0f;
            houseScale = 1.0f;
            break;
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(win, GLFW_TRUE); break;
    }
}

// ============================================================
//  MAIN
// ============================================================
int main() {
    if (!glfwInit()) { std::cerr << "GLFW init gagal\n"; return -1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(800, 600,
        "Hierarchical Modeling - Rumah & Halaman", nullptr, nullptr);
    if (!win) { std::cerr << "Window gagal dibuat\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init gagal\n"; return -1;
    }

    initRenderer();

    std::cout << "=== Hierarchical Modeling - Rumah ===\n"
              << "  A/D   : geser kiri/kanan\n"
              << "  W/S   : geser atas/bawah\n"
              << "  Q/E   : rotasi\n"
              << "  Z/X   : perbesar / perkecil\n"
              << "  R     : reset\n"
              << "  ESC   : keluar\n";

    while (!glfwWindowShouldClose(win)) {
        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);

        render(w, h);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}