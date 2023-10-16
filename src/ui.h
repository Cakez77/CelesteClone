#pragma once

#include "assets.h"
#include "input.h"
#include "render_interface.h"


// #############################################################################
//                           UI Constants
// #############################################################################
constexpr int MAX_UI_ELEMENTS = 100;
constexpr int MAX_TEXT_CHARS = 256;

// #############################################################################
//                           UI Structs
// #############################################################################
struct UIID
{
  int ID;
  int layer;
};

struct UIElement
{
  SpriteID spriteID;
  Vec2 pos;
  Vec2 size;
  DrawData drawData;
};

struct UIText
{
  int charCount;
  char text[MAX_TEXT_CHARS];
  Vec2 pos;
  TextData textData;
};

struct UIState
{
  UIID hotLastFrame;
  UIID hotThisFrame;
  UIID active;

  int layer = 0;

  Array<UIText, 100> uiTexts;
  Array<UIElement, MAX_UI_ELEMENTS> uiElements;
};

// #############################################################################
//                           UI Globals
// #############################################################################
static UIState* uiState;

// #############################################################################
//                           UI Functions
// #############################################################################
void update_ui()
{
  if(!key_is_down(KEY_MOUSE_LEFT) && 
     !key_released_this_frame(KEY_MOUSE_LEFT))
  {
    uiState->active = {};
  }

  uiState->uiElements.clear();
  uiState->uiTexts.clear();
  uiState->hotLastFrame = uiState->hotThisFrame;
  uiState->hotThisFrame = {};
}

void set_active(int ID)
{
  uiState->active = {ID, 0};
}

void set_hot(int ID, int layer = 0)
{
  if(uiState->hotThisFrame.layer <= layer)
  {
    uiState->hotThisFrame.ID = ID;
    uiState->hotThisFrame.layer = layer;
  }
}

bool is_active(int ID)
{
  return uiState->active.ID && uiState->active.ID == ID;
}

bool is_hot(int ID)
{
  return uiState->hotLastFrame.ID && uiState->hotLastFrame.ID == ID;
}

bool ui_is_hot()
{
  return uiState->hotLastFrame.ID || uiState->hotLastFrame.ID;
}

bool ui_is_active()
{
  return uiState->active.ID;
}

bool do_button(SpriteID spriteID, IVec2 pos, int ID, DrawData drawData = {})
{
  Sprite sprite = get_sprite(spriteID);

  IRect rect = {pos.x - sprite.size.x / 2, 
                pos.y - sprite.size.y / 2,
                sprite.size};
  if(is_active(ID))
  {
    pos.y += 2;

    if(key_released_this_frame(KEY_MOUSE_LEFT) &&
       point_in_rect(input->mousePosWorld, rect))
    {
      // Set inactive     
      uiState->active = {};
      return true;
    }
  }
  else if(is_hot(ID))
  {
    if(key_pressed_this_frame(KEY_MOUSE_LEFT))
    {
      set_active(ID);
    }
  }

  if(point_in_rect(input->mousePosWorld, rect))
  {
    set_hot(ID, uiState->layer);
  }

  // Draw UI Element (Adds to an array of elements to draw during render())
  {
    UIElement uiElement = 
    {
      .spriteID = spriteID,
      .pos = vec_2(pos),
      .drawData = drawData,
    };
    uiState->uiElements.add(uiElement);
  }

  return false;
}

void do_ui_text(const char* text, Vec2 pos, TextData textData = {})
{
  SM_ASSERT(text, "No Text supplied!");
  SM_ASSERT(strlen(text) < MAX_TEXT_CHARS, "Text too long!");

  UIText uiText = {};
  memcpy(uiText.text, text, strlen(text));
  uiText.charCount = strlen(text);
  uiText.pos = pos;
  uiText.textData = textData;

  uiState->uiTexts.add(uiText);
}

template <typename... Args>
void do_format_ui_text(const char* format, Vec2 pos, TextData textData = {}, Args... args)
{
  char* text = format_text(format, args...);
  do_ui_text(text, pos, textData);
}

void do_ui_quad(Vec2 pos, Vec2 size, DrawData drawData = {})
{
  UIElement uiElement = 
  {
    .spriteID = SPRITE_WHITE,
    .pos = pos,
    .size = size,
    .drawData = drawData,
  };
  uiState->uiElements.add(uiElement);
}