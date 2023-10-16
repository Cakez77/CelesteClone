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
enum Layer
{
  LAYER_GAME,
  LAYER_UI,
  LAYER_COUNT
};

struct OrthographicCamera2D
{
  float zoom = 1.0f;
  Vec2 dimensions;
  Vec2 position;
};

struct DrawData
{
  Material material = {};
  int animationIdx;
  int renderOptions;
  float layer = 0.0f;
};

struct TextData
{
  Material material = {};
  float fontSize = 1.0f;
  int renderOptions;
  float layer = 0.0f;
};

struct Glyph
{
  Vec2 offset;
  Vec2 advance;
  IVec2 textureCoords;
  IVec2 size;
};

struct RenderData
{
  OrthographicCamera2D gameCamera;
  OrthographicCamera2D uiCamera;

  int fontHeight;
  Glyph glyphs[127];

  Array<Material, 1000> materials;
  Array<Transform, 1000> transforms;
  Array<Transform, 1000> uiTransforms;
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

int get_material_idx(Material material = {})
{
  // Convert from SRGB to linear color space, to be used in the shader, poggies
  material.color.r = powf(material.color.r, 2.2f);
  material.color.g = powf(material.color.g, 2.2f);
  material.color.b = powf(material.color.b, 2.2f);
  material.color.a = powf(material.color.a, 2.2f);

  for(int materialIdx = 0; materialIdx < renderData->materials.count; materialIdx++)
  {
    if(renderData->materials[materialIdx] == material)
    {
      return materialIdx;
    }
  }

  return renderData->materials.add(material);
}

float get_layer(Layer layer, float subLayer = 0.0f)
{
  float floatLayer = (float)layer;
  float layerStep = 1.0f / (float)LAYER_COUNT;
  float result = layerStep * floatLayer + subLayer / 1000.0f;
  return result;
}

Transform get_transform(SpriteID spriteID, Vec2 pos, Vec2 size = {}, DrawData drawData = {})
{
  Sprite sprite = get_sprite(spriteID);
  size = size? size: vec_2(sprite.size);

  Transform transform = {};
  transform.materialIdx = get_material_idx(drawData.material);
  transform.pos = pos - size / 2.0f;
  transform.size = size;
  transform.atlasOffset = sprite.atlasOffset;
  // For Anmations, this is a multiple of the sprites size,
  // based on the animationIdx
  transform.atlasOffset.x += drawData.animationIdx * sprite.size.x;
  transform.spriteSize = sprite.size;
  transform.renderOptions = drawData.renderOptions;
  transform.layer = drawData.layer;

  return transform;
}

// #############################################################################
//                           Renderer Functions
// #############################################################################
void draw_quad(Transform transform)
{
  renderData->transforms.add(transform);
}

void draw_quad(Vec2 pos, Vec2 size, DrawData drawData = {})
{
  Transform transform = get_transform(SPRITE_WHITE, pos, size, drawData);
  renderData->transforms.add(transform);
}

void draw_sprite(SpriteID spriteID, Vec2 pos, DrawData drawData = {})
{
  Transform transform = get_transform(spriteID, pos, {}, drawData);
  renderData->transforms.add(transform);
}

void draw_sprite(SpriteID spriteID, IVec2 pos, DrawData drawData = {})
{
  draw_sprite(spriteID, vec_2(pos), drawData);
}

// #############################################################################
//                     Render Interface UI Rendering
// #############################################################################
void draw_ui_sprite(SpriteID spriteID, Vec2 pos, Vec2 size = {}, DrawData drawData = {})
{
  Transform transform = get_transform(spriteID, pos, size, drawData);
  renderData->uiTransforms.add(transform);
}

void draw_ui_sprite(SpriteID spriteID, Vec2 pos, DrawData drawData = {})
{
  Transform transform = get_transform(spriteID, pos, {}, drawData);
  renderData->uiTransforms.add(transform);
}

void draw_ui_sprite(SpriteID spriteID, IVec2 pos, DrawData drawData = {})
{
  draw_ui_sprite(spriteID, vec_2(pos), drawData);
}

// #############################################################################
//                     Render Interface UI Font Rendering
// #############################################################################
void draw_ui_text(char* text, Vec2 pos, TextData textData = {})
{
  SM_ASSERT(text, "No Text Supplied!");
  if(!text)
  {
    return;
  }

  Vec2 origin = pos;
  while(char c = *(text++))
  {
    if(c == '\n')
    {
      pos.y += renderData->fontHeight * textData.fontSize;
      pos.x = origin.x;
      continue;
    }

    Glyph glyph = renderData->glyphs[c];
    Transform transform = {};
    transform.materialIdx = get_material_idx(textData.material);
    transform.pos.x = pos.x + glyph.offset.x * textData.fontSize;
    transform.pos.y = pos.y - glyph.offset.y * textData.fontSize;
    transform.atlasOffset = glyph.textureCoords;
    transform.spriteSize = glyph.size;
    transform.size = vec_2(glyph.size) * textData.fontSize;
    transform.renderOptions = textData.renderOptions | RENDERING_OPTION_FONT;
    transform.layer = textData.layer;

    renderData->uiTransforms.add(transform);

    // Advance the Glyph
    pos.x += glyph.advance.x * textData.fontSize;
  }
}

template <typename... Args>
void draw_format_ui_text(char* format, Vec2 pos, Args... args)
{
  char* text = format_text(format, args...);
  draw_ui_text(text, pos);
}