#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <string>

enum GameState
{
    STATE_MENU,
    STATE_PLAYING,
    STATE_ALMANAC
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
    NODE_FAST,
    NODE_WARP
};

enum NodeIcon
{
    ICON_NONE,
    ICON_REWARD,
    ICON_HAZARD,
    ICON_FAST,
    ICON_KEY,
    ICON_LOCK,
    ICON_WARP
};

struct Arrow
{
    ArrowDir dir;
    int result = 0;
    float displayAnim = 1.0f;
    bool isFake = false;

    // hold notes
    bool isHold = false;
    int holdDurationMs = 0;
    bool isHolding = false; 
    int holdStartTimeMs = 0;

    // chord / simultaneous press notes (like left+right)
    std::vector<ArrowDir> chordDirs; // size>1 means this is a chord
    std::vector<bool> chordPressed;  // parallel flags for chords

    Arrow(ArrowDir d = ARROW_UP, int r = 0, float anim = 1.0f, bool fake = false)
        : dir(d), result(r), displayAnim(anim), isFake(fake) {}
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
    int keyNode = -1;
    int lockNode = -1;
    std::vector<ArrowDir> password;
    bool isTargetOfKey = false; // is the node linked to a known key
    bool isWarpNode = false;
};

struct Point { float x, y; };

struct Player
{
    int currentNode = 0;
    int highlightedNeighbor = -1;
    int targetNode = -1;
    float x = 0, y = 0;
    bool moving = false;
    int moveFrom = -1, moveTo = -1;
    float moveProgress = 0.0f;
    int moveStartTimeMs = 0;
    float avatarAngle = 0.0f;

    std::vector<Point> trail;
};

struct Particle
{
    float x, y, vx, vy;
    float life, initialLife;
    float r, g, b;
};

struct Shockwave
{
    float x, y;
    float radius;
    float alpha;
};

struct Virus
{
    int currentNode = -1; // -1 means inactive/not spawned
    bool active = false;
    float moveTimer = 0.0f;
    float x = 0, y = 0; 
    
    bool isAttacking = false;
    float attackTimer = 0.0f;
};

struct FloatingText
{
    float x, y;
    std::string text;
    float r, g, b;
    float life; // 0.0 to 1.0
    float maxLife;
    float scale;
};

#endif // TYPES_H