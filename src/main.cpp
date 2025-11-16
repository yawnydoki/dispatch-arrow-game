#include <GL/freeglut.h>
#include <windows.h>
#include <mmsystem.h>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <thread>
#pragma comment(lib, "winmm.lib")

// ---------- Enums & Structs ----------
enum ArrowDir
{
    ARROW_UP = 0,
    ARROW_DOWN = 1,
    ARROW_LEFT = 2,
    ARROW_RIGHT = 3
};

struct Arrow
{
    ArrowDir dir;
    int result = 0;           // 0=unpressed, 1=Perfect, 2=Good, 3=Bad, -1=Miss/Wrong
    float displayAnim = 1.0f; // scale animation
};

struct Node
{
    float x, y;
    std::vector<int> links;
    bool visited = false;
    bool hasReward = false;
};

struct Player
{
    int currentNode = 0;
    int highlightedNeighbor = 0;
    int targetNode = -1;
    float x = 0, y = 0;
    bool moving = false;
    int moveFrom = -1, moveTo = -1;
    float moveProgress = 0.0f;
    int moveStartTimeMs = 0;
};

// ---------- Globals ----------
std::vector<Node> nodes;
Player player;

std::vector<Arrow> sequence;
int seqIdx = 0;
bool inSequence = false;
bool stepWaitingForInput = false;
int stepStartTimeMs = 0;

int score = 0, combo = 0, maxCombo = 0;

int windowW = 960, windowH = 600;

GLuint uiTex = 0;

// Timing windows (ms)
const int PERFECT_MS = 300;
const int GOOD_MS = 600;
const int BAD_MS = 1000;
const int MOVE_DURATION = 600; // ms

// ---------- Utility ----------
const char *dirName(ArrowDir d)
{
    switch (d)
    {
    case ARROW_UP:
        return "UP";
    case ARROW_DOWN:
        return "DOWN";
    case ARROW_LEFT:
        return "LEFT";
    default:
        return "RIGHT";
    }
}

void drawString(const char *s, float x, float y)
{
    glRasterPos2f(x, y);
    for (const char *p = s; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);
}

void drawCircle(float cx, float cy, float r, int seg = 32)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; i++)
    {
        float a = (float)i / seg * 2.0f * 3.1415926f;
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

void drawRing(float cx, float cy, float ro, float ri, int seg = 32)
{
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= seg; i++)
    {
        float a = (float)i / seg * 2.0f * 3.1415926f;
        glVertex2f(cx + cosf(a) * ro, cy + sinf(a) * ro);
        glVertex2f(cx + cosf(a) * ri, cy + sinf(a) * ri);
    }
    glEnd();
}

void playBackgroundMusic()
{
    PlaySound(TEXT("Dispatch - Gristle.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
}

void playSFX(const char* file) {
    std::thread([file](){
        PlaySound(TEXT(file), NULL, SND_FILENAME | SND_ASYNC);
    }).detach();
}

// ---------- Procedural UI Texture ----------
GLuint makeProceduralUITexture(int w = 256, int h = 64)
{
    std::vector<unsigned char> pixels(w * h * 4);
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            float fx = (float)x / (w - 1), fy = (float)y / (h - 1);
            unsigned char r = (unsigned char)(30 + 80 * fy + 30 * fx);
            unsigned char g = (unsigned char)(50 + 80 * fy);
            unsigned char b = (unsigned char)(70 + 50 * fx);
            int gx = (x / 8) % 2, gy = (y / 8) % 2;
            int add = (gx ^ gy) ? 6 : 0;
            int idx = (y * w + x) * 4;
            pixels[idx + 0] = std::min(255, r + add);
            pixels[idx + 1] = std::min(255, g + add);
            pixels[idx + 2] = std::min(255, b + add);
            pixels[idx + 3] = 255;
        }
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return tex;
}

// ---------- Arrow Drawing ----------
void drawArrowShape(float x, float y, ArrowDir dir, int result, float size = 0.07f, float animScale = 1.0f)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    float ang = 0;
    if (dir == ARROW_RIGHT)
        ang = 0;
    else if (dir == ARROW_UP)
        ang = 90;
    else if (dir == ARROW_LEFT)
        ang = 180;
    else
        ang = -90;
    glRotatef(ang, 0, 0, 1);
    glScalef(size * animScale, size * animScale, 1);

    if (result == 1)
        glColor3f(0.1f, 1.0f, 0.2f); // Perfect
    else if (result == 2)
        glColor3f(0.4f, 0.8f, 0.2f); // Good
    else if (result == 3)
        glColor3f(1.0f, 0.7f, 0.2f); // Bad
    else if (result == -1)
        glColor3f(1.0f, 0.2f, 0.2f); // Miss/Wrong
    else
        glColor3f(0.95f, 0.95f, 0.95f); // Default

    glBegin(GL_TRIANGLES);
    glVertex2f(0.9f, 0.0f);
    glVertex2f(-0.4f, 0.55f);
    glVertex2f(-0.4f, -0.55f);
    glEnd();
    glPopMatrix();
}

