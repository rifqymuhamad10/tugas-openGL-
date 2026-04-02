#include <windows.h>
#include "GL/glut.h"
#include <math.h>


float wheelAngle = 0;
float steerAngle = 0;

void drawWheel() {
    glPushMatrix();
    glRotatef(90, 0, 1, 0);
    glScalef(1, 1, 0.4);
    glutSolidTorus(0.15, 0.35, 12, 24);
    glPopMatrix();
}

void drawCar() {
    glPushMatrix();

    // === BODI BAWAH ===
    glPushMatrix();
    glScalef(4, 1, 2);
    glColor3f(0.2, 0.5, 1.0);
    glutSolidCube(1);
    glPopMatrix();

    // === KABIN (atap) ===
    glPushMatrix();
    glTranslatef(-0.3, 0.85, 0);
    glScalef(2.2, 0.7, 1.8);
    glColor3f(0.15, 0.4, 0.85);
    glutSolidCube(1);
    glPopMatrix();

    // === LAMPU DEPAN KIRI ===
    glPushMatrix();
    glTranslatef(2.0, 0.1, 0.6);
    glScalef(0.15, 0.25, 0.35);
    glColor3f(1.0, 1.0, 0.6);
    glutSolidCube(1);
    glPopMatrix();

    // === LAMPU DEPAN KANAN ===
    glPushMatrix();
    glTranslatef(2.0, 0.1, -0.6);
    glScalef(0.15, 0.25, 0.35);
    glColor3f(1.0, 1.0, 0.6);
    glutSolidCube(1);
    glPopMatrix();

    // === LAMPU BELAKANG KIRI ===
    glPushMatrix();
    glTranslatef(-2.0, 0.1, 0.6);
    glScalef(0.15, 0.25, 0.35);
    glColor3f(1.0, 0.1, 0.1);
    glutSolidCube(1);
    glPopMatrix();

    // === LAMPU BELAKANG KANAN ===
    glPushMatrix();
    glTranslatef(-2.0, 0.1, -0.6);
    glScalef(0.15, 0.25, 0.35);
    glColor3f(1.0, 0.1, 0.1);
    glutSolidCube(1);
    glPopMatrix();

    // === RODA DEPAN KIRI ===
    glPushMatrix();
    glTranslatef(1.2, -0.45, 1.1);
    glRotatef(steerAngle, 0, 1, 0);   // kemudi
    glRotatef(wheelAngle, 1, 0, 0);   // berputar
    glColor3f(0.15, 0.15, 0.15);
    drawWheel();
    glPopMatrix();

    // === RODA DEPAN KANAN ===
    glPushMatrix();
    glTranslatef(1.2, -0.45, -1.1);
    glRotatef(steerAngle, 0, 1, 0);
    glRotatef(wheelAngle, 1, 0, 0);
    glColor3f(0.15, 0.15, 0.15);
    drawWheel();
    glPopMatrix();

    // === RODA BELAKANG KIRI ===
    glPushMatrix();
    glTranslatef(-1.2, -0.45, 1.1);
    glRotatef(wheelAngle, 1, 0, 0);
    glColor3f(0.15, 0.15, 0.15);
    drawWheel();
    glPopMatrix();

    // === RODA BELAKANG KANAN ===
    glPushMatrix();
    glTranslatef(-1.2, -0.45, -1.1);
    glRotatef(wheelAngle, 1, 0, 0);
    glColor3f(0.15, 0.15, 0.15);
    drawWheel();
    glPopMatrix();

    glPopMatrix();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(6, 4, 8,
              0, 0, 0,
              0, 1, 0);
    drawCar();
    glutSwapBuffers();
}

void update(int v) {
    wheelAngle += 5;
    steerAngle = 25.0 * sin(wheelAngle * 3.14159 / 180.0 * 0.3);
    if (wheelAngle > 360) wheelAngle -= 360;
    glutPostRedisplay();
    glutTimerFunc(30, update, 0);
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (float)w / h, 1, 100);
    glMatrixMode(GL_MODELVIEW);
}

void init() {
    glClearColor(0.1, 0.1, 0.1, 1);
    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Hierarchical Car");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(30, update, 0);
    glutMainLoop();
    return 0;
}