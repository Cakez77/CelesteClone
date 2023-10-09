#include "schnitzel_lib.h"

#include "input.h"

#include "game.h"

#include "sound.h"

#include "ui.h"

#define APIENTRY
#define GL_GLEXT_PROTOTYPES
#include "glcorearb.h"

static KeyCodeID KeyCodeLookupTable[KEY_COUNT];

#include "platform.h"
#ifdef _WIN32
#include "win32_platform.cpp"
#endif

#include "gl_renderer.cpp"

// #############################################################################
//                           Game DLL Stuff
// #############################################################################
// This is the function pointer to update_game in game.cpp
typedef decltype(update_game) update_game_type;
static update_game_type* update_game_ptr;

// #############################################################################
//                           Cross Platform functions
// #############################################################################
// Used to get Delta Time
#include <chrono>
double get_delta_time();
void reload_game_dll(BumpAllocator* transientStorage);


int main()
{
  // Initialize timestamp
  get_delta_time();

  BumpAllocator transientStorage = make_bump_allocator(MB(50));
  BumpAllocator persistentStorage = make_bump_allocator(MB(256));

  input = (Input*)bump_alloc(&persistentStorage, sizeof(Input));
  if(!input)
  {
    SM_ERROR("Failed to allocate Input");
    return -1;
  }

  renderData = (RenderData*)bump_alloc(&persistentStorage, sizeof(RenderData));
  if(!renderData)
  {
    SM_ERROR("Failed to allocate RenderData");
    return -1;
  }

  gameState = (GameState*)bump_alloc(&persistentStorage, sizeof(GameState));
  if(!gameState)
  {
    SM_ERROR("Failed to allocate GameState");
    return -1;
  }

  uiState = (UIState*)bump_alloc(&persistentStorage, sizeof(UIState));
  if(!uiState)
  {
    SM_ERROR("Failed to allocate UIState")
    return -1;
  }

  soundState = (SoundState*)bump_alloc(&persistentStorage, sizeof(SoundState));
  if(!soundState)
  {
    SM_ERROR("Failed to allocate SoundState");
    return -1;
  }
  soundState->transientStorage = &transientStorage;
  soundState->allocatedsoundsBuffer = bump_alloc(&persistentStorage, SOUNDS_BUFFER_SIZE);
  if(!soundState->allocatedsoundsBuffer)
  {
    SM_ERROR("Failed to allocated Sounds Buffer");
    return -1;
  }

  platform_fill_keycode_lookup_table();
  platform_create_window(1280, 720, "Schnitzel Motor");
  platform_set_vsync(true);
  if(!platform_init_audio())
  {
    SM_ERROR("Failed to initialize Audio");
    return -1;
  }

  gl_init(&transientStorage);

  while(running)
  {
    float dt = get_delta_time();

    reload_game_dll(&transientStorage);

    // Update
    platform_update_window();
    update_game(gameState, renderData, input, soundState, uiState, dt);
    gl_render(&transientStorage);
    platform_update_audio(dt);

    platform_swap_buffers();

    transientStorage.used = 0;
  }

  return 0;
}

void update_game(GameState* gameStateIn, 
                RenderData* renderDataIn, 
                Input* inputIn,
                SoundState* soundStateIn,
                UIState* uiStateIn,
                float dt)
{
  update_game_ptr(gameStateIn ,renderDataIn, inputIn, soundStateIn, uiStateIn, dt);
}

double get_delta_time()
{
  // Only executed once when entering the function (static)
  static auto lastTime = std::chrono::steady_clock::now();
  auto currentTime = std::chrono::steady_clock::now();

  // seconds
  double delta = std::chrono::duration<double>(currentTime - lastTime).count(); 
  lastTime = currentTime; 

  return delta;
}


void reload_game_dll(BumpAllocator* transientStorage)
{
  static void* gameDLL;
  static long long lastEditTimestampGameDLL;

  long long currentTimestampGameDLL = get_timestamp("game.dll");
  if(currentTimestampGameDLL > lastEditTimestampGameDLL)
  {
    if(gameDLL)
    {
      bool freeResult = platform_free_dynamic_library(gameDLL);
      SM_ASSERT(freeResult, "Failed to free game.dll");
      gameDLL = nullptr;
      SM_TRACE("Freed game.dll");
    }

    while(!copy_file("game.dll", "game_load.dll", transientStorage))
    {
      Sleep(10);
    }
    SM_TRACE("Copied game.dll into game_load.dll");

    gameDLL = platform_load_dynamic_library("game_load.dll");
    SM_ASSERT(gameDLL, "Failed to load game.dll");

    update_game_ptr = (update_game_type*)platform_load_dynamic_function(gameDLL, "update_game");
    SM_ASSERT(update_game_ptr, "Failed to load update_game function");
    lastEditTimestampGameDLL = currentTimestampGameDLL;
  }
}