// ---------- Map ----------
void makeDemoMap()
{
    nodes.clear();
    nodes.resize(9);
    float coords[9][2] = {
        {-0.8f, 0.5f}, {-0.4f, 0.5f}, {0.0f, 0.5f}, {0.0f, 0.0f}, {-0.4f, 0.0f}, {0.4f, 0.0f}, {0.8f, 0.4f}, {0.4f, -0.4f}, {0.0f, -0.5f}};
    for (int i = 0; i < 9; i++)
    {
        nodes[i].x = coords[i][0];
        nodes[i].y = coords[i][1];
    }

    auto link = [&](int a, int b)
    { nodes[a].links.push_back(b); nodes[b].links.push_back(a); };
    link(0, 1);
    link(1, 2);
    link(2, 3);
    link(3, 4);
    link(4, 1);
    link(3, 5);
    link(5, 6);
    link(5, 7);
    link(7, 8);

    nodes[6].hasReward = true;
    nodes[8].hasReward = true;

    for (auto &n : nodes)
        n.visited = false;
    player.currentNode = 3;
    player.x = nodes[player.currentNode].x;
    player.y = nodes[player.currentNode].y;
    player.highlightedNeighbor = 0;
    player.targetNode = -1;
    player.moving = false;
}

// ---------- Sequence ----------
void generateSequence(int len = 4)
{
    sequence.clear();
    for (int i = 0; i < len; i++)
    {
        Arrow a;
        a.dir = static_cast<ArrowDir>(rand() % 4);
        a.result = 0;
        a.displayAnim = 1.0f;
        sequence.push_back(a);
    }
    seqIdx = 0;
    stepWaitingForInput = true;
    stepStartTimeMs = glutGet(GLUT_ELAPSED_TIME);
}

// ---------- Neighbor Helper ----------
void clampHighlightedNeighbor()
{
    if (nodes[player.currentNode].links.empty())
    {
        player.highlightedNeighbor = 0;
        return;
    }
    if (player.highlightedNeighbor < 0)
        player.highlightedNeighbor = (int)nodes[player.currentNode].links.size() - 1;
    if (player.highlightedNeighbor >= (int)nodes[player.currentNode].links.size())
        player.highlightedNeighbor = 0;
}

int getHighlightedNeighbor()
{
    if (nodes[player.currentNode].links.empty())
        return -1;
    clampHighlightedNeighbor();
    return nodes[player.currentNode].links[player.highlightedNeighbor];
}

// ---------- Movement ----------
void startMoveAnimation(int from, int to)
{
    player.moving = true;
    player.moveFrom = from;
    player.moveTo = to;
    player.moveProgress = 0.0f;
    player.moveStartTimeMs = glutGet(GLUT_ELAPSED_TIME);
    playSFX("Movementplayback");
}

