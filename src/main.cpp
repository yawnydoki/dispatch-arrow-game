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

#pragma comment(lib, "winmm.lib")

// ---------- Constants ----------
const int TIMING_PERFECT_MS = 300;
const int TIMING_GOOD_MS = 600;
const int TIMING_BAD_MS = 1000;
const int MOVE_DURATION_MS = 600;

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
const float ARROW_UI_START_X = -0.15f;
const float ARROW_UI_POS_Y = 0.55f;

const float HUD_POS_X = -0.95f;
const float HUD_POS_Y = 0.95f;
const float HUD_LINE_SPACING = 0.03f;

// ---------- Enums & Structs ----------

// NEW: Game State
enum GameState
{
    STATE_MENU,
    STATE_PLAYING
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
    NODE_REWARD,
    NODE_HAZARD,
    NODE_FAST
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

bool finalNodeSpawned = false;
bool gameWon = false;
int finalNodeIndex = -1;

std::string gameMessage = "";
float gameMessageTimer = 0.0f;

std::mt19937 rng(static_cast<unsigned int>(std::time(0)));

bool currentSequenceIsHazard = false;
bool currentSequenceIsFast = false;

// NEW: State and Menu Globals
GameState currentGameState = STATE_MENU;
int musicVolume = 70; // 0-100
bool sfxEnabled = true;
int menuSelectedItem = 0;
bool gameInProgress = false; // Is a game active (not on first menu)


// ---------- Utility ----------
const char* dirName(ArrowDir d)
{
    switch (d)
    {
    case ARROW_UP: return "UP";
    case ARROW_DOWN: return "DOWN";
    case ARROW_LEFT: return "LEFT";
    default: return "RIGHT";
    }
}

void drawString(const char* s, float x, float y)
{
    glRasterPos2f(x, y);
    for (const char* p = s; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p); // Larger font
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

// ---------- Sound (MODIFIED) ----------

// NEW: Function to update music volume
void updateMusicVolume()
{
    // MCI volume is 0-1000, our slider is 0-100
    int mciVol = musicVolume * 10;
    std::string cmd = "setaudio music volume to " + std::to_string(mciVol);
    // MODIFIED: Removed TEXT() macro from around the variable
    mciSendString(cmd.c_str(), NULL, 0, NULL);
}

void playBackgroundMusic()
{
    mciSendString(TEXT("open \"Dispatch - Gristle.wav\" type mpegvideo alias music"), NULL, 0, NULL);
    mciSendString(TEXT("play music repeat"), NULL, 0, NULL);
    // Set initial volume
    updateMusicVolume();
}

void playSFX(const char* file)
{
    // MODIFIED: SFX can be disabled
    if (!sfxEnabled) return;
    // MODIFIED: Removed TEXT() macro from around the variable
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

    finalNodeSpawned = false;
    gameWon = false;
    finalNodeIndex = -1;
    gameMessage = "";
    gameMessageTimer = 0.0f;

    for (auto& n : nodes) n.visited = false;
    player.currentNode = nodeDist(rng);
    player.x = nodes[player.currentNode].x;
    player.y = nodes[player.currentNode].y;
    player.highlightedNeighbor = 0;
    player.targetNode = -1;
    player.moving = false;
}

// NEW: Game Reset Function
void resetGame()
{
    score = combo = maxCombo = 0;
    makeDemoMap();
    inSequence = false;
    sequence.clear();
    player.targetNode = -1;
    finalNodeSpawned = false;
    gameWon = false;
    finalNodeIndex = -1;
    gameInProgress = true;
    setGameMessage("New Game Started!", 1.5f);

    // Stop and restart music
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
    nodes[idx].links.push_back(player.currentNode);
    nodes[player.currentNode].links.push_back(idx);
    finalNodeSpawned = true;
    finalNodeIndex = idx;
    playSFX("Engage.wav");
    setGameMessage("The Final Node has appeared!");
}

void clampHighlightedNeighbor()
{
    if (nodes.empty() || player.currentNode < 0 || player.currentNode >= (int)nodes.size())
    {
        player.highlightedNeighbor = 0;
        return;
    }
    int n = (int)nodes[player.currentNode].links.size();
    if (n <= 0)
    {
        player.highlightedNeighbor = 0;
        return;
    }
    while (player.highlightedNeighbor < 0) player.highlightedNeighbor += n;
    if (player.highlightedNeighbor >= n) player.highlightedNeighbor %= n;
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
    if (player.currentNode < 0 || player.currentNode >= (int)nodes.size()) return;

    int linkCount = (int)nodes[player.currentNode].links.size();
    if (linkCount == 0)
    {
        playSFX("Mistake.wav");
        return;
    }

    clampHighlightedNeighbor();
    int neighbor = nodes[player.currentNode].links[player.highlightedNeighbor];
    player.targetNode = neighbor;

    currentSequenceIsHazard = false;
    currentSequenceIsFast = false;
    NodeType targetType = nodes[neighbor].type;

    if (nodes[neighbor].isFinal)
    {
        setGameMessage("Final Sequence!");
    }
    else if (targetType == NODE_HAZARD)
    {
        currentSequenceIsHazard = true;
        setGameMessage("HAZARD! Perfect/Good only!");
    }
    else if (targetType == NODE_FAST)
    {
        currentSequenceIsFast = true;
        setGameMessage("FAST! Timing is halved!");
    }
    else if (targetType == NODE_REWARD)
    {
        setGameMessage("Reward Node!");
    }

    std::uniform_int_distribution<int> arrowDist(0, 3);
    std::uniform_int_distribution<int> lenDist(0, 2);
    int baseLen = 3 + (score / 20);
    int len = nodes[neighbor].isFinal ? (baseLen + 2) : (baseLen + lenDist(rng));

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
        if (gameMessageTimer <= 0.0f)
        {
            gameMessage = "";
        }
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

    if (inSequence && stepWaitingForInput)
    {
        int dt = now - stepStartTimeMs;
        if (dt > currentBadMs + 300)
        {
            sequence[seqIdx].result = -1;
            sequence[seqIdx].displayAnim = 1.2f;
            combo = 0;
            seqIdx++;
            stepWaitingForInput = false;
            stepStartTimeMs = now + 120;
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
                setGameMessage("VICTORY! Press R to restart.", 1000.0f);
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

    // No glutPostRedisplay() here, moved to timerFunc
}

// ---------- Input ----------
void specialKeys(int key, int x, int y)
{
    // MODIFIED: Only handle input if in playing state
    if (currentGameState != STATE_PLAYING) return;
    if (gameWon) return;

    if (!inSequence || !stepWaitingForInput)
    {
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
        return;
    }

    ArrowDir pressed;
    if (key == GLUT_KEY_UP) pressed = ARROW_UP;
    else if (key == GLUT_KEY_DOWN) pressed = ARROW_DOWN;
    else if (key == GLUT_KEY_LEFT) pressed = ARROW_LEFT;
    else pressed = ARROW_RIGHT;

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

        if (tier == 1)
        {
            score += 3; combo++;
            sequence[seqIdx].result = 1;
            playSFX("Success.wav");
        }
        else if (tier == 2)
        {
            score += 2; combo++;
            sequence[seqIdx].result = 2;
            playSFX("Success.wav");
        }
        else if (tier == 3)
        {
            if (currentSequenceIsHazard)
            {
                sequenceFailed = true;
                setGameMessage("Hazard Failed!");
            }
            else
            {
                score += 1; combo++;
                sequence[seqIdx].result = 3;
                playSFX("Success.wav");
            }
        }
        else
        {
            sequenceFailed = true;
        }
    }
    else
    {
        sequenceFailed = true;
    }

    if (sequenceFailed)
    {
        sequence[seqIdx].result = -1;
        combo = 0;
        score = std::max(0, score - 1);
        playSFX("Mistake.wav");

        if (currentSequenceIsHazard)
        {
            inSequence = false;
            sequence.clear();
            seqIdx = 0;
            stepWaitingForInput = false;
            glutPostRedisplay();
            return;
        }
    }

    if (combo > maxCombo) maxCombo = combo;
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
        bool anyMiss = false;
        for (auto& a : sequence)
            if (a.result == -1)
                anyMiss = true;
        if (!anyMiss && player.targetNode >= 0)
        {
            nodes[player.targetNode].visited = true;
            if (nodes[player.targetNode].type == NODE_REWARD)
            {
                score += 10;
                setGameMessage("Reward collected! +10 Score", 2.0f);
                nodes[player.targetNode].type = NODE_NORMAL;
            }
            startMoveAnimation(player.currentNode, player.targetNode);
            player.currentNode = player.targetNode;
            player.targetNode = -1;
            player.highlightedNeighbor = 0;
        }
        inSequence = false;
        sequence.clear();
        seqIdx = 0;
        stepWaitingForInput = false;
    }

    glutPostRedisplay();
}

void keyboard(unsigned char k, int x, int y)
{
    // --- Global Keys ---
    if (k == 27) // ESC key
    {
        if (currentGameState == STATE_PLAYING)
        {
            currentGameState = STATE_MENU;
            setGameMessage("Paused", 1.0f);
        }
        else if (currentGameState == STATE_MENU && gameInProgress)
        {
            currentGameState = STATE_PLAYING;
            setGameMessage("", 0.0f); // Clear message
        }
        glutPostRedisplay();
        return;
    }

    // --- Menu State Keys ---
    if (currentGameState == STATE_MENU)
    {
        std::vector<std::string> menuItems;
        if (gameInProgress)
        {
            menuItems.push_back("Resume Game");
            menuItems.push_back("Music Volume");
            menuItems.push_back("SFX");
            menuItems.push_back("Restart Game");
            menuItems.push_back("Quit");
        }
        else
        {
            menuItems.push_back("New Game");
            menuItems.push_back("Music Volume");
            menuItems.push_back("SFX");
            menuItems.push_back("Quit");
        }
        int numItems = (int)menuItems.size();


        if (k == 'w' || k == 'W') // Up
        {
            menuSelectedItem = (menuSelectedItem - 1 + numItems) % numItems;
            playSFX("Select.wav");
        }
        else if (k == 's' || k == 'S') // Down
        {
            menuSelectedItem = (menuSelectedItem + 1) % numItems;
            playSFX("Select.wav");
        }
        else if (k == 'a' || k == 'A') // Left
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
        else if (k == 'd' || k == 'D') // Right
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
        else if (k == 'e' || k == 'E' || k == 13) // Select (E or Enter)
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
            else if (item == "Restart Game")
            {
                resetGame();
                currentGameState = STATE_PLAYING;
            }
            else if (item == "Quit")
            {
                exit(0);
            }
        }
        glutPostRedisplay();
        return;
    }

    // --- Playing State Keys ---
    if (currentGameState == STATE_PLAYING)
    {
        if (gameWon)
        {
            if (k == 'r' || k == 'R') resetGame();
            return;
        }

        if (k == 'r' || k == 'R')
        {
            resetGame();
            return;
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
                setGameMessage("Sequence Cancelled", 1.0f);
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
    glColor4f(0.10f, 0.12f, 0.16f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -1.0f; x <= 1.01f; x += 0.06f)
    {
        glVertex2f(x, -1.0f); glVertex2f(x, 1.0f);
    }
    for (float y = -1.0f; y <= 1.01f; y += 0.06f)
    {
        glVertex2f(-1.0f, y); glVertex2f(1.0f, y);
    }
    glEnd();
}

void drawMap()
{
    drawBackgroundGrid();

    glLineWidth(3.0f);
    for (size_t i = 0; i < nodes.size(); i++)
    {
        for (int j : nodes[i].links)
        {
            if (j < (int)i) continue;
            glColor3f(0.6f, 0.75f, 0.9f);
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
            drawString("FINISH", x - 0.035f, y - 0.005f);
            continue;
        }

        if ((int)i == player.currentNode)
            glColor3f(0.12f, 0.45f, 0.85f), drawRing(x, y, NODE_RING_CURRENT_OUT, NODE_RING_CURRENT_IN);

        if (nodes[i].visited)
        {
            if (nodes[i].type == NODE_HAZARD) glColor3f(0.6f, 0.2f, 0.2f);
            else if (nodes[i].type == NODE_FAST) glColor3f(0.6f, 0.5f, 0.1f);
            else glColor3f(0.22f, 0.45f, 0.85f);
            drawCircle(x, y, NODE_RADIUS_VISITED);
        }
        else
        {
            if (nodes[i].type == NODE_HAZARD) glColor3f(1.0f, 0.3f, 0.3f);
            else if (nodes[i].type == NODE_FAST) glColor3f(1.0f, 0.8f, 0.2f);
            else glColor3f(0.95f, 0.95f, 0.95f);
            drawCircle(x, y, NODE_RADIUS_UNVISITED);
        }

        if (nodes[i].type == NODE_REWARD)
            glColor3f(1.0f, 0.9f, 0.3f), drawCircle(x + 0.06f, y + 0.06f, NODE_RADIUS_REWARD);
    }

    if (!nodes.empty() && player.currentNode >= 0 && player.currentNode < (int)nodes.size() &&
        !nodes[player.currentNode].links.empty() && !inSequence && !player.moving)
    {
        clampHighlightedNeighbor();
        int n = nodes[player.currentNode].links[player.highlightedNeighbor];
        glColor3f(0.4f, 1.0f, 0.5f);
        drawRing(nodes[n].x, nodes[n].y, NODE_RING_HIGHLIGHT_OUT, NODE_RING_HIGHLIGHT_IN);
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

void drawSequenceUI()
{
    float cx = 0.0f, cy = ARROW_UI_POS_Y, w = 0.6f, h = 0.2f;
    drawTexturedQuad(cx, cy, w, h, 0.95f);

    glColor3f(0.15f, 0.5f, 0.9f);
    drawQuad(cx - w / 2, cy + h / 2 - 0.03f, cx + w / 2, cy + h / 2);
    glColor3f(1, 1, 1);
    drawString("BUILD PATH", cx - w / 2 + 0.02f, cy + h / 2 - 0.02f);

    float startX = cx + ARROW_UI_START_X;
    float arrowY = cy - 0.01f;
    for (size_t i = 0; i < sequence.size(); i++)
    {
        float px = startX + i * ARROW_UI_X_SPACING;
        float animScale = 1.0f + 0.12f * sequence[i].displayAnim;
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

// ---------- HUD & Menu ----------
void displayHUD()
{
    glColor3f(1, 1, 1);
    float y = HUD_POS_Y;
    drawString("Controls: A/D or Left/Right | E: move | R: restart | ESC: menu", HUD_POS_X, y);
    y -= HUD_LINE_SPACING;

    std::string scoreText = "Score: " + std::to_string(score) +
        " | Combo: " + std::to_string(combo) +
        " | Max Combo: " + std::to_string(maxCombo);
    drawString(scoreText.c_str(), HUD_POS_X, y);
    y -= HUD_LINE_SPACING;

    if (inSequence)
        drawString("Sequence active: press arrows!", HUD_POS_X, y);
    if (player.moving)
        drawString("Moving...", 0.75f, 0.9f);

    if (gameMessageTimer > 0.0f)
    {
        glColor3f(1.0f, 0.9f, 0.2f);
        drawString(gameMessage.c_str(), -0.2f, 0.0f);
    }

    if (gameWon)
    {
        glColor3f(0.2f, 1.0f, 0.3f);
        drawString("VICTORY! Press R to restart", -0.12f, 0.1f);
    }

    // --- Legend ---
    float legendX = -0.95f;
    float legendY = -0.65f;
    float legendSpacing = 0.05f;
    float legendIconSize = 0.02f;
    float legendTextX = legendX + 0.04f;

    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Legend:", legendX, legendY);
    legendY -= legendSpacing;

    drawAvatarShape(legendX + legendIconSize * 0.5f, legendY, legendIconSize * 0.7f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("You", legendTextX, legendY - 0.005f);
    legendY -= legendSpacing;

    glColor3f(0.95f, 0.95f, 0.95f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.5f);
    drawString("Unvisited Node", legendTextX, legendY - 0.005f);
    legendY -= legendSpacing;

    glColor3f(0.22f, 0.45f, 0.85f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_VISITED * 0.5f);
    drawString("Visited Node", legendTextX, legendY - 0.005f);
    legendY -= legendSpacing;

    glColor3f(0.4f, 1.0f, 0.5f);
    drawRing(legendX + legendIconSize * 0.5f, legendY, NODE_RING_HIGHLIGHT_OUT * 0.4f, NODE_RING_HIGHLIGHT_IN * 0.4f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Highlighted Target", legendTextX, legendY - 0.005f);
    legendY -= legendSpacing;

    glColor3f(1.0f, 0.9f, 0.3f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_REWARD * 1.5f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Reward Node", legendTextX, legendY - 0.005f);
    legendY -= legendSpacing;

    glColor3f(1.0f, 0.3f, 0.3f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.5f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Hazard Node (Good/Perfect only!)", legendTextX, legendY - 0.005f);
    legendY -= legendSpacing;

    glColor3f(1.0f, 0.8f, 0.2f);
    drawCircle(legendX + legendIconSize * 0.5f, legendY, NODE_RADIUS_UNVISITED * 0.5f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("Fast Node (Timing halved!)", legendTextX, legendY - 0.005f);
    legendY -= legendSpacing;
}

// NEW: Draw Menu
void drawMenu()
{
    // Draw a semi-transparent overlay
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    drawQuad(-2.0f, -2.0f, 2.0f, 2.0f); // Cover whole screen

    // --- Draw Menu Items ---
    float cx = 0.0f;
    float cy = 0.3f;
    float menuSpacing = 0.12f;

    glColor3f(1.0f, 1.0f, 1.0f);
    drawString("THIS PATCHED", cx - 0.18f, cy + 0.15f); // Title
    
    std::vector<std::string> menuItems;
    if (gameInProgress)
    {
        menuItems.push_back("Resume Game");
        menuItems.push_back("Music Volume");
        menuItems.push_back("SFX");
        menuItems.push_back("Restart Game");
        menuItems.push_back("Quit");
    }
    else
    {
        menuItems.push_back("New Game");
        menuItems.push_back("Music Volume");
        menuItems.push_back("SFX");
        menuItems.push_back("Quit");
    }

    for (size_t i = 0; i < menuItems.size(); ++i)
    {
        float y = cy - i * menuSpacing;
        
        // Draw selector
        if (i == menuSelectedItem)
        {
            glColor3f(0.1f, 1.0f, 0.2f);
            drawArrowShape(cx - 0.3f, y + 0.01f, ARROW_RIGHT, 0, 0.05f);
        }
        else
        {
            glColor3f(1.0f, 1.0f, 1.0f);
        }

        // Draw item text
        std::string itemText = menuItems[i];
        if (itemText == "Music Volume")
        {
            itemText = std::string("Music Volume: < ") + std::to_string(musicVolume) + " >";
        }
        else if (itemText == "SFX")
        {
            itemText = std::string("SFX: < ") + (sfxEnabled ? "ON" : "OFF") + " >";
        }

        drawString(itemText.c_str(), cx - 0.25f, y);
    }

    glColor3f(0.8f, 0.8f, 0.8f);
    drawString("Controls: W/S to navigate, A/D to change, E to select", cx - 0.4f, cy - (menuItems.size() + 1) * menuSpacing);
}

// ---------- Draw Game (NEW) ----------
// Encapsulates all game-world drawing
void drawGame()
{
    drawMap();
    drawAvatar(player.x, player.y);
    displayHUD();
    if (inSequence)
        drawSequenceUI();
}

// ---------- Display & Reshape (MODIFIED) ----------
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Draw the game world always
    drawGame();

    // Draw the menu on top if we are in the menu state
    if (currentGameState == STATE_MENU)
    {
        drawMenu();
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
    if (asp >= 1.0f)
        glOrtho(-asp, asp, -1, 1, -1, 1);
    else
        glOrtho(-1, 1, -1.0f / asp, 1.0f / asp, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// ---------- Timer (MODIFIED) ----------
void timerFunc(int)
{
    // Only update game logic if not paused
    if (currentGameState == STATE_PLAYING)
    {
        updateGame();
    }
    
    // Always redisplay to keep menu responsive
    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0);
}

// ---------- Main ----------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(windowW, windowH);
    glutCreateWindow("This Patched");

    playBackgroundMusic(); // Play music (will play on menu)
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutReshapeFunc(reshape);

    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    uiTex = makeProceduralUITexture(256, 64);
    
    // Don't call makeDemoMap here, player will start it from the menu
    // We can call it to populate the background, but set gameInProgress = false
    makeDemoMap(); 
    gameInProgress = false; 
    currentGameState = STATE_MENU; // Start on the menu

    glutTimerFunc(16, timerFunc, 0);
    glutMainLoop();
    return 0;
}