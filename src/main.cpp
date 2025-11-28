#include "game.h"
#include <windows.h>

void display()
{
    drawGame();

    if (currentGameState == STATE_MENU)
    {
        drawMenu();
    }
    else if (currentGameState == STATE_ALMANAC)
    {
        drawAlmanac();
    }
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    windowW = w;
    windowH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (h == 0) h = 1;
    float asp = (float)w / (float)h;

    screenAspect = asp;

    if (asp >= 1.0f)
        glOrtho(-asp, asp, -1, 1, -1, 1);
    else
        glOrtho(-1, 1, -1.0f / asp, 1.0f / asp, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void timerFunc(int)
{
    if (currentGameState == STATE_PLAYING)
    {
        updateGame();
    }

    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0); 
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowSize(windowW, windowH);
    glutCreateWindow("THISPATCHED");
    glutFullScreen();

    try
    {
        HWND hwnd = FindWindow(NULL, TEXT("THISPATCHED"));
        if (hwnd)
        {
            HICON hIcon = (HICON)LoadImage(NULL, TEXT("thispatched.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
            if (hIcon)
            {
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            }
        }
    }
    catch (...) {}

    playBackgroundMusic();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);
    glutReshapeFunc(reshape);
    glutPassiveMotionFunc(passiveMouse);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    uiTex = makeProceduralUITexture(256, 64);
    initLore();

    makeMap(9, 1, 3, 1);
    gameInProgress = false;
    currentGameState = STATE_MENU;

    glutTimerFunc(16, timerFunc, 0);
    glutMainLoop();
    return 0;
}