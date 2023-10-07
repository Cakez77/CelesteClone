#pragma once

#include "assets.h"
#include "shader_header.h"
#include "schnitzel_lib.h"


// #############################################################################
//                           Renderer Constants
// #############################################################################
int RENDER_OPTION_FLIP_X = BIT(0);
int RENDER_OPTION_FLIP_Y = BIT(1);

// #############################################################################
//                           Renderer Structs
// #############################################################################
struct OrthographicCamera2D
{
  float zoom = 1.0f;
  Vec2 dimensions;
  Vec2 position;
};

struct DrawData
{
  int animationIdx;
  int renderOptions;
};

struct RenderData
{
  OrthographicCamera2D gameCamera;
  OrthographicCamera2D uiCamera;

  Array<Transform, 1000> transforms;
};

// #############################################################################
//                           Renderer Globals
// #############################################################################
static RenderData* renderData;

// #############################################################################
//                           Renderer Untility
// #############################################################################
IVec2 screen_to_world(IVec2 screenPos)
{
  OrthographicCamera2D camera = renderData->gameCamera;

  int xPos = (float)screenPos.x / 
             (float)input->screenSize.x * 
             camera.dimensions.x; // [0; dimensions.x]

  // Offset using dimensions and position
  xPos += -camera.dimensions.x / 2.0f + camera.position.x;

  int yPos = (float)screenPos.y / 
             (float)input->screenSize.y * 
             camera.dimensions.y; // [0; dimensions.y]

  // Offset using dimensions and position
  yPos += camera.dimensions.y / 2.0f + camera.position.y;

  return {xPos, yPos};
}

int animate(float* time, int frameCount, float duration = 1.0f)
{
  while(*time > duration)
  {
    *time -= duration;
  }
  
  int animationIdx = (int)((*time / duration) * frameCount);
  
  // Clamp
  if (animationIdx >= frameCount)
  {
    animationIdx = frameCount - 1;
  }
  
  return animationIdx;
}

// #############################################################################
//                           Renderer Functions
// #############################################################################
void draw_quad(Transform transform)
{
  renderData->transforms.add(transform);
}

void draw_quad(Vec2 pos, Vec2 size)
{
  Transform transform = {};
  transform.pos = pos - size / 2.0f;
  transform.size = size;
  transform.atlasOffset = {0, 0};
  transform.spriteSize = {1, 1}; // Indexing into white

  renderData->transforms.add(transform);
}

void draw_sprite(SpriteID spriteID, Vec2 pos, DrawData drawData = {})
{
  Sprite sprite = get_sprite(spriteID);
  // For Anmations, this is a multiple of the sprites size,
  // based on the animationIdx
  sprite.atlasOffset.x += drawData.animationIdx * sprite.size.x;

  Transform transform = {};
  transform.pos = pos - vec_2(sprite.size) / 2.0f;
  transform.size = vec_2(sprite.size);
  transform.atlasOffset = sprite.atlasOffset;
  transform.spriteSize = sprite.size;
  transform.renderOptions = drawData.renderOptions;

  renderData->transforms.add(transform);
}

void draw_sprite(SpriteID spriteID, IVec2 pos, DrawData drawData = {})
{
  draw_sprite(spriteID, vec_2(pos), drawData);
}