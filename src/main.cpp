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
#include <string>
#include <random>
#include <map>

#pragma comment(lib, "winmm.lib")

// ---------- Constants ----------
const int TIMING_PERFECT_MS = 300;
const int TIMING_GOOD_MS = 600;
const int TIMING_BAD_MS = 1000;
const int MOVE_DURATION_MS = 600;

const int STARTING_LIVES = 3;

const float NODE_RADIUS_VISITED = 0.06f;
const float NODE_RADIUS_UNVISITED = 0.05f;
const float NODE_RADIUS_REWARD = 0.02f;
const float NODE_RING_CURRENT_OUT = 0.12f;
const float NODE_RING_CURRENT_IN = 0.08f;
const float NODE_RING_HIGHLIGHT_OUT = 0.10f;
const float NODE_RING_HIGHLIGHT_IN = 0.065f;
const float NODE_RING_FINAL_OUT = 0.14f;
const float NODE_RING_FINAL_IN = 0.09f;

const float PLAYER_AVATAR_RADIUS = 0.05f;
const int   PLAYER_AVATAR_SIDES = 6;
const float PLAYER_AVATAR_SPIN_SPEED = 180.0f; // deg/sec

const float ARROW_DISPLAY_SIZE = 0.07f;
const float ARROW_UI_X_SPACING = 0.12f;
const float ARROW_UI_POS_Y = 0.55f;

const float FONT_SCALE_MEDIUM = 0.00016f;
const float FONT_SCALE_LARGE = 0.00022f;
const float FONT_SCALE_XLARGE = 0.0003f;


// ---------- Enums & Structs ----------

enum GameState
{
    STATE_MENU,
    STATE_PLAYING,
    STATE_VIEW_PASSWORDS 
};

enum ArrowDir
{
    ARROW_UP = 0,
    ARROW_DOWN = 1,
    ARROW_LEFT = 2,
    ARROW_RIGHT = 3
};

enum NodeType
{
    NODE_NORMAL,
    NODE_REWARD, // Now grants an extra life
    NODE_HAZARD,
    NODE_FAST
};

enum NodeIcon
{
    ICON_NONE,
    ICON_REWARD,
    ICON_HAZARD,
    ICON_FAST,
    ICON_KEY,
    ICON_LOCK
};

struct Arrow
{
    ArrowDir dir;
    int result = 0;
    float displayAnim = 1.0f;
};

struct Node
{
    float x, y;
    std::vector<int> links;
    bool visited = false;
    NodeType type = NODE_NORMAL;
    bool isFinal = false;

    // Lock and Key system
    bool isLocked = false;
    bool isKey = false;
    int keyNode = -1; // If this is a lock, stores the ID of its key
    int lockNode = -1; // If this is a key, stores the ID of its lock
    std::vector<ArrowDir> password; // Stores the password for this lock
};

struct Player
{
    int currentNode = 0;
    int highlightedNeighbor = -1; // MODIFIED: -1 means 'self' is highlighted
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

int playerLives = STARTING_LIVES;

int windowW = 960, windowH = 600;
GLuint uiTex = 0;

bool finalNodeSpawned = false;
bool gameWon = false;
bool isGameOver = false;
int finalNodeIndex = -1;

std::string gameMessage = "";
float gameMessageTimer = 0.0f;

std::mt19937 rng(static_cast<unsigned int>(std::time(0)));

bool currentSequenceIsHazard = false;
bool currentSequenceIsFast = false;

GameState currentGameState = STATE_MENU;
int musicVolume = 70; // 0-100
bool sfxEnabled = true;
int menuSelectedItem = 0;
bool gameInProgress = false;

float screenAspect = 1.6f;

bool displayingPassword = false;
bool isPasswordInput = false;
bool justRevealedPassword = false;

std::map<int, std::vector<ArrowDir>> revealedPasswords;


// ---------- Utility ----------

void drawString(const char* s, float x, float y, float scale)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    glLineWidth(2.0f);
    for (const char* p = s; *p; ++p)
    {
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *p);
    }
    glPopMatrix();
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

