#pragma once

#include "assets.h"
#include "schnitzel_lib.h"


// #############################################################################
//                           Renderer Constants
// #############################################################################
constexpr int MAX_TRANSFORMS = 1000;

// #############################################################################
//                           Renderer Structs
// #############################################################################
struct Transform 
{
  Vec2 pos;
  Vec2 size;
  IVec2 atlasOffset;
  IVec2 spriteSize;
};

struct RenderData
{
  int transformCount;
  Transform transforms[MAX_TRANSFORMS];
};

// #############################################################################
//                           Renderer Globals
// #############################################################################
static RenderData* renderData;

// #############################################################################
//                           Renderer Functions
// #############################################################################
void draw_sprite(SpriteID spriteID, Vec2 pos, Vec2 size)
{
  Sprite sprite = get_sprite(spriteID);

  Transform transform = {};
  transform.pos = pos;
  transform.size = size;
  transform.atlasOffset = sprite.atlasOffset;
  transform.spriteSize = sprite.spriteSize;

  renderData->transforms[renderData->transformCount++] = transform;
}