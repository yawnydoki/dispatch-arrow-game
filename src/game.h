#ifndef GAME_H
#define GAME_H

#include <GL/freeglut.h>
#include <vector>
#include <utility>
#include <string>
#include <random>
#include <map>
#include "types.h"
#include "constants.h"

// defined in game.cpp
extern std::vector<Node> nodes;
extern Player player;
extern std::vector<Particle> particles;
extern std::vector<Arrow> sequence;
extern int seqIdx;
extern bool inSequence;
extern bool stepWaitingForInput;
extern int stepStartTimeMs;
extern int playerLives;
extern int currentLevel;
extern int hazardSequencesRemaining;
extern int totalHazardSequences;
extern int windowW, windowH;
extern GLuint uiTex;
extern bool finalNodeSpawned;
extern bool gameWon;
extern bool isGameOver;
extern int finalNodeIndex;
extern std::string gameMessage;
extern float gameMessageTimer;
extern std::mt19937 rng;
extern bool currentSequenceIsHazard;
extern bool currentSequenceIsFast;
extern GameState currentGameState;
extern int musicVolume;
extern bool sfxEnabled;
extern int menuSelectedItem;
extern bool gameInProgress;
extern float screenAspect;
extern bool displayingPassword;
extern bool isPasswordInput;
extern bool justRevealedPassword;
extern std::map<int, std::vector<ArrowDir>> revealedPasswords;
extern float screenShakeTimer;
extern std::vector<std::string> loreSnippets;
extern float gameMessageMaxTimer;
extern std::vector<Shockwave> shockwaves;

// Camera
extern float cameraX;
extern float cameraY;

// Transition
extern bool isLevelTransitioning;
extern float levelTransitionTimer;

// virus
extern Virus virus;
extern float virusSpawnTimer;

extern std::vector<FloatingText> floatingTexts;
extern float inputCooldown;

extern int g_mouseX, g_mouseY;

extern std::map<unsigned char, bool> keysState;
extern std::map<int, bool> specialKeysState;
extern int tutorialPage;
extern float uiScale;
extern bool isFullscreen;
extern std::vector<std::pair<int,int>> supportedResolutions;
extern int currentResolutionIndex;

void applyResolutionSetting();

// core funcs
void passiveMouse(int x, int y);
void setGameMessage(const std::string& msg, float duration = 2.0f);
std::vector<std::string> getMenuItems();
void resetGame();
void updateGame();
void spawnFinalNode();
void clampHighlightedNeighbor();
void startMoveAnimation(int from, int to);
void attemptMoveToHighlightedNeighbor();
void handleArrowPress(ArrowDir pressed);
void handlePasswordInput(ArrowDir pressed);
void generateNextHazardSequence();
void handleSequenceFailure(bool passwordFail);
void finishSequenceSuccess();
void makeMap(int numNodes, int minLinks, int maxLinks, int numLocks);
void initLore();
void updateShockwaves(float dt);

void spawnFloatingText(const std::string& text, float x, float y, float r, float g, float b, float scale = 1.0f);
void updateFloatingTexts(float dt);

// Drawing
void drawString(const char* s, float x, float y, float scale);
void drawCircle(float cx, float cy, float r, int seg = 32);
void drawRing(float cx, float cy, float ro, float ri, int seg = 32);
void drawQuad(float x1, float y1, float x2, float y2);
void drawEnergyIcon(float cx, float cy, float radius, float pulseAlpha);
void drawArrowShape(float x, float y, ArrowDir dir, int result, float size = ARROW_DISPLAY_SIZE, float animScale = 1.0f, bool isFake = false);
void drawAvatarShape(float x, float y, float radius, float angle = -1.0f);
void drawNodeIcon(NodeIcon icon, float x, float y);
GLuint makeProceduralUITexture(int w = 256, int h = 64);
void drawGame();
void drawMenu();
void drawAlmanac();
void drawTutorial();
void drawShockwaves();
void drawPlayerTrail();
void drawWarpEffect(); 
void drawVirus(float x, float y);
void drawMiniMap();
void drawFloatingTexts();

// particle funcs
void spawnParticles(int count, float x, float y, float initialSpeed, float r, float g, float b);
void updateParticles(float dt);
void drawParticles();

// sound func
void updateMusicVolume();
void playBackgroundMusic();
void playSFX(const char* file);

// main loop handlers
void display();
void reshape(int w, int h);
void keyboard(unsigned char k, int x, int y);
void keyboardUp(unsigned char k, int x, int y);
void specialKeys(int key, int x, int y);
void specialKeysUp(int key, int x, int y);
void timerFunc(int);

#endif // GAME_H