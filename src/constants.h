#ifndef CONSTANTS_H
#define CONSTANTS_H

// timing n movement
const int TIMING_PERFECT_MS = 450; 
const int TIMING_GOOD_MS = 900;   
const int TIMING_BAD_MS = 1500;    
const int MOVE_DURATION_MS = 600;

// game setting
const int STARTING_LIVES = 3;

// node rendering
const float NODE_RADIUS_VISITED = 0.06f;
const float NODE_RADIUS_UNVISITED = 0.05f;
const float NODE_RADIUS_REWARD = 0.02f;
const float NODE_RING_CURRENT_OUT = 0.12f;
const float NODE_RING_CURRENT_IN = 0.08f;
const float NODE_RING_HIGHLIGHT_OUT = 0.10f;
const float NODE_RING_HIGHLIGHT_IN = 0.065f;
const float NODE_RING_FINAL_OUT = 0.14f;
const float NODE_RING_FINAL_IN = 0.09f;
const float NODE_RING_KEY_PULSE_OUT = 0.11f;
const float NODE_RING_KEY_PULSE_IN = 0.10f;

// player rendering
const float PLAYER_AVATAR_RADIUS = 0.05f;
const int PLAYER_AVATAR_SIDES = 6;
const float PLAYER_AVATAR_SPIN_SPEED = 180.0f; // deg/sec
const float PLAYER_AVATAR_TURN_SPEED = 0.2f; // 20% lerp per frame

// rendering ui hud
const float ARROW_DISPLAY_SIZE = 0.07f;
const float ARROW_UI_X_SPACING = 0.12f;
const float ARROW_UI_POS_Y = 0.55f;

const float FONT_SCALE_MEDIUM = 0.00016f;
const float FONT_SCALE_LARGE = 0.00022f;
const float FONT_SCALE_XLARGE = 0.0003f;

// sfx
extern const char* BGM_FILE;
extern const char* SFX_MENU_SELECT;        // highlight/menu movement
extern const char* SFX_MENU_ENGAGE;        // Menu selection / Start sequence
extern const char* SFX_MOVE_HIGHLIGHT;       // Left/Right selection on map
extern const char* SFX_MOVE_SUCCESS;      // after sequence or simple move
extern const char* SFX_TIMING_PERFECT;  // Perfect hit
extern const char* SFX_TIMING_GOOD;        // Good/Okay hit
extern const char* SFX_TIMING_FAIL;        // Failed sequence / Damage taken / Error
extern const char* SFX_UNLOCK;                  // Firewall unlocked
extern const char* SFX_VIRUS_WARNING;
extern const char* SFX_FINAL_NODE_SPAWN;         // finish node

// particle system
const float PARTICLE_DRAG = 0.98f;
const float PARTICLE_GRAVITY = -0.1f;

// map n cam
const float MAP_WIDTH = 3.5f;  // half width (total width is 7.0)
const float MAP_HEIGHT = 3.5f; // half height
const float CAMERA_LERP_SPEED = 0.08f; // how fast camera follows player

// minimap
const float MINIMAP_SIZE = 0.55f; 
const float MINIMAP_OFFSET_X = 0.05f; 
const float MINIMAP_OFFSET_Y = 0.05f; 
const float MINIMAP_OPACITY = 0.8f;   

// transition / tp to next level
const float WARP_DURATION = 2.0f; 

// virus
const float VIRUS_SPAWN_DELAY = 30.0f; 
const float VIRUS_MOVE_INTERVAL = 3.0f; // seconds per step
const float VIRUS_ATTACK_DELAY = 1.5f;  // time required to stay on player to damage

#endif // CONSTANTS_H