#pragma once

#include "schnitzel_lib.h"

// #############################################################################
//                           Assets Constants
// #############################################################################

// #############################################################################
//                           Assets Structs
// #############################################################################
enum SpriteID
{
  SPRITE_DICE,

  SPRITE_COUNT
};

struct Sprite
{
  IVec2 atlasOffset;
  IVec2 spriteSize;
};

// #############################################################################
//                           Assets Functions
// #############################################################################
Sprite get_sprite(SpriteID spriteID)
{
  Sprite sprite = {};

  switch(spriteID)
  {
    case SPRITE_DICE:
    {
      sprite.atlasOffset = {0, 0};
      sprite.spriteSize = {16, 16};
    }
  }

  return sprite;
}