// ---------- Game Update ----------
void updateGame()
{
    int now = glutGet(GLUT_ELAPSED_TIME);

    // Animate arrows
    for (auto &a : sequence)
    {
        if (a.displayAnim > 0.0f)
        {
            a.displayAnim -= 0.02f;
            if (a.displayAnim < 0.0f)
                a.displayAnim = 0.0f;
        }
    }

    // Step timeout
    if (inSequence && stepWaitingForInput)
    {
        int dt = now - stepStartTimeMs;
        if (dt > BAD_MS + 300)
        {
            sequence[seqIdx].result = -1;
            sequence[seqIdx].displayAnim = 1.2f;
            combo = 0;
            seqIdx++;
            stepWaitingForInput = false;
            stepStartTimeMs = now + 120;
        }
    }

    // Movement animation
    if (player.moving)
    {
        float t = (now - player.moveStartTimeMs) / (float)MOVE_DURATION;
        if (t >= 1.0f)
        {
            player.moving = false;
            player.x = nodes[player.moveTo].x;
            player.y = nodes[player.moveTo].y;
        }
        else
        {
            float s = t * t * (3 - 2 * t); // ease
            player.x = nodes[player.moveFrom].x * (1.0f - s) + nodes[player.moveTo].x * s;
            player.y = nodes[player.moveFrom].y * (1.0f - s) + nodes[player.moveTo].y * s;
        }
    }

    glutPostRedisplay();
}

// ---------- Input ----------
void attemptMoveToHighlightedNeighbor()
{
    int nidx = getHighlightedNeighbor();
    if (nidx < 0)
        return;
    player.targetNode = nidx;
    inSequence = true;
    generateSequence(4);
    playSFX("Engage.wav");
}

// Arrow input
void specialKeys(int key, int x, int y)
{
    if (inSequence)
    {
        ArrowDir pressed;
        if (key == GLUT_KEY_UP)
            pressed = ARROW_UP;
        else if (key == GLUT_KEY_DOWN)
            pressed = ARROW_DOWN;
        else if (key == GLUT_KEY_LEFT)
            pressed = ARROW_LEFT;
        else
            pressed = ARROW_RIGHT;

        if (seqIdx >= (int)sequence.size())
            return;

        int dt = glutGet(GLUT_ELAPSED_TIME) - stepStartTimeMs;

        if (sequence[seqIdx].dir == pressed)
        {
            int tier = 0;
            if (dt <= PERFECT_MS)
                tier = 1;
            else if (dt <= GOOD_MS)
                tier = 2;
            else if (dt <= BAD_MS)
                tier = 3;
            else
                tier = -1;

            if (tier == 1)
            {
                score += 3;
                combo++;
                sequence[seqIdx].result = 1;
                playSFX("Success.wav");
            }
            else if (tier == 2)
            {
                score += 2;
                combo++;
                sequence[seqIdx].result = 2;
                playSFX("Success.wav");
            }
            else if (tier == 3)
            {
                score += 1;
                combo++;
                sequence[seqIdx].result = 3;
                playSFX("Success.wav");
            }
            else
            {
                sequence[seqIdx].result = -1;
                combo = 0;
                score = std::max(0, score - 1);
                playSFX("Mistake.wav");
            }
        }
        else
        {
            sequence[seqIdx].result = -1;
            sequence[seqIdx].displayAnim = 1.2f;
            combo = 0;
            score = std::max(0, score - 1);
            playSFX("Mistake.wav");
        }

        if (combo > maxCombo)
            maxCombo = combo;
        sequence[seqIdx].displayAnim = 1.2f;
        seqIdx++;
        stepWaitingForInput = false;

        if (seqIdx < (int)sequence.size())
        {
            stepStartTimeMs = glutGet(GLUT_ELAPSED_TIME) + 100;
            stepWaitingForInput = true;
        }
        else
        {
            // sequence finished
            bool anyMiss = false;
            for (auto &a : sequence)
                if (a.result == -1)
                    anyMiss = true;
            if (!anyMiss && player.targetNode >= 0)
            {
                nodes[player.targetNode].visited = true;
                if (nodes[player.targetNode].hasReward)
                {
                    score += 5;
                    nodes[player.targetNode].hasReward = false;
                }
                startMoveAnimation(player.currentNode, player.targetNode);
                player.currentNode = player.targetNode;
                player.targetNode = -1;
            }
            inSequence = false;
            sequence.clear();
            seqIdx = 0;
            stepWaitingForInput = false;
        }

        glutPostRedisplay();
        return;
    }

    // Not in sequence: navigate neighbors
    if (key == GLUT_KEY_LEFT || key == GLUT_KEY_DOWN)
    {
        player.highlightedNeighbor--;
        clampHighlightedNeighbor();
    }
    if (key == GLUT_KEY_RIGHT || key == GLUT_KEY_UP)
    {
        player.highlightedNeighbor++;
        clampHighlightedNeighbor();
    }
    glutPostRedisplay();
}

