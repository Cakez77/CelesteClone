#pragma once

#include "input.h"
#include "schnitzel_lib.h"
#include "render_interface.h"

// #############################################################################
//                           Game Globals
// #############################################################################
constexpr int tset = 5;

// #############################################################################
//                           Game Structs
// #############################################################################
enum GameInputType
{
  MOVE_LEFT,
  MOVE_RIGHT,
  MOVE_UP,
  MOVE_DOWN,
  JUMP,

  MOUSE_LEFT,
  MOUSE_RIGHT,

  GAME_INPUT_COUNT
};

struct KeyMapping
{
  Array<KeyCodeID, 3> keys;
};

struct GameState
{
  bool initialized = false;
  IVec2 playerPos;
  
  KeyMapping keyMappings[GAME_INPUT_COUNT];
};

// #############################################################################
//                           Game Globals
// #############################################################################
static GameState* gameState;

// #############################################################################
//                           Game Functions (Exposed)
// #############################################################################
extern "C"
{
  EXPORT_FN void update_game(GameState* gameStateIn, RenderData* renderDataIn, Input* inputIn);
}