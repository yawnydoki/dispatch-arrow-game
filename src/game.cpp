#include "game.h"
#include <ctime>

//  tracks pressed keys for hold detection
std::map<unsigned char, bool> keysState;
std::map<int, bool> specialKeysState;

std::vector<Node> nodes;
Player player;
std::vector<Particle> particles;
std::vector<Shockwave> shockwaves;

std::vector<FloatingText> floatingTexts;
float inputCooldown = 0.0f;

int g_mouseX = 0;
int g_mouseY = 0;

void passiveMouse(int x, int y)
{
    g_mouseX = x;
    g_mouseY = y;
}


std::vector<Arrow> sequence;
int seqIdx = 0;
bool inSequence = false;
bool stepWaitingForInput = false;
int stepStartTimeMs = 0;

int playerLives = STARTING_LIVES;
int currentLevel = 1;

int hazardSequencesRemaining = 0;
int totalHazardSequences = 0;

int windowW = 1920, windowH = 1080;
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

int tutorialPage = 0;
float uiScale = 1.0f;

// resolution / fullscreen settings
bool isFullscreen = true;
std::vector<std::pair<int,int>> supportedResolutions = { {1920,1080}, {1600,900}, {1366,768}, {1280,720} };
int currentResolutionIndex = 0;

void applyResolutionSetting()
{
    if (currentResolutionIndex < 0 || currentResolutionIndex >= (int)supportedResolutions.size())
        currentResolutionIndex = 0;

    int w = supportedResolutions[currentResolutionIndex].first;
    int h = supportedResolutions[currentResolutionIndex].second;
    windowW = w;
    windowH = h;

    if (isFullscreen)
    {
        glutFullScreen();
    }
    else
    {
        glutReshapeWindow(windowW, windowH);
        glutPositionWindow(100, 100);
    }
}

bool displayingPassword = false;
bool isPasswordInput = false;
bool justRevealedPassword = false;

std::map<int, std::vector<ArrowDir>> revealedPasswords;

float screenShakeTimer = 0.0f; 
std::vector<std::string> loreSnippets; 
float gameMessageMaxTimer = 0.0f;

float cameraX = 0.0f;
float cameraY = 0.0f;

bool isLevelTransitioning = false;
float levelTransitionTimer = 0.0f;

Virus virus;
float virusSpawnTimer = 0.0f;

const char* BGM_FILE = "assets/audio/Dispatch - Gristle.wav"; //
const char* SFX_MENU_SELECT = "assets/audio/Menu_Select.wav"; //
const char* SFX_MENU_ENGAGE = "assets/audio/Menu_Engage.wav"; //
const char* SFX_MOVE_HIGHLIGHT = "assets/audio/Highlight.wav"; // ?
const char* SFX_MOVE_SUCCESS = "assets/audio/Move_Success.wav"; //
const char* SFX_TIMING_PERFECT = "assets/audio/Timing_Perfect.wav";
const char* SFX_TIMING_GOOD = "assets/audio/Timing_Good.wav";
const char* SFX_TIMING_FAIL = "assets/audio/Timing_Fail.wav";  //
const char* SFX_UNLOCK = "assets/audio/Unlock.wav";  //
const char* SFX_VIRUS_WARNING = "assets/audio/Warning.wav";
const char* SFX_FINAL_NODE_SPAWN = "assets/audio/Spawn.wav"; 


void setGameMessage(const std::string& msg, float duration)
{
    gameMessage = msg;
    gameMessageTimer = duration;
    gameMessageMaxTimer = duration; 
}

std::vector<std::string> getMenuItems()
{
    if (isGameOver)
        return { "New Game", "How to Play", "Codex", "Quit" };
    else if (gameInProgress)
        return { "Resume Game", "Restart Game", "Codex", "Resolution", "Music Volume", "SFX", "Quit" };
    else
        return { "New Game", "Tutorial", "Codex", "Resolution", "Music Volume", "SFX", "Quit" };
}

void spawnFloatingText(const std::string& text, float x, float y, float r, float g, float b, float scale)
{
    FloatingText ft;
    ft.text = text;
    ft.x = x;
    ft.y = y;
    ft.r = r; ft.g = g; ft.b = b;
    ft.life = 1.0f;
    ft.maxLife = 1.0f;
    ft.scale = scale;
    floatingTexts.push_back(ft);
}

void updateFloatingTexts(float dt)
{
    for (size_t i = 0; i < floatingTexts.size(); )
    {
        floatingTexts[i].life -= dt * 1.5f; // fade speed
        floatingTexts[i].y += dt * 0.2f;    // float up
        
        if (floatingTexts[i].life <= 0.0f) {
            floatingTexts[i] = floatingTexts.back();
            floatingTexts.pop_back();
        } else {
            i++;
        }
    }
}