void keyboard(unsigned char k, int x, int y)
{
    if (k == 27)
        exit(0);
    if (k == 'r' || k == 'R')
    {
        score = combo = maxCombo = 0;
        makeDemoMap();
        inSequence = false;
        sequence.clear();
        player.targetNode = -1;
        glutPostRedisplay();
        playBackgroundMusic();
    }
    if (k == 'e' || k == 'E')
    {
        if (inSequence)
        {
            inSequence = false;
            sequence.clear();
            seqIdx = 0;
            player.targetNode = -1;
            stepWaitingForInput = false;
        }
        else
            attemptMoveToHighlightedNeighbor();
        glutPostRedisplay();
    }
    if (!inSequence)
    {
        if (k == 'a' || k == 'A')
        {
            player.highlightedNeighbor--;
            clampHighlightedNeighbor();
            playSFX("Select.wav");
            glutPostRedisplay();
        }
        if (k == 'd' || k == 'D')
        {
            player.highlightedNeighbor++;
            clampHighlightedNeighbor();
            playSFX("Select.wav");
            glutPostRedisplay();
        }
    }
}

// ---------- Draw Avatar (spinning) ----------
void drawAvatar(float x, float y)
{
    float radius = 0.05f;
    int sides = 6;
    float spinSpeed = 180.0f; // deg/sec
    float angle = fmod(glutGet(GLUT_ELAPSED_TIME) * spinSpeed / 1000.0f, 360.0f);

    glColor3f(0.9f, 0.2f, 0.9f);
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);
    glBegin(GL_POLYGON);
    for (int i = 0; i < sides; i++)
    {
        float a = i / (float)sides * 2.0f * 3.1415926f;
        glVertex2f(cosf(a) * radius, sinf(a) * radius);
    }
    glEnd();
    glPopMatrix();
}

// ---------- Draw Map ----------
void drawBackgroundGrid()
{
    glColor4f(0.10f, 0.12f, 0.16f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -1.0f; x <= 1.01f; x += 0.06f)
    {
        glVertex2f(x, -1.0f);
        glVertex2f(x, 1.0f);
    }
    for (float y = -1.0f; y <= 1.01f; y += 0.06f)
    {
        glVertex2f(-1.0f, y);
        glVertex2f(1.0f, y);
    }
    glEnd();
}

void drawMap()
{
    drawBackgroundGrid();

    // links
    glLineWidth(3.0f);
    for (size_t i = 0; i < nodes.size(); i++)
    {
        for (int j : nodes[i].links)
        {
            if (j < (int)i)
                continue;
            glColor3f(0.6f, 0.75f, 0.9f);
            glBegin(GL_LINES);
            glVertex2f(nodes[i].x, nodes[i].y);
            glVertex2f(nodes[j].x, nodes[j].y);
            glEnd();
        }
    }

    // nodes
    for (size_t i = 0; i < nodes.size(); i++)
    {
        float x = nodes[i].x, y = nodes[i].y;
        if ((int)i == player.currentNode)
            glColor3f(0.12f, 0.45f, 0.85f), drawRing(x, y, 0.12f, 0.08f);
        if (nodes[i].visited)
            glColor3f(0.22f, 0.45f, 0.85f), drawCircle(x, y, 0.06f);
        else
            glColor3f(0.95f, 0.95f, 0.95f), drawCircle(x, y, 0.05f);
        if (nodes[i].hasReward)
            glColor3f(1.0f, 0.9f, 0.3f), drawCircle(x + 0.06f, y + 0.06f, 0.02f);
    }

    // highlight neighbor
    if (!nodes[player.currentNode].links.empty())
    {
        clampHighlightedNeighbor();
        int n = nodes[player.currentNode].links[player.highlightedNeighbor];
        glColor3f(0.4f, 1.0f, 0.5f);
        drawRing(nodes[n].x, nodes[n].y, 0.10f, 0.065f);
    }
}

