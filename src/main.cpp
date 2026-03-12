/**
 * Hierarchical Modeling - Rumah dengan Halaman
 * =============================================
 * Setup: GLFW + GLAD (Modern OpenGL 3.3 Core)
 *
 * Hierarki:
 *   Dunia
 *   ├── Halaman (tanah, pohon, jalan, semak)
 *   ├── Langit (matahari, awan)
 *   └── Rumah (parent)
 *       ├── Pondasi
 *       ├── Dinding  (parent)
 *       │   ├── Pintu + Kenop
 *       │   ├── Jendela Kiri
 *       │   └── Jendela Kanan
 *       ├── Atap
 *       └── Cerobong + Asap
 *
 * Kompilasi (dari folder root project):
 *   g++ src/glad.c src/main.cpp -o main.exe -Iinclude -Llib -lglfw3dll -lopengl32 -lgdi32
 *
 * Tekan ESC untuk keluar.
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
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
//  MATH - Mat4 column-major (sama seperti OpenGL)
// ============================================================
using Mat4 = float[16];

void identity(Mat4 m) {
    for (int i = 0; i < 16; i++) m[i] = 0.f;
    m[0]=m[5]=m[10]=m[15]=1.f;
}

void ortho(Mat4 m, float l, float r, float b, float t) {
    identity(m);
    m[0]  =  2.f/(r-l);
    m[5]  =  2.f/(t-b);
    m[10] = -1.f;
    m[12] = -(r+l)/(r-l);
    m[13] = -(t+b)/(t-b);
}

void makeModel(Mat4 m, float tx, float ty) {
    identity(m);
    m[12]=tx; m[13]=ty;
}

void mul(Mat4 out, const Mat4 a, const Mat4 b) {
    for (int c=0;c<4;c++)
        for (int r=0;r<4;r++) {
            out[c*4+r]=0;
            for (int k=0;k<4;k++)
                out[c*4+r]+=a[k*4+r]*b[c*4+k];
        }
}

// ============================================================
//  RENDERER
// ============================================================
GLuint gVAO, gVBO, gProg;

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

void initRenderer() {
    GLuint vs = compileShader(GL_VERTEX_SHADER,   VS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FS);
    gProg = glCreateProgram();
    glAttachShader(gProg,vs); glAttachShader(gProg,fs);
    glLinkProgram(gProg);
    glDeleteShader(vs); glDeleteShader(fs);

    glGenVertexArrays(1,&gVAO);
    glGenBuffers(1,&gVBO);
    glBindVertexArray(gVAO);
    glBindBuffer(GL_ARRAY_BUFFER,gVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

/**
 * drawRect - gambar persegi panjang, pivot di tengah
 *   proj       : projection matrix
 *   parent     : matrix parent (inti matrix stack)
 *   lx, ly     : posisi lokal relatif dari parent
 *   w, h       : lebar dan tinggi
 *   r, g, b    : warna
 *   outModel   : (opsional) simpan combined matrix untuk dipakai child
 */
void drawRect(const Mat4 proj, const Mat4 parent,
              float lx, float ly, float w, float h,
              float r, float g, float b,
              Mat4 outModel = nullptr)
{
    // Buat local transform lalu kalikan dengan parent → MATRIX STACK
    Mat4 local; makeModel(local, lx, ly);
    Mat4 combined; mul(combined, parent, local);
    if (outModel) for(int i=0;i<16;i++) outModel[i]=combined[i];

    float hw=w*.5f, hh=h*.5f;
    float v[]={ -hw,-hh, hw,-hh, hw,hh, -hw,-hh, hw,hh, -hw,hh };
    glBindBuffer(GL_ARRAY_BUFFER,gVBO);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v);

    glUseProgram(gProg);
    glUniformMatrix4fv(glGetUniformLocation(gProg,"uProjection"),1,GL_FALSE,proj);
    glUniformMatrix4fv(glGetUniformLocation(gProg,"uModel"),     1,GL_FALSE,combined);
    glUniform3f(glGetUniformLocation(gProg,"uColor"),r,g,b);
    glBindVertexArray(gVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}

/**
 * drawTriangle - gambar segitiga, pivot di tengah bawah
 */