void drawQuad(float x1, float y1, float x2, float y2)
{
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

void setGameMessage(const std::string& msg, float duration = 2.0f)
{
    gameMessage = msg;
    gameMessageTimer = duration;
}

// ---------- Sound ----------
void updateMusicVolume()
{
    int mciVol = musicVolume * 10;
    std::string cmd = "setaudio music volume to " + std::to_string(mciVol);
    mciSendString(cmd.c_str(), NULL, 0, NULL);
}

void playBackgroundMusic()
{
    mciSendString(TEXT("open \"Dispatch - Gristle.wav\" type mpegvideo alias music"), NULL, 0, NULL);
    mciSendString(TEXT("play music repeat"), NULL, 0, NULL);
    updateMusicVolume();
}

void playSFX(const char* file)
{
    if (!sfxEnabled) return;
    PlaySound(file, NULL, SND_FILENAME | SND_ASYNC);
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
            pixels[idx + 0] = (unsigned char)std::min(255, r + add);
            pixels[idx + 1] = (unsigned char)std::min(255, g + add);
            pixels[idx + 2] = (unsigned char)std::min(255, b + add);
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
void drawArrowShape(float x, float y, ArrowDir dir, int result, float size = ARROW_DISPLAY_SIZE, float animScale = 1.0f)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    float ang = 0;
    if (dir == ARROW_RIGHT) ang = 0;
    else if (dir == ARROW_UP) ang = 90;
    else if (dir == ARROW_LEFT) ang = 180;
    else ang = -90;
    glRotatef(ang, 0, 0, 1);
    glScalef(size * animScale, size * animScale, 1);

    if (result == 1) glColor3f(0.1f, 1.0f, 0.2f); // Perfect
    else if (result == 2) glColor3f(0.4f, 0.8f, 0.2f); // Good
    else if (result == 3) glColor3f(1.0f, 0.7f, 0.2f); // Bad
    else if (result == -1) glColor3f(1.0f, 0.2f, 0.2f); // Miss/Wrong
    else glColor3f(0.95f, 0.95f, 0.95f); // Default

    glBegin(GL_TRIANGLES);
    glVertex2f(0.9f, 0.0f);
    glVertex2f(-0.4f, 0.55f);
    glVertex2f(-0.4f, -0.55f);
    glEnd();
    glPopMatrix();
}

void drawAvatarShape(float x, float y, float radius)
{
    int sides = PLAYER_AVATAR_SIDES;
    float angle = fmod(glutGet(GLUT_ELAPSED_TIME) * PLAYER_AVATAR_SPIN_SPEED / 1000.0f, 360.0f);

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

void drawNodeIcon(NodeIcon icon, float x, float y)
{
    float iconSize = 0.025f;
    float lineSize = 0.008f;

    switch (icon)
    {
        case ICON_REWARD:
            glColor3f(1.0f, 0.9f, 0.3f);
            drawQuad(x - iconSize, y - lineSize/2, x + iconSize, y + lineSize/2);
            drawQuad(x - lineSize/2, y - iconSize, x + lineSize/2, y + iconSize);
            break;
        case ICON_HAZARD:
            glColor3f(0.6f, 0.1f, 0.1f);
            glPushMatrix();
            glTranslatef(x, y, 0);
            glRotatef(45, 0, 0, 1);
            drawQuad(-iconSize, -lineSize/2, iconSize, lineSize/2);
            drawQuad(-lineSize/2, -iconSize, lineSize/2, iconSize);
            glPopMatrix();
            break;
        case ICON_FAST:
            glColor3f(0.8f, 0.6f, 0.0f);
            glBegin(GL_TRIANGLES);
                glVertex2f(x + 0.005f, y + 0.015f);
                glVertex2f(x + 0.02f, y);
                glVertex2f(x + 0.005f, y - 0.015f);
                glVertex2f(x - 0.015f, y + 0.015f);
                glVertex2f(x + 0.00f, y);
                glVertex2f(x - 0.015f, y - 0.015f);
            glEnd();
            break;
        case ICON_KEY:
            glColor3f(0.1f, 0.1f, 0.1f); // Black key
            drawCircle(x - 0.01f, y + 0.01f, 0.015f); // Key head
            drawQuad(x - 0.01f, y - 0.003f, x + 0.02f, y + 0.003f); // Key shaft
            drawQuad(x + 0.015f, y - 0.01f, x + 0.02f, y + 0.003f); // Key tooth
            break;
        case ICON_LOCK:
            glColor3f(1.0f, 0.3f, 0.3f);
            drawQuad(x - 0.015f, y - 0.02f, x + 0.015f, y + 0.005f); // Lock body
            drawRing(x, y + 0.005f, 0.01f, 0.007f, 16); // Shackle
            break;
        default:
            break;
    }
}


// ---------- Map ----------
void makeDemoMap()
{
    nodes.clear();
    const int N = 9;
    nodes.resize(N);

    std::uniform_real_distribution<float> posDistX(-0.9f, 0.9f);
    std::uniform_real_distribution<float> posDistY(-0.85f, 0.85f);
    std::uniform_int_distribution<int> nodeDist(0, N - 1);
    std::uniform_int_distribution<int> linkDist(1, 3);
    std::uniform_int_distribution<int> typeDist(0, 10);
    std::uniform_int_distribution<int> arrowDist(0, 3);

    const float MIN_DIST = 0.14f;
    auto placeNode = [&](int idx) {
        int attempts = 0;
        while (attempts++ < 500)
        {
            float x = posDistX(rng);
            float y = posDistY(rng);
            bool ok = true;
            for (int j = 0; j < idx; ++j)
            {
                float dx = x - nodes[j].x;
                float dy = y - nodes[j].y;
                if (sqrtf(dx * dx + dy * dy) < MIN_DIST) { ok = false; break; }
            }
            if (ok) { nodes[idx].x = x; nodes[idx].y = y; return; }
        }
        float gx = -0.8f + (idx % 3) * 0.8f;
        float gy = 0.6f - (idx / 3) * 0.6f;
        nodes[idx].x = gx; nodes[idx].y = gy;
    };

    for (int i = 0; i < N; ++i)
    {
        nodes[i].links.clear();
        nodes[i].visited = false;
        nodes[i].isFinal = false;
        nodes[i].isLocked = false;
        nodes[i].isKey = false;
        nodes[i].keyNode = -1;
        nodes[i].lockNode = -1;
        nodes[i].password.clear();

        int typeRoll = typeDist(rng);
        if (typeRoll == 0) nodes[i].type = NODE_HAZARD;
        else if (typeRoll == 1) nodes[i].type = NODE_FAST;
        else if (typeRoll == 2 || typeRoll == 3) nodes[i].type = NODE_REWARD;
        else nodes[i].type = NODE_NORMAL;

        placeNode(i);
    }

    auto orient = [](float ax, float ay, float bx, float by, float cx, float cy) {
        return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    };
    auto segmentsIntersect = [&](int a, int b, int c, int d) {
        if (a == c || a == d || b == c || b == d) return false;
        float ax = nodes[a].x, ay = nodes[a].y, bx = nodes[b].x, by = nodes[b].y;
        float cx = nodes[c].x, cy = nodes[c].y, dx = nodes[d].x, dy = nodes[d].y;
        float o1 = orient(ax, ay, bx, by, cx, cy);
        float o2 = orient(ax, ay, bx, by, dx, dy);
        float o3 = orient(cx, cy, dx, dy, ax, ay);
        float o4 = orient(cx, cy, dx, dy, bx, by);
        return (o1 * o2 < 0.0f) && (o3 * o4 < 0.0f);
    };
    auto edgeWouldCross = [&](int a, int b) {
        for (int i = 0; i < (int)nodes.size(); ++i)
        {
            for (int j : nodes[i].links)
            {
                if (i < j && segmentsIntersect(a, b, i, j))
                    return true;
            }
        }
        return false;
    };

    for (int i = 0; i < N; ++i)
    {
        int desired = linkDist(rng);
        int attempts = 0;
        while ((int)nodes[i].links.size() < desired && attempts++ < 200)
        {
            int j = nodeDist(rng);
            if (j == i) continue;
            bool already = false;
            for (int k : nodes[i].links) if (k == j) { already = true; break; }
            if (already) continue;
            if ((int)nodes[j].links.size() >= 3) continue;
            if (edgeWouldCross(i, j)) {
                if (attempts % 8 != 0) continue;
            }
            nodes[i].links.push_back(j);
            nodes[j].links.push_back(i);
        }
    }

    std::vector<int> comp(N, -1);
    int compId = 0;
    for (int i = 0; i < N; ++i)
    {
        if (comp[i] != -1) continue;
        std::vector<int> q = { i };
        comp[i] = compId;
        for (size_t qi = 0; qi < q.size(); ++qi)
        {
            int u = q[qi];
            for (int v : nodes[u].links)
            {
                if (comp[v] == -1) { comp[v] = compId; q.push_back(v); }
            }
        }
        compId++;
    }
    if (compId > 1)
    {
        for (int targetComp = 1; targetComp < compId; ++targetComp)
        {
            bool connected = false;
            for (int a = 0; a < N && !connected; ++a)
            {
                if (comp[a] != 0) continue;
                for (int b = 0; b < N && !connected; ++b)
                {
                    if (comp[b] != targetComp) continue;
                    if (!edgeWouldCross(a, b) || (rng() % 3 == 0))
                    {
                        nodes[a].links.push_back(b);
                        nodes[b].links.push_back(a);
                        int from = comp[b], to = comp[a];
                        for (int k = 0; k < N; ++k) if (comp[k] == from) comp[k] = to;
                        connected = true;
                    }
                }
            }
            if (!connected)
            {
                for (int a = 0; a < N && !connected; ++a)
                {
                    if (comp[a] != 0) continue;
                    for (int b = 0; b < N && !connected; ++b)
                    {
                        if (comp[b] != targetComp) continue;
                        nodes[a].links.push_back(b);
                        nodes[b].links.push_back(a);
                        int from = comp[b], to = comp[a];
                        for (int k = 0; k < N; ++k) if (comp[k] == from) comp[k] = to;
                        connected = true;
                    }
                }
            }
        }
    }
    
    // --- Add Lock and Key ---
    int lockNode = nodeDist(rng);
    int keyNode = nodeDist(rng);
    while (lockNode == keyNode || nodes[lockNode].type != NODE_NORMAL || nodes[keyNode].type != NODE_NORMAL) {
        lockNode = nodeDist(rng);
        keyNode = nodeDist(rng);
    }
    nodes[lockNode].isLocked = true;
    nodes[lockNode].keyNode = keyNode;
    nodes[keyNode].isKey = true;
    nodes[keyNode].lockNode = lockNode;

    for (int i = 0; i < 3; ++i) {
        nodes[lockNode].password.push_back((ArrowDir)arrowDist(rng));
    }
    // --- End ---


    finalNodeSpawned = false;
    gameWon = false;
    isGameOver = false;
    finalNodeIndex = -1;
    gameMessage = "";
    gameMessageTimer = 0.0f;
    revealedPasswords.clear();

    for (auto& n : nodes) n.visited = false;
    player.currentNode = nodeDist(rng);
    while(nodes[player.currentNode].isKey || nodes[player.currentNode].isLocked || nodes[player.currentNode].isFinal)
    {
        player.currentNode = nodeDist(rng);
    }

    player.x = nodes[player.currentNode].x;
    player.y = nodes[player.currentNode].y;
    player.highlightedNeighbor = -1; // NEW: Default to highlighting self
    player.targetNode = -1;
    player.moving = false;
}

void resetGame()
{
    playerLives = STARTING_LIVES;
    makeDemoMap();
    inSequence = false;
    sequence.clear();
    player.targetNode = -1;
    finalNodeSpawned = false;
    gameWon = false;
    isGameOver = false;
    finalNodeIndex = -1;
    gameInProgress = true;
    setGameMessage("New Game Started!", 1.5f);

    mciSendString(TEXT("stop music"), NULL, 0, NULL);
    mciSendString(TEXT("close music"), NULL, 0, NULL);
    playBackgroundMusic();
}


void spawnFinalNode()
{
    std::uniform_real_distribution<float> posDistX(-0.9f, 0.9f);
    std::uniform_real_distribution<float> posDistY(-0.85f, 0.85f);
    const float MIN_DIST = 0.16f;
    int attempts = 0;
    Node fn;
    fn.visited = false;
    fn.type = NODE_NORMAL;
    fn.isFinal = true;
    while (attempts++ < 500)
    {
        float x = posDistX(rng);
        float y = posDistY(rng);
        bool ok = true;
        for (auto& n : nodes)
        {
            float dx = x - n.x, dy = y - n.y;
            if (sqrtf(dx * dx + dy * dy) < MIN_DIST) { ok = false; break; }
        }
        if (ok) { fn.x = x; fn.y = y; break; }
    }
    if (attempts >= 500) { fn.x = 0.7f; fn.y = 0.7f; }

    int idx = (int)nodes.size();
    nodes.push_back(fn);

    std::uniform_int_distribution<int> nodeDist(0, (int)nodes.size() - 2);
    int link1 = nodeDist(rng);
    
    nodes[idx].links.push_back(link1);
    nodes[link1].links.push_back(idx);

    int link2 = nodeDist(rng);
    int linkAttempts = 0;
    while (link2 == link1 && linkAttempts < 50)
    {
        link2 = nodeDist(rng);
        linkAttempts++;
    }

    if (link1 != link2)
    {
        nodes[idx].links.push_back(link2);
        nodes[link2].links.push_back(idx);
    }
    
    finalNodeSpawned = true;
    finalNodeIndex = idx;
    playSFX("Engage.wav");
    setGameMessage("The Final Node has appeared!");
}

// MODIFIED: To handle new range -1 to n-1
void clampHighlightedNeighbor()
{
    if (nodes.empty() || player.currentNode < 0 || player.currentNode >= (int)nodes.size()) {
        player.highlightedNeighbor = -1;
        return;
    }
    int n = (int)nodes[player.currentNode].links.size();
    if (n == 0) {
        player.highlightedNeighbor = -1; // Only self is selectable
        return;
    }
    
    // Allow range from -1 (self) to n-1 (last neighbor)
    if (player.highlightedNeighbor < -1) {
        player.highlightedNeighbor = n - 1; // Wrap from self to last neighbor
    }
    if (player.highlightedNeighbor >= n) {
        player.highlightedNeighbor = -1; // Wrap from last neighbor to self
    }
}

void startMoveAnimation(int from, int to)
{
    if (from < 0 || from >= (int)nodes.size() || to < 0 || to >= (int)nodes.size()) return;
    player.moveFrom = from;
    player.moveTo = to;
    player.moveStartTimeMs = glutGet(GLUT_ELAPSED_TIME);
    player.moving = true;
    playSFX("Move.wav");
}

void attemptMoveToHighlightedNeighbor()
{
    if (player.moving) return;
    // NEW: Check if highlighting self
    if (player.highlightedNeighbor == -1)
    {
        playSFX("Mistake.wav"); // Can't move to self
        return;
    }
    
    if (player.currentNode < 0 || player.currentNode >= (int)nodes.size()) return;

    int linkCount = (int)nodes[player.currentNode].links.size();
    if (linkCount == 0)
    {
        playSFX("Mistake.wav");
        return;
    }

    clampHighlightedNeighbor();
    int neighbor = nodes[player.currentNode].links[player.highlightedNeighbor];
    Node& targetNode = nodes[neighbor];
    
    if (targetNode.isLocked)
    {
        player.targetNode = neighbor;
        inSequence = true;
        isPasswordInput = true;
        seqIdx = 0;
        sequence.clear();
        setGameMessage("Enter Password!", 2.0f);
        playSFX("Engage.wav");
        return;
    }

    player.targetNode = neighbor;

    currentSequenceIsHazard = false;
    currentSequenceIsFast = false;
    NodeType targetType = targetNode.type;

    if (targetNode.isFinal) setGameMessage("Final Sequence!");
    else if (targetType == NODE_HAZARD)
    {
        currentSequenceIsHazard = true;
        currentSequenceIsFast = true; // Hazard nodes are also fast
        setGameMessage("HAZARD! Fast Timing & Perfect/Good only!");
    }
    else if (targetType == NODE_FAST)
    {
        currentSequenceIsFast = true;
        setGameMessage("FAST! Timing is halved!");
    }
    else if (targetType == NODE_REWARD) setGameMessage("Extra Life Node!");
    else if (targetNode.isKey) setGameMessage("Key Node! Complete the sequence.");


    std::uniform_int_distribution<int> arrowDist(0, 3);
    std::uniform_int_distribution<int> lenDist(0, 2);
    int len = targetNode.isFinal ? 5 : (3 + lenDist(rng));

    sequence.clear();
    for (int i = 0; i < len; ++i)
    {
        Arrow a;
        a.dir = (ArrowDir)arrowDist(rng);
        a.result = 0;
        a.displayAnim = 1.0f;
        sequence.push_back(a);
    }

    seqIdx = 0;
    inSequence = true;
    stepWaitingForInput = true;
    stepStartTimeMs = glutGet(GLUT_ELAPSED_TIME) + 200;
    playSFX("Engage.wav");
}

// ---------- Game Update ----------
void updateGame()
{
    if (isGameOver) return;
    
    int now = glutGet(GLUT_ELAPSED_TIME);

    for (auto& a : sequence)
    {
        if (a.displayAnim > 0.0f)
        {
            a.displayAnim -= 0.02f;
            if (a.displayAnim < 0.0f) a.displayAnim = 0.0f;
        }
    }

    if (gameMessageTimer > 0.0f)
    {
        gameMessageTimer -= 0.016f;
        if (gameMessageTimer <= 0.0f) gameMessage = "";
    }

    if (!finalNodeSpawned && !gameWon)
    {
        bool allVisited = true;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (nodes[i].isFinal) continue;
            if (!nodes[i].visited) { allVisited = false; break; }
        }
        if (allVisited)
            spawnFinalNode();
    }

    int currentBadMs = currentSequenceIsFast ? (TIMING_BAD_MS / 2) : TIMING_BAD_MS;

    if (inSequence && !isPasswordInput && stepWaitingForInput)
    {
        int dt = now - stepStartTimeMs;
        if (dt > currentBadMs + 300)
        {
            // --- TIME OUT ---
            sequence[seqIdx].result = -1;
            sequence[seqIdx].displayAnim = 1.2f;
            playSFX("Mistake.wav");

            playerLives--;
            if (playerLives <= 0)
            {
                isGameOver = true;
                setGameMessage("GAME OVER. Press ESC.", 1000.0f);
            }
            else
            {
                setGameMessage("Sequence Failed! Lives: " + std::to_string(playerLives), 2.0f);
            }

            inSequence = false;
            sequence.clear();
            seqIdx = 0;
            stepWaitingForInput = false;
        }
    }

    if (player.moving)
    {
        float t = (now - player.moveStartTimeMs) / (float)MOVE_DURATION_MS;
        if (t >= 1.0f)
        {
            player.moving = false;
            player.x = nodes[player.moveTo].x;
            player.y = nodes[player.moveTo].y;
            if (player.moveTo >= 0 && player.moveTo < (int)nodes.size() && nodes[player.moveTo].isFinal)
            {
                gameWon = true;
                setGameMessage("VICTORY! Press SPACE to restart.", 1000.0f);
                playSFX("Success.wav");
            }
        }
        else
        {
            float s = t * t * (3 - 2 * t); // ease
            player.x = nodes[player.moveFrom].x * (1.0f - s) + nodes[player.moveTo].x * s;
            player.y = nodes[player.moveFrom].y * (1.0f - s) + nodes[player.moveTo].y * s;
        }
    }
}

// ---------- Input ----------

void handlePasswordInput(ArrowDir pressed)
{
    Node& lockNode = nodes[player.targetNode];
    std::vector<ArrowDir>& password = lockNode.password;

    bool correct = (password[seqIdx] == pressed);

    if (correct)
    {
        sequence.push_back({pressed, 1, 1.2f});
        seqIdx++;
        playSFX("Select.wav");

        if (seqIdx == password.size()) // --- PASSWORD COMPLETE ---
        {
            lockNode.isLocked = false;
            nodes[lockNode.keyNode].isKey = false;
            lockNode.visited = true;
            
            inSequence = false;
            isPasswordInput = false;
            
            setGameMessage("Node Unlocked!", 2.0f);
            playSFX("Engage.wav");
            
            startMoveAnimation(player.currentNode, player.targetNode);
            player.currentNode = player.targetNode;
            player.targetNode = -1;
            player.highlightedNeighbor = -1; // Default to self
        }
    }
    else // --- WRONG KEY ---
    {
        playerLives--;
        if (playerLives <= 0)
        {
            isGameOver = true;
            setGameMessage("GAME OVER. Press ESC.", 1000.0f);
        }
        else
        {
            setGameMessage("Wrong Password! Lives: " + std::to_string(playerLives), 2.0f);
        }
        
        playSFX("Mistake.wav");
        inSequence = false;
        isPasswordInput = false;
    }
    glutPostRedisplay();
}


void handleArrowPress(ArrowDir pressed)
{
    if (isPasswordInput)
    {
        handlePasswordInput(pressed);
        return;
    }

    if (seqIdx >= (int)sequence.size()) return;

    int dt = glutGet(GLUT_ELAPSED_TIME) - stepStartTimeMs;

    int currentPerfectMs = currentSequenceIsFast ? (TIMING_PERFECT_MS / 2) : TIMING_PERFECT_MS;
    int currentGoodMs = currentSequenceIsFast ? (TIMING_GOOD_MS / 2) : TIMING_GOOD_MS;
    int currentBadMs = currentSequenceIsFast ? (TIMING_BAD_MS / 2) : TIMING_BAD_MS;

    bool sequenceFailed = false;

    if (sequence[seqIdx].dir == pressed)
    {
        int tier = 0;
        if (dt <= currentPerfectMs) tier = 1;
        else if (dt <= currentGoodMs) tier = 2;
        else if (dt <= currentBadMs) tier = 3;
        else tier = -1;

        if (tier == 1) // Perfect
        {
            sequence[seqIdx].result = 1;
            playSFX("Success.wav");
        }
        else if (tier == 2) // Good
        {
            sequence[seqIdx].result = 2;
            playSFX("Success.wav");
        }
        else if (tier == 3) // Bad
        {
            if (currentSequenceIsHazard)
            {
                sequenceFailed = true;
                setGameMessage("Hazard Failed!");
            }
            else
            {
                sequence[seqIdx].result = 3;
                playSFX("Success.wav");
            }
        }
        else sequenceFailed = true; // Too late
    }
    else sequenceFailed = true; // Wrong key

    
    if (sequenceFailed)
    {
        // --- FAILURE ---
        sequence[seqIdx].result = -1;
        playSFX("Mistake.wav");
        
        playerLives--;
        if (playerLives <= 0)
        {
            isGameOver = true;
            setGameMessage("GAME OVER. Press ESC.", 1000.0f);
        }
        else
        {
            setGameMessage("Sequence Failed! Lives: " + std::to_string(playerLives), 2.0f);
        }
        
        inSequence = false;
        sequence.clear();
        seqIdx = 0;
        stepWaitingForInput = false;
        glutPostRedisplay();
        return;
    }

    // --- SUCCESS (for this note) ---
    sequence[seqIdx].displayAnim = 1.2f;
    seqIdx++;
    stepWaitingForInput = false;

    if (seqIdx < (int)sequence.size()) // More notes in sequence
    {
        stepStartTimeMs = glutGet(GLUT_ELAPSED_TIME) + 100;
        stepWaitingForInput = true;
    }
    else // --- SEQUENCE COMPLETE (All notes hit) ---
    {
        if (player.targetNode >= 0)
        {
            Node& target = nodes[player.targetNode];

            if (target.isKey)
            {
                int lockId = target.lockNode;
                // Load the password into the global sequence to display it
                sequence.clear();
                for (ArrowDir dir : nodes[lockId].password) {
                    sequence.push_back({dir, 0, 1.0f});
                }
                
                revealedPasswords[lockId] = nodes[lockId].password;

                displayingPassword = true;
                justRevealedPassword = true;
                inSequence = false;
                stepWaitingForInput = false;
                setGameMessage("Password Revealed! Press SPACE.", 1000.0f);
                glutPostRedisplay();
                return;
            }
            
            target.visited = true;
            if (target.type == NODE_REWARD)
            {
                playerLives++;
                setGameMessage("Extra Life! (Lives: " + std::to_string(playerLives) + ")", 2.0f);
                target.type = NODE_NORMAL;
            }
            
            startMoveAnimation(player.currentNode, player.targetNode);
            player.currentNode = player.targetNode;
            player.targetNode = -1;
            player.highlightedNeighbor = -1; // Default to self
        }
        
        inSequence = false;
        sequence.clear();
        seqIdx = 0;
        stepWaitingForInput = false;
    }

    glutPostRedisplay();
}


void specialKeys(int key, int x, int y)
{
    if (displayingPassword) return;
    if (isGameOver) return;
    
    if (currentGameState == STATE_MENU)
    {
        std::vector<std::string> menuItems;
        if (isGameOver) menuItems = { "New Game", "Quit" };
        else if (gameInProgress) menuItems = { "Resume Game", "View Passwords", "Music Volume", "SFX", "Restart Game", "Quit" };
        else menuItems = { "New Game", "Music Volume", "SFX", "Quit" };
        
        if (menuSelectedItem >= (int)menuItems.size()) menuSelectedItem = 0;
        int numItems = (int)menuItems.size();

        if (key == GLUT_KEY_UP)
        {
            menuSelectedItem = (menuSelectedItem - 1 + numItems) % numItems;
            playSFX("Select.wav");
        }
        else if (key == GLUT_KEY_DOWN)
        {
            menuSelectedItem = (menuSelectedItem + 1) % numItems;
            playSFX("Select.wav");
        }
        else if (key == GLUT_KEY_LEFT)
        {
            const std::string& item = menuItems[menuSelectedItem];
            if (item == "Music Volume")
            {
                musicVolume = std::max(0, musicVolume - 10);
                updateMusicVolume();
                playSFX("Select.wav");
            }
            else if (item == "SFX")
            {
                sfxEnabled = !sfxEnabled;
                playSFX("Select.wav");
            }
        }
        else if (key == GLUT_KEY_RIGHT)
        {
            const std::string& item = menuItems[menuSelectedItem];
            if (item == "Music Volume")
            {
                musicVolume = std::min(100, musicVolume + 10);
                updateMusicVolume();
                playSFX("Select.wav");
            }
            else if (item == "SFX")
            {
                sfxEnabled = !sfxEnabled;
                playSFX("Select.wav");
            }
        }
        glutPostRedisplay();
        return;
    }

    if (currentGameState != STATE_PLAYING) return;
    if (gameWon) return;

    if (inSequence && (stepWaitingForInput || isPasswordInput))
    {
        ArrowDir pressed;
        if (key == GLUT_KEY_UP) pressed = ARROW_UP;
        else if (key == GLUT_KEY_DOWN) pressed = ARROW_DOWN;
        else if (key == GLUT_KEY_LEFT) pressed = ARROW_LEFT;
        else if (key == GLUT_KEY_RIGHT) pressed = ARROW_RIGHT;
        else return;
        handleArrowPress(pressed);
        return;
    }
    
    if (!inSequence)
    {
        if (key == GLUT_KEY_LEFT)
        {
            player.highlightedNeighbor--;
            clampHighlightedNeighbor();
        }
        if (key == GLUT_KEY_RIGHT)
        {
            player.highlightedNeighbor++;
            clampHighlightedNeighbor();
        }
        glutPostRedisplay();
        return;
    }
}

void keyboard(unsigned char k, int x, int y)
{
    if (k == 27) // ESC key
    {
        if (displayingPassword)
        {
            return;
        }
        if (currentGameState == STATE_VIEW_PASSWORDS)
        {
            currentGameState = STATE_MENU;
            glutPostRedisplay();
            return;
        }
        if (isGameOver)
        {
            currentGameState = STATE_MENU;
            setGameMessage("", 0.0f);
            isGameOver = false;
        }
        else if (currentGameState == STATE_PLAYING)
        {
            currentGameState = STATE_MENU;
            setGameMessage("Paused", 1.0f);
        }
        else if (currentGameState == STATE_MENU && gameInProgress)
        {
            currentGameState = STATE_PLAYING;
            setGameMessage("", 0.0f);
        }
        glutPostRedisplay();
        return;
    }

    if (displayingPassword)
    {
        if (k == ' ' || k == 13)
        {
            displayingPassword = false;
            
            if (justRevealedPassword)
            {
                Node& keyNode = nodes[player.targetNode];
                keyNode.visited = true;
                
                startMoveAnimation(player.currentNode, player.targetNode);
                player.currentNode = player.targetNode;
                player.targetNode = -1;
                player.highlightedNeighbor = -1; // Default to self
            }
            glutPostRedisplay();
        }
        return;
    }
    
    if (isGameOver) return;
    
    if (currentGameState == STATE_MENU)
    {
        std::vector<std::string> menuItems;
        if (isGameOver) menuItems = { "New Game", "Quit" };
        // MODIFIED: Removed "View Passwords" from here
        else if (gameInProgress) menuItems = { "Resume Game", "Music Volume", "SFX", "Restart Game", "Quit" };
        else menuItems = { "New Game", "Music Volume", "SFX", "Quit" };

        if (menuSelectedItem >= (int)menuItems.size()) menuSelectedItem = 0;
        int numItems = (int)menuItems.size();

        if (k == 'w' || k == 'W')
        {
            menuSelectedItem = (menuSelectedItem - 1 + numItems) % numItems;
            playSFX("Select.wav");
        }
        else if (k == 's' || k == 'S')
        {
            menuSelectedItem = (menuSelectedItem + 1) % numItems;
            playSFX("Select.wav");
        }
        else if (k == 'a' || k == 'A')
        {
            const std::string& item = menuItems[menuSelectedItem];
            if (item == "Music Volume")
            {
                musicVolume = std::max(0, musicVolume - 10);
                updateMusicVolume();
                playSFX("Select.wav");
            }
            else if (item == "SFX")
            {
                sfxEnabled = !sfxEnabled;
                playSFX("Select.wav");
            }
        }
        else if (k == 'd' || k == 'D')
        {
            const std::string& item = menuItems[menuSelectedItem];
            if (item == "Music Volume")
            {
                musicVolume = std::min(100, musicVolume + 10);
                updateMusicVolume();
                playSFX("Select.wav");
            }
            else if (item == "SFX")
            {
                sfxEnabled = !sfxEnabled;
                playSFX("Select.wav");
            }
        }
        else if (k == ' ' || k == 13)
        {
            const std::string& item = menuItems[menuSelectedItem];
            playSFX("Engage.wav");
            if (item == "New Game")
            {
                resetGame();
                currentGameState = STATE_PLAYING;
            }
            else if (item == "Resume Game")
            {
                currentGameState = STATE_PLAYING;
                setGameMessage("", 0.0f);
            }
            // MODIFIED: Removed "View Passwords"
            else if (item == "Restart Game")
            {
                resetGame();
                currentGameState = STATE_PLAYING;
            }
            else if (item == "Quit") exit(0);
        }
        glutPostRedisplay();
        return;
    }

    if (currentGameState == STATE_PLAYING)
    {
        if (gameWon)
        {
            if (k == ' ' || k == 13) resetGame();
            return;
        }

        if (inSequence && (stepWaitingForInput || isPasswordInput))
        {
            // NEW: Allow SPACE to cancel password input
            if (isPasswordInput && (k == ' ' || k == 13))
            {
                inSequence = false;
                isPasswordInput = false;
                sequence.clear();
                seqIdx = 0;
                player.targetNode = -1;
                stepWaitingForInput = false;
                setGameMessage("Password Cancelled", 1.0f);
                glutPostRedisplay();
                return;
            }

            ArrowDir pressed;
            if (k == 'w' || k == 'W') pressed = ARROW_UP;
            else if (k == 's' || k == 'S') pressed = ARROW_DOWN;
            else if (k == 'a' || k == 'A') pressed = ARROW_LEFT;
            else if (k == 'd' || k == 'D') pressed = ARROW_RIGHT;
            else return;
            handleArrowPress(pressed);
            return;
        }

        if (k == ' ' || k == 13)
        {
            if (inSequence) // This now only cancels normal sequences
            {
                inSequence = false;
                isPasswordInput = false;
                sequence.clear();
                seqIdx = 0;
                player.targetNode = -1;
                stepWaitingForInput = false;
                setGameMessage("Sequence Cancelled", 1.0f);
            }
            else
            {
                // MODIFIED: Handle spacebar on self-node
                if (player.highlightedNeighbor == -1) // Player is highlighting self
                {
                    Node& selfNode = nodes[player.currentNode];
                    int lockId = -1;
                    if (selfNode.isKey) lockId = selfNode.lockNode;
                    
                    if (lockId != -1 && revealedPasswords.count(lockId))
                    {
                        // View password
                        sequence.clear();
                        for (ArrowDir dir : revealedPasswords[lockId]) {
                            sequence.push_back({dir, 0, 1.0f});
                        }
                        displayingPassword = true;
                        justRevealedPassword = false;
                        setGameMessage("Viewing Password. Press SPACE.", 1000.0f);
                    }
                    else if (selfNode.isKey)
                    {
                        setGameMessage("Password not yet discovered!", 1.0f);
                        playSFX("Mistake.wav");
                    }
                    else
                    {
                        playSFX("Mistake.wav"); // No action on self
                    }
                }
                else // Player is highlighting a neighbor
                {
                    attemptMoveToHighlightedNeighbor();
                }
            }
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
            if (k == 'v' || k == 'V')
            {
                if (!nodes.empty())
                {
                    clampHighlightedNeighbor();
                    int targetIdx = -1;
                    if (player.highlightedNeighbor == -1)
                        targetIdx = player.currentNode;
                    else if (player.highlightedNeighbor < (int)nodes[player.currentNode].links.size())
                        targetIdx = nodes[player.currentNode].links[player.highlightedNeighbor];
                    
                    if (targetIdx != -1)
                    {
                        Node& targetNode = nodes[targetIdx];
                        int lockId = -1;
                        if (targetNode.isKey) lockId = targetNode.lockNode;
                        if (targetNode.isLocked) lockId = targetIdx;

                        if (lockId != -1)
                        {
                            if (revealedPasswords.count(lockId))
                            {
                                sequence.clear();
                                for (ArrowDir dir : revealedPasswords[lockId]) {
                                    sequence.push_back({dir, 0, 1.0f});
                                }
                                displayingPassword = true;
                                justRevealedPassword = false;
                                setGameMessage("Viewing Password. Press SPACE.", 1000.0f);
                            }
                            else
                            {
                                setGameMessage("Password not yet discovered!", 1.5f);
                            }
                        }
                        else
                        {
                            setGameMessage("Not a Lock or Key node.", 1.0f);
                        }
                    }
                }
            }
        }
    }
}

// ---------- Draw Avatar (spinning) ----------
void drawAvatar(float x, float y)
{
    float radius = PLAYER_AVATAR_RADIUS;
    int sides = PLAYER_AVATAR_SIDES;
    float angle = fmod(glutGet(GLUT_ELAPSED_TIME) * PLAYER_AVATAR_SPIN_SPEED / 1000.0f, 360.0f);

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
    float time = glutGet(GLUT_ELAPSED_TIME) / 5000.0f;
    
    glColor4f(0.10f, 0.12f, 0.16f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -2.0f; x <= 2.01f; x += 0.12f)
    {
        glVertex2f(x, -1.0f); glVertex2f(x, 1.0f);
    }
    float y_offset_far = fmod(time * 0.1f, 0.12f);
    for (float y = -1.0f; y <= 1.01f; y += 0.12f)
    {
        glVertex2f(-screenAspect, y + y_offset_far); glVertex2f(screenAspect, y + y_offset_far);
    }
    glEnd();

    glColor4f(0.15f, 0.18f, 0.24f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    float y_offset_near = fmod(time * 0.3f, 0.12f);
    for (float y = -1.0f; y <= 1.01f; y += 0.12f)
    {
        glVertex2f(-screenAspect, y + y_offset_near - 0.06f); glVertex2f(screenAspect, y + y_offset_near - 0.06f);
    }
    glEnd();
}

void drawMap()
{
    drawBackgroundGrid();

    glLineWidth(3.0f);
    float globalPulse = 0.8f + 0.2f * (1.0f + sinf(glutGet(GLUT_ELAPSED_TIME) * 0.005f));
    
    int highlightedNodeIndex = -1;
    if (!nodes.empty() && player.currentNode >= 0 && player.currentNode < (int)nodes.size() && !inSequence && !player.moving && !displayingPassword)
    {
        clampHighlightedNeighbor();
        if(player.highlightedNeighbor >= 0) // Only if highlighting a neighbor
            highlightedNodeIndex = nodes[player.currentNode].links[player.highlightedNeighbor];
    }
    
    for (size_t i = 0; i < nodes.size(); i++)
    {
        for (int j : nodes[i].links)
        {
            if (j < (int)i) continue;

            bool isPlayerLink = (i == player.currentNode || j == player.currentNode) && !player.moving && !inSequence && !displayingPassword;
            // NEW: Check if this link is connected to the highlighted node
            bool isHighlightLink = (highlightedNodeIndex != -1) && (i == highlightedNodeIndex || j == highlightedNodeIndex) && !player.moving && !inSequence && !displayingPassword;


            if (isHighlightLink)
            {
                glColor3f(0.4f * globalPulse, 1.0f * globalPulse, 0.5f * globalPulse); // Pulsing Green
            }
            else if (isPlayerLink)
            {
                glColor3f(0.6f * globalPulse, 0.75f * globalPulse, 0.9f * globalPulse); // Pulsing Blue
            }
            else
            {
                glColor3f(0.6f, 0.75f, 0.9f); // Standard link
            }
            
            glBegin(GL_LINES);
            glVertex2f(nodes[i].x, nodes[i].y);
            glVertex2f(nodes[j].x, nodes[j].y);
            glEnd();
        }
    }

    for (size_t i = 0; i < nodes.size(); i++)
    {
        float x = nodes[i].x, y = nodes[i].y;
        if (nodes[i].isFinal)
        {
            glColor3f(1.0f, 0.8f, 0.1f);
            drawRing(x, y, NODE_RING_FINAL_OUT, NODE_RING_FINAL_IN);
            glColor3f(0.95f, 0.85f, 0.2f);
            drawCircle(x, y, NODE_RADIUS_VISITED);
            glColor3f(0.05f, 0.05f, 0.05f);
            drawString("FINISH", x - 0.07f, y - 0.01f, FONT_SCALE_LARGE);
            continue;
        }

        if ((int)i == player.currentNode)
            glColor3f(0.12f, 0.45f, 0.85f), drawRing(x, y, NODE_RING_CURRENT_OUT, NODE_RING_CURRENT_IN);

        if (nodes[i].visited)
        {
            glColor3f(0.22f, 0.45f, 0.85f);
            drawCircle(x, y, NODE_RADIUS_VISITED);
        }
        else
        {
            if (nodes[i].type == NODE_HAZARD) glColor3f(1.0f, 0.3f, 0.3f);
            else if (nodes[i].type == NODE_FAST) glColor3f(1.0f, 0.8f, 0.2f);
            else glColor3f(0.95f, 0.95f, 0.95f);
            drawCircle(x, y, NODE_RADIUS_UNVISITED);
        }

        // --- Draw Icons ---
        NodeIcon icon = ICON_NONE;
        if (nodes[i].type == NODE_REWARD) icon = ICON_REWARD;
        else if (nodes[i].type == NODE_HAZARD) icon = ICON_HAZARD;
        else if (nodes[i].type == NODE_FAST) icon = ICON_FAST;
        
        if (nodes[i].isKey) icon = ICON_KEY;
        else if (nodes[i].isLocked) icon = ICON_LOCK;
        
        if (icon != ICON_NONE)
        {
            drawNodeIcon(icon, x, y);
        }
    }

    if (!nodes.empty() && player.currentNode >= 0 && player.currentNode < (int)nodes.size() && !inSequence && !player.moving && !displayingPassword)
    {
        if (highlightedNodeIndex != -1) // Highlighting a neighbor
        {
            glColor3f(0.4f, 1.0f, 0.5f);
            drawRing(nodes[highlightedNodeIndex].x, nodes[highlightedNodeIndex].y, NODE_RING_HIGHLIGHT_OUT, NODE_RING_HIGHLIGHT_IN);
        }
        else if (player.highlightedNeighbor == -1) // NEW: Highlighting self
        {
            glColor3f(0.4f, 1.0f, 0.5f); // Same green highlight
            drawRing(nodes[player.currentNode].x, nodes[player.currentNode].y, NODE_RING_HIGHLIGHT_OUT, NODE_RING_HIGHLIGHT_IN);
        }
    }
}

// ---------- Sequence UI ----------
void drawTexturedQuad(float cx, float cy, float w, float h, float alpha = 1.0f)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, uiTex);
    glColor4f(1, 1, 1, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(cx - w / 2, cy - h / 2);
    glTexCoord2f(1, 0); glVertex2f(cx + w / 2, cy - h / 2);
    glTexCoord2f(1, 1); glVertex2f(cx + w / 2, cy + h / 2);
    glTexCoord2f(0, 1); glVertex2f(cx - w / 2, cy + h / 2);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void drawPasswordDisplay()
{
    float cx = 0.0f, cy = ARROW_UI_POS_Y, w = 0.6f, h = 0.2f;
    drawTexturedQuad(cx, cy, w, h, 0.95f);

    glColor3f(0.15f, 0.5f, 0.9f);
    drawQuad(cx - w / 2, cy + h / 2 - 0.03f, cx + w / 2, cy + h / 2);
    glColor3f(1, 1, 1);
    
    if (justRevealedPassword)
        drawString("PASSWORD REVEALED", cx - 0.22f, cy + h / 2 - 0.022f, FONT_SCALE_LARGE);
    else
        drawString("VIEWING PASSWORD", cx - 0.22f, cy + h / 2 - 0.022f, FONT_SCALE_LARGE);


    float arrowY = cy - 0.01f;
    int numArrows = (int)sequence.size();
    if (numArrows == 0) return;

    float totalWidth = (numArrows - 1) * ARROW_UI_X_SPACING;
    float startX = cx - totalWidth / 2.0f;

    for (size_t i = 0; i < numArrows; i++)
    {
        float px = startX + i * ARROW_UI_X_SPACING;
        drawArrowShape(px, arrowY, sequence[i].dir, 0, ARROW_DISPLAY_SIZE, 1.0f);
    }

    drawString("Press SPACE to Continue", cx - 0.2f, cy - 0.08f, FONT_SCALE_MEDIUM);
}


void drawSequenceUI()
{
    if (isPasswordInput)
    {
        float cx = 0.0f, cy = ARROW_UI_POS_Y, w = 0.6f, h = 0.2f;
        drawTexturedQuad(cx, cy, w, h, 0.95f);

        glColor3f(0.9f, 0.2f, 0.2f);
        drawQuad(cx - w / 2, cy + h / 2 - 0.03f, cx + w / 2, cy + h / 2);
        glColor3f(1, 1, 1);
        drawString("ENTER PASSWORD", cx - 0.18f, cy + h / 2 - 0.022f, FONT_SCALE_LARGE);

        float arrowY = cy - 0.01f;
        int numArrows = (int)nodes[player.targetNode].password.size();
        if (numArrows == 0) return;

        float totalWidth = (numArrows - 1) * ARROW_UI_X_SPACING;
        float startX = cx - totalWidth / 2.0f;

        for (size_t i = 0; i < numArrows; i++)
        {
            float px = startX + i * ARROW_UI_X_SPACING;
            if (i < sequence.size()) // Arrows already entered
            {
                drawArrowShape(px, arrowY, sequence[i].dir, sequence[i].result, ARROW_DISPLAY_SIZE, sequence[i].displayAnim);
            }
            else // Not yet entered
            {
                glColor3f(0.5f, 0.5f, 0.5f);
                drawCircle(px, arrowY, 0.02f);
            }
        }
        // NEW: Add cancel prompt
        drawString("Press SPACE to Cancel", cx - 0.2f, cy - 0.08f, FONT_SCALE_MEDIUM);
        return;
    }

    // --- Normal Sequence UI ---
    float cx = 0.0f, cy = ARROW_UI_POS_Y, w = 0.6f, h = 0.2f;
    drawTexturedQuad(cx, cy, w, h, 0.95f);

    glColor3f(0.15f, 0.5f, 0.9f);
    drawQuad(cx - w / 2, cy + h / 2 - 0.03f, cx + w / 2, cy + h / 2);
    glColor3f(1, 1, 1);
    drawString("BUILD PATH", cx - 0.15f, cy + h / 2 - 0.022f, FONT_SCALE_LARGE);

    float arrowY = cy - 0.01f;
    int numArrows = (int)sequence.size();
    if (numArrows == 0) return;

    float totalWidth = (numArrows - 1) * ARROW_UI_X_SPACING;
    float startX = cx - totalWidth / 2.0f;

    for (size_t i = 0; i < numArrows; i++)
    {
        float px = startX + i * ARROW_UI_X_SPACING;
        float animScale = 1.0f + 0.12f * sequence[i].displayAnim;
        
        if (i == seqIdx && stepWaitingForInput)
        {
            float pulse = 1.0f + 0.1f * (1.0f + sinf(glutGet(GLUT_ELAPSED_TIME) * 0.008f));
            animScale *= pulse;
        }

        drawArrowShape(px, arrowY, sequence[i].dir, sequence[i].result, ARROW_DISPLAY_SIZE, animScale);
    }

    if (inSequence && stepWaitingForInput && seqIdx < (int)sequence.size())
    {
        float barY = cy + 0.04f;
        float barW = w * 0.9f;
        float barX = cx - barW / 2;
        float barH = 0.01f;

        int currentPerfectMs = currentSequenceIsFast ? (TIMING_PERFECT_MS / 2) : TIMING_PERFECT_MS;
        int currentGoodMs = currentSequenceIsFast ? (TIMING_GOOD_MS / 2) : TIMING_GOOD_MS;
        int currentBadMs = currentSequenceIsFast ? (TIMING_BAD_MS / 2) : TIMING_BAD_MS;

        float perfectP = (float)currentPerfectMs / currentBadMs;
        float goodP = (float)currentGoodMs / currentBadMs;

        glColor3f(0.1f, 1.0f, 0.2f); // Perfect
        drawQuad(barX, barY - barH, barX + barW * perfectP, barY + barH);
        glColor3f(0.4f, 0.8f, 0.2f); // Good
        drawQuad(barX + barW * perfectP, barY - barH, barX + barW * goodP, barY + barH);
        glColor3f(1.0f, 0.7f, 0.2f); // Bad
        drawQuad(barX + barW * goodP, barY - barH, barX + barW, barY + barH);

        int dt = glutGet(GLUT_ELAPSED_TIME) - stepStartTimeMs;
        float progress = (float)dt / (float)currentBadMs;
        if (progress > 1.0f) progress = 1.0f;

        glColor3f(1.0f, 0.2f, 0.2f);
        float timerX = barX + barW * progress;
        drawQuad(timerX - 0.005f, barY - barH * 2.0f, timerX + 0.005f, barY + barH * 2.0f);
    }
}

// ---------- HUD & Menu (MODIFIED) ----------
void displayHUD()
{
    float hudX = -screenAspect + 0.05f;
    float hudY = 0.95f;
    float lineH = 0.05f;

    glColor3f(1, 1, 1);
    drawString("Controls: A/D/Arrows | SPACE: action | V: view pass | ESC: menu", hudX, hudY, FONT_SCALE_MEDIUM);
    hudY -= lineH;

    std::string livesText = "Lives: " + std::to_string(playerLives);
    drawString(livesText.c_str(), hudX, hudY, FONT_SCALE_MEDIUM);
    hudY -= lineH;

    if (inSequence)
        drawString("Sequence active: (WASD / Arrows)", hudX, hudY, FONT_SCALE_MEDIUM);
    if (player.moving)
        drawString("Moving...", 0.75f, 0.9f, FONT_SCALE_MEDIUM);

    if (gameMessageTimer > 0.0f)
    {
        if (isGameOver) glColor3f(1.0f, 0.2f, 0.2f);
        else glColor3f(1.0f, 0.9f, 0.2f);
        drawString(gameMessage.c_str(), -0.6f, 0.0f, FONT_SCALE_LARGE);
    }
    if (gameWon)
    {
        glColor3f(0.2f, 1.0f, 0.3f);
        drawString("VICTORY! Press SPACE to restart", -0.7f, 0.1f, FONT_SCALE_LARGE);
    }

    // --- Legend ---
    float legendX = -screenAspect + 0.05f;
    float legendY = -0.45f;
    float legendSpacing = 0.05f;
    float legendIconSize = 0.02f;
    float legendTextX = legendX + 0.05f;
    float textYOff = -0.01f;

    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Legend:", legendX, legendY, FONT_SCALE_LARGE);
    legendY -= (legendSpacing * 1.2f);

    drawAvatarShape(legendX + legendIconSize * 0.5f, legendY, legendIconSize * 0.7f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("You", legendTextX, legendY + textYOff, FONT_SCALE_MEDIUM);
    legendY -= legendSpacing;

    glColor3f(0.95f, 0.95f, 0.95f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.5f);
    drawString("Unvisited Node", legendTextX, legendY + textYOff, FONT_SCALE_MEDIUM);
    legendY -= legendSpacing;

    glColor3f(0.22f, 0.45f, 0.85f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_VISITED * 0.5f);
    drawString("Visited Node", legendTextX, legendY + textYOff, FONT_SCALE_MEDIUM);
    legendY -= legendSpacing;

    glColor3f(0.4f, 1.0f, 0.5f);
    drawRing(legendX + legendIconSize * 0.5f, legendY, NODE_RING_HIGHLIGHT_OUT * 0.4f, NODE_RING_HIGHLIGHT_IN * 0.4f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Highlighted Target", legendTextX, legendY + textYOff, FONT_SCALE_MEDIUM);
    legendY -= legendSpacing;

    drawNodeIcon(ICON_KEY, legendX + legendIconSize * 0.5f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Key Node (Reveals Password)", legendTextX, legendY + textYOff, FONT_SCALE_MEDIUM);
    legendY -= legendSpacing;

    drawNodeIcon(ICON_LOCK, legendX + legendIconSize * 0.5f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Locked Node (Requires Password)", legendTextX, legendY + textYOff, FONT_SCALE_MEDIUM);
    legendY -= legendSpacing;

    glColor3f(1.0f, 0.9f, 0.3f);
    drawNodeIcon(ICON_REWARD, legendX + legendIconSize * 0.5f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Extra Life Node", legendTextX, legendY + textYOff, FONT_SCALE_MEDIUM);
    legendY -= legendSpacing;

    glColor3f(1.0f, 0.3f, 0.3f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.5f);
    drawNodeIcon(ICON_HAZARD, legendX + legendIconSize * 0.5f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Hazard Node (Fast & Good/Perfect!)", legendTextX, legendY + textYOff, FONT_SCALE_MEDIUM);
    legendY -= legendSpacing;

    glColor3f(1.0f, 0.8f, 0.2f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.5f);
    drawNodeIcon(ICON_FAST, legendX + 0.005f, legendY);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Fast Node (Timing halved!)", legendTextX, legendY + textYOff, FONT_SCALE_MEDIUM);
    legendY -= legendSpacing;
}

void drawPasswordScreen()
{
    glColor4f(0.0f, 0.0f, 0.0f, 0.9f);
    drawQuad(-screenAspect, -1.0f, screenAspect, 1.0f);

    float cx = -0.7f;
    float cy = 0.8f;
    float lineSpacing = 0.1f;

    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("REVEALED PASSWORDS", cx, cy, FONT_SCALE_LARGE);
    cy -= lineSpacing * 1.5f;

    if (revealedPasswords.empty())
    {
        drawString("No passwords found yet. Find a Key Node!", cx, cy, FONT_SCALE_MEDIUM);
    }
    else
    {
        for (auto const& [lockId, password] : revealedPasswords)
        {
            std::string title = "Lock " + std::to_string(lockId) + ":";
            drawString(title.c_str(), cx, cy, FONT_SCALE_MEDIUM);
            
            float arrowX = cx + 0.3f;
            for(size_t i = 0; i < password.size(); ++i)
            {
                drawArrowShape(arrowX + i * ARROW_UI_X_SPACING, cy + 0.01f, password[i], 0, ARROW_DISPLAY_SIZE, 1.0f);
            }
            cy -= lineSpacing;
        }
    }

    glColor3f(0.8f, 0.8f, 0.8f);
    drawString("Press ESC to return to Menu", cx, -0.9f, FONT_SCALE_MEDIUM);
}


void drawMenu()
{
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    drawQuad(-screenAspect, -1.0f, screenAspect, 1.0f);

    float cx = -0.7f;
    float cy = 0.5f;
    float menuSpacing = 0.15f;

    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("THISPATCHED", cx, cy, FONT_SCALE_XLARGE);
    cy -= menuSpacing * 1.5f;
    
    std::vector<std::string> menuItems;
    
    if (isGameOver)
    {
        menuItems = { "New Game", "Quit" };
    }
    else if (gameInProgress)
    {
        // MODIFIED: Removed "View Passwords"
        menuItems = { "Resume Game", "Music Volume", "SFX", "Restart Game", "Quit" };
    }
    else
    {
        menuItems = { "New Game", "Music Volume", "SFX", "Quit" };
    }

    if (menuSelectedItem >= (int)menuItems.size())
    {
        menuSelectedItem = 0;
    }

    for (size_t i = 0; i < menuItems.size(); ++i)
    {
        float y = cy - i * menuSpacing;
        
        if (i == menuSelectedItem)
        {
            glColor3f(0.1f, 1.0f, 0.2f);
            drawArrowShape(cx - 0.05f, y + 0.02f, ARROW_RIGHT, 0, 0.05f);
        }
        else glColor3f(1.0f, 1.0f, 1.0f);

        std::string itemText = menuItems[i];
        if (itemText == "Music Volume") itemText = std::string("Music Volume: < ") + std::to_string(musicVolume) + " >";
        else if (itemText == "SFX") itemText = std::string("SFX: < ") + (sfxEnabled ? "ON" : "OFF") + " >";

        drawString(itemText.c_str(), cx, y, FONT_SCALE_LARGE);
    }

    glColor3f(0.8f, 0.8f, 0.8f);
    drawString("Controls: W/S/Arrows to navigate, A/D/Arrows to change, SPACE/Enter to select", cx - 0.3f, cy - (menuItems.size() + 1) * menuSpacing, FONT_SCALE_MEDIUM);
}

// ---------- Draw Game ----------
void drawGame()
{
    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    drawMap();
    drawAvatar(player.x, player.y);
    displayHUD();
    
    if (inSequence)
        drawSequenceUI();
    else if (displayingPassword)
        drawPasswordDisplay();


    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f( 1.0f, -1.0f);
    glVertex2f( 1.0f,  1.0f);
    glVertex2f(-1.0f,  1.0f);
    glVertex2f(-1.0f, -1.0f);
    glEnd();
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ---------- Display & Reshape ----------
void display()
{
    drawGame();

    if (currentGameState == STATE_MENU)
    {
        drawMenu();
    }
    else if (currentGameState == STATE_VIEW_PASSWORDS)
    {
        drawMenu();
        drawPasswordScreen();
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

// ---------- Timer ----------
void timerFunc(int)
{
    if (currentGameState == STATE_PLAYING)
    {
        updateGame();
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0);
}

// ---------- Main ----------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowSize(windowW, windowH);
    glutCreateWindow("ThisPatched");

    try {
        HWND hwnd = FindWindow(NULL, TEXT("ThisPatched"));
        if (hwnd)
        {
            HICON hIcon = (HICON)LoadImage(NULL, TEXT("my_icon.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
            if (hIcon)
            {
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            }
        }
    }
    catch (...) { }

    playBackgroundMusic(); 
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutReshapeFunc(reshape);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    
    uiTex = makeProceduralUITexture(256, 64);
    
    makeDemoMap(); 
    gameInProgress = false; 
    currentGameState = STATE_MENU; 

    glutTimerFunc(16, timerFunc, 0);
    glutMainLoop();
    return 0;
}