// ---------- Sequence UI ----------
void drawTexturedQuad(float cx, float cy, float w, float h, float alpha = 1.0f)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, uiTex);
    glColor4f(1, 1, 1, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(cx - w / 2, cy - h / 2);
    glTexCoord2f(1, 0);
    glVertex2f(cx + w / 2, cy - h / 2);
    glTexCoord2f(1, 1);
    glVertex2f(cx + w / 2, cy + h / 2);
    glTexCoord2f(0, 1);
    glVertex2f(cx - w / 2, cy + h / 2);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void drawSequenceUI()
{
    float cx = 0.0f, cy = 0.55f, w = 0.6f, h = 0.2f;
    drawTexturedQuad(cx, cy, w, h, 0.95f);

    // header
    glColor3f(0.15f, 0.5f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(cx - w / 2, cy + h / 2 - 0.03f);
    glVertex2f(cx + w / 2, cy + h / 2 - 0.03f);
    glVertex2f(cx + w / 2, cy + h / 2);
    glVertex2f(cx - w / 2, cy + h / 2);
    glEnd();
    glColor3f(1, 1, 1);
    drawString("BUILD PATH", cx - w / 2 + 0.02f, cy + h / 2 - 0.02f);

    // arrows
    float startX = cx - 0.15f;
    for (size_t i = 0; i < sequence.size(); i++)
    {
        float px = startX + i * 0.12f, py = cy;
        float animScale = 1.0f + 0.12f * sequence[i].displayAnim;
        drawArrowShape(px, py, sequence[i].dir, sequence[i].result, 0.07f, animScale);
    }
}

// ---------- HUD ----------
void displayHUD()
{
    char buf[128];
    glColor3f(1, 1, 1);
    drawString("Controls: A/D or Left/Right to pick neighbor | E: move | R: restart | ESC: quit", -0.95f, 0.95f);
    sprintf(buf, "Score: %d Combo: %d MaxCombo: %d", score, combo, maxCombo);
    drawString(buf, -0.95f, 0.92f);
    if (inSequence)
        drawString("Sequence active: press arrows!", -0.95f, 0.89f);
    if (player.moving)
        drawString("Moving...", 0.75f, 0.9f);
}

// ---------- Display & Reshape ----------
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    displayHUD();
    drawMap();
    drawAvatar(player.x, player.y);
    if (inSequence)
        drawSequenceUI();
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    windowW = w;
    windowH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float asp = (float)w / (float)h;
    if (asp >= 1.0f)
        glOrtho(-asp, asp, -1, 1, -1, 1);
    else
        glOrtho(-1, 1, -1.0f / asp, 1.0f / asp, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// ---------- Timer ----------
void timerFunc(int)
{
    updateGame();
    glutTimerFunc(16, timerFunc, 0);
}



// ---------- Main ----------
int main(int argc, char **argv)
{
    srand((unsigned)time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(windowW, windowH);
    glutCreateWindow("Node Sequence v2");

    playBackgroundMusic();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutReshapeFunc(reshape);

    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    uiTex = makeProceduralUITexture(256, 64);
    makeDemoMap();

    glutTimerFunc(16, timerFunc, 0);
    glutMainLoop();
    return 0;
}
