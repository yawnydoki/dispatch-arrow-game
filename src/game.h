#ifndef GAME_H
#define GAME_H

#include <GL/freeglut.h>
#include <vector>
#include <string>
#include <random>
#include <map>
#include "types.h"
#include "constants.h"

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

// cam
extern float cameraX;
extern float cameraY;

// transitioning between levels
extern bool isLevelTransitioning;
extern float levelTransitionTimer;

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
void makeMap(int numNodes, int minLinks, int maxLinks, int numLocks);
void initLore();
void updateShockwaves(float dt);

// for drawing
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
void drawShockwaves();
void drawPlayerTrail();
void drawWarpEffect(); 

// minimap
void drawMiniMap();

// particle funcs
void spawnParticles(int count, float x, float y, float initialSpeed, float r, float g, float b);
void updateParticles(float dt);
void drawParticles();

//sound func
void updateMusicVolume();
void playBackgroundMusic();
void playSFX(const char* file);

void display();
void reshape(int w, int h);
void keyboard(unsigned char k, int x, int y);
void specialKeys(int key, int x, int y);
void timerFunc(int);

#endif // GAME_H