void drawTriangle(const Mat4 proj, const Mat4 parent,
                  float lx, float ly, float w, float h,
                  float r, float g, float b)
{
    Mat4 local; makeModel(local, lx, ly);
    Mat4 combined; mul(combined, parent, local);

    float hw=w*.5f;
    float v[]={ -hw,0.f, hw,0.f, 0.f,h };
    glBindBuffer(GL_ARRAY_BUFFER,gVBO);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v);

    glUseProgram(gProg);
    glUniformMatrix4fv(glGetUniformLocation(gProg,"uProjection"),1,GL_FALSE,proj);
    glUniformMatrix4fv(glGetUniformLocation(gProg,"uModel"),     1,GL_FALSE,combined);
    glUniform3f(glGetUniformLocation(gProg,"uColor"),r,g,b);
    glBindVertexArray(gVAO);
    glDrawArrays(GL_TRIANGLES,0,3);
    glBindVertexArray(0);
}

// ============================================================
//  SCENE
// ============================================================
void render(int width, int height) {
    glClearColor(0.52f, 0.80f, 0.98f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    float aspect = (float)width/height;
    float vH=3.f, vW=vH*aspect;
    Mat4 proj; ortho(proj,-vW,vW,-vH,vH);
    Mat4 world; identity(world);

    // ════════════════════════════════════════════
    //  LANGIT
    // ════════════════════════════════════════════

    // Matahari
    drawRect(proj,world, vW-0.8f,vH-0.8f, 0.55f,0.55f, 1.f,0.92f,0.2f);

    // Awan 1
    drawRect(proj,world, -1.6f,2.3f,  0.75f,0.25f, 1.f,1.f,1.f);
    drawRect(proj,world, -1.4f,2.48f, 0.5f,0.22f,  1.f,1.f,1.f);
    drawRect(proj,world, -1.8f,2.43f, 0.38f,0.18f, 1.f,1.f,1.f);

    // Awan 2
    drawRect(proj,world,  1.3f,2.4f,  0.65f,0.22f, 1.f,1.f,1.f);
    drawRect(proj,world,  1.5f,2.54f, 0.42f,0.18f, 1.f,1.f,1.f);
    drawRect(proj,world,  1.1f,2.5f,  0.35f,0.16f, 1.f,1.f,1.f);

    // ════════════════════════════════════════════
    //  HALAMAN
    // ════════════════════════════════════════════

    // Tanah hijau
    drawRect(proj,world, 0.f,-2.0f, vW*2,2.0f, 0.24f,0.62f,0.24f);

    // Jalan setapak
    drawRect(proj,world, 0.f,-1.6f, 0.48f,1.2f,  0.55f,0.55f,0.55f);
    // nat / garis jalan
    for (float ny : {-1.2f,-1.5f,-1.8f})
        drawRect(proj,world, 0.f,ny, 0.48f,0.025f, 0.42f,0.42f,0.42f);

    // Pohon kiri
    drawRect(proj,world,   -2.5f,-1.1f, 0.18f,0.7f,  0.45f,0.28f,0.07f); // batang
    drawTriangle(proj,world,-2.5f,-0.74f,1.1f,0.65f,  0.1f,0.55f,0.1f);
    drawTriangle(proj,world,-2.5f,-0.36f,0.85f,0.55f, 0.12f,0.62f,0.12f);
    drawTriangle(proj,world,-2.5f, 0.0f, 0.6f,0.45f,  0.15f,0.7f,0.15f);

    // Pohon kanan
    drawRect(proj,world,    2.5f,-1.1f, 0.18f,0.7f,  0.45f,0.28f,0.07f);
    drawTriangle(proj,world, 2.5f,-0.74f,1.1f,0.65f,  0.1f,0.55f,0.1f);
    drawTriangle(proj,world, 2.5f,-0.36f,0.85f,0.55f, 0.12f,0.62f,0.12f);
    drawTriangle(proj,world, 2.5f, 0.0f, 0.6f,0.45f,  0.15f,0.7f,0.15f);

    // Semak kiri depan
    drawTriangle(proj,world,-1.35f,-1.2f, 0.45f,0.32f, 0.1f,0.5f,0.1f);
    drawTriangle(proj,world,-1.15f,-1.2f, 0.38f,0.26f, 0.12f,0.56f,0.12f);

    // Semak kanan depan
    drawTriangle(proj,world, 1.35f,-1.2f, 0.45f,0.32f, 0.1f,0.5f,0.1f);
    drawTriangle(proj,world, 1.15f,-1.2f, 0.38f,0.26f, 0.12f,0.56f,0.12f);

    // ════════════════════════════════════════════
    //  RUMAH  ← parent semua bagian rumah
    // ════════════════════════════════════════════
    Mat4 house; makeModel(house, 0.f, 0.f);

    // Pondasi
    drawRect(proj,house, 0.f,-0.62f, 2.15f,0.14f, 0.5f,0.47f,0.43f);

    // Dinding → jadi parent pintu & jendela
    Mat4 wall;
    drawRect(proj,house, 0.f,0.18f, 2.0f,1.15f, 0.94f,0.87f,0.73f, wall);

    // ── Pintu (child dinding) ──
    Mat4 door;
    drawRect(proj,wall,  0.f,-0.21f, 0.33f,0.68f, 0.44f,0.27f,0.09f, door);
    // kenop
    drawRect(proj,door,  0.1f,0.f,   0.06f,0.06f, 0.85f,0.72f,0.1f);
    // kusen atas pintu
    drawRect(proj,wall,  0.f,0.135f, 0.39f,0.05f, 0.58f,0.4f,0.2f);

    // ── Jendela Kiri (child dinding) ──
    // kusen
    drawRect(proj,wall, -0.65f,0.3f, 0.44f,0.38f, 0.58f,0.4f,0.2f);
    // kaca
    drawRect(proj,wall, -0.65f,0.3f, 0.38f,0.32f, 0.65f,0.87f,1.f);
    // palang horizontal & vertikal
    drawRect(proj,wall, -0.65f,0.3f, 0.38f,0.028f,0.88f,0.88f,0.88f);
    drawRect(proj,wall, -0.65f,0.3f, 0.028f,0.32f,0.88f,0.88f,0.88f);

    // ── Jendela Kanan (child dinding) ──
    drawRect(proj,wall,  0.65f,0.3f, 0.44f,0.38f, 0.58f,0.4f,0.2f);
    drawRect(proj,wall,  0.65f,0.3f, 0.38f,0.32f, 0.65f,0.87f,1.f);
    drawRect(proj,wall,  0.65f,0.3f, 0.38f,0.028f,0.88f,0.88f,0.88f);
    drawRect(proj,wall,  0.65f,0.3f, 0.028f,0.32f,0.88f,0.88f,0.88f);

    // ── Atap (child rumah) ──
    drawTriangle(proj,house, 0.f,0.76f, 2.35f,0.88f, 0.70f,0.20f,0.10f);
    // lisplang / tepi bawah atap
    drawRect(proj,house, 0.f,0.76f, 2.38f,0.07f, 0.52f,0.14f,0.07f);

    // ── Cerobong (child rumah) ──
    drawRect(proj,house, 0.55f,1.45f, 0.24f,0.5f,  0.62f,0.38f,0.3f);
    // tutup cerobong
    drawRect(proj,house, 0.55f,1.72f, 0.30f,0.06f, 0.45f,0.27f,0.2f);

    // ── Asap (child rumah) ──
    drawRect(proj,house, 0.55f,1.90f, 0.16f,0.16f, 0.82f,0.82f,0.82f);
    drawRect(proj,house, 0.62f,2.10f, 0.20f,0.20f, 0.74f,0.74f,0.74f);
    drawRect(proj,house, 0.54f,2.32f, 0.22f,0.22f, 0.66f,0.66f,0.66f);
}

// ============================================================
//  MAIN
// ============================================================
void keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (action==GLFW_PRESS && key==GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(win, GLFW_TRUE);
}

int main() {
    if (!glfwInit()) { std::cerr<<"GLFW gagal\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(800,600,
        "Hierarchical Modeling - Rumah & Halaman",nullptr,nullptr);
    if (!win) { std::cerr<<"Window gagal\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr<<"GLAD gagal\n"; return -1;
    }

    initRenderer();

    while (!glfwWindowShouldClose(win)) {
        int w,h;
        glfwGetFramebufferSize(win,&w,&h);
        glViewport(0,0,w,h);
        render(w,h);
        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}