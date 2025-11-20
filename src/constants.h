#ifndef CONSTANTS_H
#define CONSTANTS_H

// timing n movement
const int TIMING_PERFECT_MS = 300;
const int TIMING_GOOD_MS = 600;
const int TIMING_BAD_MS = 1000;
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

// ui/hud rendering
const float ARROW_DISPLAY_SIZE = 0.07f;
const float ARROW_UI_X_SPACING = 0.12f;
const float ARROW_UI_POS_Y = 0.55f;

const float FONT_SCALE_MEDIUM = 0.00016f;
const float FONT_SCALE_LARGE = 0.00022f;
const float FONT_SCALE_XLARGE = 0.0003f;

// particle system
const float PARTICLE_DRAG = 0.98f;
const float PARTICLE_GRAVITY = -0.1f;

// map n cam
const float MAP_WIDTH = 3.5f;  // half width (total width is 7.0)
const float MAP_HEIGHT = 3.5f; // half height
const float CAMERA_LERP_SPEED = 0.08f; // how fast camera follows player

const float MINIMAP_SIZE = 0.55f; // 
const float MINIMAP_OFFSET_X = 0.05f; // right padding
const float MINIMAP_OFFSET_Y = 0.05f; // bottom padding
const float MINIMAP_OPACITY = 0.8f;   // dark bg

const float WARP_DURATION = 2.0f; // secs

#endif // CONSTANTS_H