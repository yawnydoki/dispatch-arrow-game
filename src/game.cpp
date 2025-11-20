#include "game.h"
#include <ctime>

std::vector<Node> nodes;
Player player;
std::vector<Particle> particles;
std::vector<Shockwave> shockwaves;

std::vector<Arrow> sequence;
int seqIdx = 0;
bool inSequence = false;
bool stepWaitingForInput = false;
int stepStartTimeMs = 0;

int playerLives = STARTING_LIVES;
int currentLevel = 1;

// multi-sequence hazard nodes
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

void setGameMessage(const std::string& msg, float duration)
{
    gameMessage = msg;
    gameMessageTimer = duration;
    gameMessageMaxTimer = duration; 
}

std::vector<std::string> getMenuItems()
{
    if (isGameOver)
        return { "New Game", "Codex", "Quit" };
    else if (gameInProgress)
        return { "Resume Game", "Restart Game", "Codex", "Music Volume", "SFX", "Quit" };
    else
        return { "New Game", "Codex", "Music Volume", "SFX", "Quit" };
}