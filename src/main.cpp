#include <GL/freeglut.h>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

enum ArrowDir { ARROW_UP=0, ARROW_DOWN=1, ARROW_LEFT=2, ARROW_RIGHT=3 };

struct Arrow {
    ArrowDir dir;
    int result = 0; // 0 = unpressed, 1 = correct, -1 = wrong
};

std::vector<Arrow> sequence;
int currentIdx = 0; 

int windowW = 640, windowH = 480;

int score = 0;
int combo = 0;
int maxCombo = 0;

const char* dirName(ArrowDir d) {
    switch (d) {
        case ARROW_UP: return "UP";
        case ARROW_DOWN: return "DOWN";
        case ARROW_LEFT: return "LEFT";
        default: return "RIGHT";
    }
}

void drawArrowShape(float x, float y, ArrowDir dir, int result) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    float angle = 0.0f;
    if (dir == ARROW_RIGHT) angle = 0.0f;
    else if (dir == ARROW_UP) angle = 90.0f;
    else if (dir == ARROW_LEFT) angle = 180.0f;
    else if (dir == ARROW_DOWN) angle = -90.0f;
    glRotatef(angle, 0, 0, 1);

    if (result == 1) glColor3f(0.2f, 1.0f, 0.2f); // correct = green
    else if (result == -1) glColor3f(1.0f, 0.2f, 0.2f); // wrong = red
    else glColor3f(0.9f, 0.9f, 0.9f); // unpressed = white

    glBegin(GL_TRIANGLES);
      glVertex2f(0.25f, 0.0f);
      glVertex2f(-0.1f, 0.15f);
      glVertex2f(-0.1f, -0.15f);
    glEnd();

    glPopMatrix();
}

void displayHUD() {
    char buf[128];
    glColor3f(1,1,1);
    glRasterPos2f(-0.95f, 0.9f);
    const char* msg = "Controls: arrow keys to press, \nR: restart \nESC to quit.\n";
    for (const char* p = msg; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);

    sprintf(buf, "Score: %d   Combo: %d   MaxCombo: %d", score, combo, maxCombo);
    glRasterPos2f(-0.95f, 0.85f);
    for (const char* p = buf; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);
}

void generateSequence() {
    sequence.clear();
    srand((unsigned)time(0));
    for (int i = 0; i < 4; ++i) {
        sequence.push_back({static_cast<ArrowDir>(rand()%4)});
    }
    currentIdx = 0;
    combo = 0;

    // Play sound for new sequence
   // if (std::ifstream("start.wav")) {
       // PlaySound(TEXT("start.wav"), NULL, SND_FILENAME | SND_ASYNC);
   // }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    displayHUD();

    const float gap = 0.4f;
    float centerX = 0.0f;
    float xs[4];
    for (int i = 0; i < 4; ++i) xs[i] = centerX + (i-1.5f)*gap;

    for (int i = 0; i < 4; ++i) {
        drawArrowShape(xs[i], 0.0f, sequence[i].dir, sequence[i].result);
    }

    glutSwapBuffers();
}

void specialKeys(int key, int x, int y) {
    if (currentIdx >= (int)sequence.size()) return;

    ArrowDir pressedDir;
    if (key == GLUT_KEY_UP) pressedDir = ARROW_UP;
    else if (key == GLUT_KEY_DOWN) pressedDir = ARROW_DOWN;
    else if (key == GLUT_KEY_LEFT) pressedDir = ARROW_LEFT;
    else pressedDir = ARROW_RIGHT;

    // debugging
    if (pressedDir == sequence[currentIdx].dir) {
        sequence[currentIdx].result = 1; // correct
        score += 1;
        combo++;
        if (combo > maxCombo) maxCombo = combo;
        printf("correct %s   Combo: %d   Score: %d\n", dirName(pressedDir), combo, score);
    } else {
        sequence[currentIdx].result = -1; // wrong
        combo = 0;
        printf("wrong Pressed %s, expected %s\n", dirName(pressedDir), dirName(sequence[currentIdx].dir));
    }

    currentIdx++;

    if (currentIdx >= (int)sequence.size()) {
        printf("Sequence complete! Generating new sequence...\n");
        generateSequence();
    }

    glutPostRedisplay();
}

// for reset
void keyboard(unsigned char k, int x, int y) {
    if (k == 27) exit(0); 
    if (k == 'r' || k == 'R') {
        score = combo = maxCombo = 0;
        generateSequence();
        glutPostRedisplay();
    }
}


void reshape(int w, int h) {
    windowW = w; windowH = h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)w / (float)h;
    if (aspect >= 1.0f)
        glOrtho(-aspect, aspect, -1, 1, -1, 1);
    else
        glOrtho(-1, 1, -1.0f/aspect, 1.0f/aspect, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(windowW, windowH);
    glutCreateWindow("dispatch");

    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);

    generateSequence();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
