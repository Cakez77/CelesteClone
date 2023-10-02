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
  SPRITE_WHITE,
  SPRITE_DICE,
  SPRITE_CELESTE,
  SPRITE_SOLID_01,
  SPRITE_SOLID_02,

  SPRITE_COUNT
};

struct Sprite
{
  IVec2 atlasOffset;
  IVec2 size;
};

// #############################################################################
//                           Assets Functions
// #############################################################################
Sprite get_sprite(SpriteID spriteID)
{
  Sprite sprite = {};

  switch(spriteID)
  {
    case SPRITE_WHITE:
    {
      sprite.atlasOffset = {0, 0};
      sprite.size = {1, 1};
      break;
    }

    case SPRITE_DICE:
    {
      sprite.atlasOffset = {16, 0};
      sprite.size = {16, 16};
      break;
    }

    case SPRITE_CELESTE:
    {
      sprite.atlasOffset = {112, 0};
      sprite.size = {17, 20};
      break;
    }

    case SPRITE_SOLID_01:
    {
      sprite.atlasOffset = {0, 16};
      sprite.size = {28, 18};
      break;
    }

    case SPRITE_SOLID_02:
    {
      sprite.atlasOffset = {32, 16};
      sprite.size = {16, 13};
      break;
    }
  }

  return sprite;
}
