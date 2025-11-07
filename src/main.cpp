#include <GL/freeglut.h>
#include <cstdio>

int windowW = 640, windowH = 480;

int arrowSequence[] = {GLUT_KEY_UP, GLUT_KEY_DOWN, GLUT_KEY_LEFT, GLUT_KEY_RIGHT};
const char* arrowNames[] = {"UP", "DOWN", "LEFT", "RIGHT"};
int currentIndex = 0; 
const int totalArrows = sizeof(arrowSequence) / sizeof(arrowSequence[0]);

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glBegin(GL_TRIANGLES);
    switch (arrowSequence[currentIndex]) {
        case GLUT_KEY_UP:
            glVertex2f(0.0f, 0.25f);
            glVertex2f(-0.15f, -0.1f);
            glVertex2f(0.15f, -0.1f);
            break;
        case GLUT_KEY_DOWN:
            glVertex2f(0.0f, -0.25f);
            glVertex2f(-0.15f, 0.1f);
            glVertex2f(0.15f, 0.1f);
            break;
        case GLUT_KEY_LEFT:
            glVertex2f(-0.25f, 0.0f);
            glVertex2f(0.1f, 0.15f);
            glVertex2f(0.1f, -0.15f);
            break;
        case GLUT_KEY_RIGHT:
            glVertex2f(0.25f, 0.0f);
            glVertex2f(-0.1f, 0.15f);
            glVertex2f(-0.1f, -0.15f);
            break;
    }
    glEnd();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    windowW = w; windowH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)w / h;
    if (aspect >= 1.0f)
        glOrtho(-aspect, aspect, -1, 1, -1, 1);
    else
        glOrtho(-1, 1, -1.0f/aspect, 1.0f/aspect, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// for debug
void specialKeys(int key, int x, int y) {
    if (key == arrowSequence[currentIndex]) {
        printf("correct! pressed: %s\n", arrowNames[currentIndex]);
        currentIndex++;
        if (currentIndex >= totalArrows) {
            printf("done\n\n");
            currentIndex = 0;
        } else {
            printf("Next arrow: %s\n", arrowNames[currentIndex]);
        }
    } else {
        printf("wrong!\n");
        printf("restarting.\n\n");
        currentIndex = 0;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(windowW, windowH);
    glutCreateWindow("dispatch");

    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);


    glutMainLoop();
    return 0;
}
