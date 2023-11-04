// mac_platform.cpp
#include "platform.h"
#include "schnitzel_lib.h"

// Suppress deprecation warnings for OpenGL functions on Mac
#define GL_SILENCE_DEPRECATION

// Prevent GLFW from including any OpenGL headers on its own,
// to avoid conflicts with the included OpenGL headers.
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h> // Includes the GLFW library for creating windows and handling input.
#include <OpenGL/gl3.h> // Includes the OpenGL 3 header for rendering.

#include <dlfcn.h>  // for loading the so (DLL) file
#include <libgen.h> // For dirname function
#include <unistd.h> // for sleep

GLFWwindow* window;
const char* const windowName = "Schnitzel Motor";

// #############################################################################
// Mac Globals
// #############################################################################
extern bool running;

void error_callback(int error, const char* description)
{
  SM_ERROR("GLFW Error %d: %s", error, description);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  // Ensure the key is within our lookup table range
  if (key >= 0 && key < sizeof(KeyCodeLookupTable) / sizeof(KeyCodeLookupTable[0]))
  {
    bool isDown = action == GLFW_PRESS || action == GLFW_REPEAT;
    KeyCodeID keyCode = KeyCodeLookupTable[key];
    Key* keyState = &input->keys[keyCode];

    // Update the key state
    if (isDown)
    {
      // Key is down
      keyState->justPressed = !keyState->isDown;
      keyState->halfTransitionCount++;
    }
    else if (action == GLFW_RELEASE)
    {
      // Key is up
      keyState->justReleased = keyState->isDown;
      keyState->halfTransitionCount++;
    }

    keyState->isDown = isDown;
  }
}

// Define the cursor position callback as a static function
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  int scaledWidth, scaledHeight;
  glfwGetWindowSize(window, &scaledWidth, &scaledHeight);
  // scaling mouse position is needed to match the screen size
  // with the world size (camera dimensions)
  float scaleX = (float)input->screenSize.x * 1.0f / (float)scaledWidth;
  float scaleY = (float)input->screenSize.y * 1.0f / (float)scaledHeight;

  input->mousePos = {(int)(xpos * scaleX), (int)(ypos * scaleY)};
  input->mousePosWorld = screen_to_world(input->mousePos);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  // Map GLFW mouse button to your KeyCode
  KeyCodeID keyCode = (button == GLFW_MOUSE_BUTTON_LEFT) ? KEY_MOUSE_LEFT : KEY_MOUSE_RIGHT;
  Key* keyState = &input->keys[keyCode];

  // Determine the button state
  bool isDown = (action == GLFW_PRESS);

  // Update the key state
  keyState->justPressed = !keyState->justPressed && !keyState->isDown && isDown;
  keyState->justReleased = !keyState->justReleased && keyState->isDown && !isDown;
  keyState->isDown = isDown;
  keyState->halfTransitionCount++;
}

void frameBufferResizeCallback(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
  input->screenSize.x = width;
  input->screenSize.y = height;
}

void window_close_callback(GLFWwindow* window)
{
  running = false;
}

void platform_fill_keycode_lookup_table()
{
  KeyCodeLookupTable[GLFW_KEY_A] = KEY_A;
  KeyCodeLookupTable[GLFW_KEY_B] = KEY_B;
  KeyCodeLookupTable[GLFW_KEY_C] = KEY_C;
  KeyCodeLookupTable[GLFW_KEY_D] = KEY_D;
  KeyCodeLookupTable[GLFW_KEY_E] = KEY_E;
  KeyCodeLookupTable[GLFW_KEY_F] = KEY_F;
  KeyCodeLookupTable[GLFW_KEY_G] = KEY_G;
  KeyCodeLookupTable[GLFW_KEY_H] = KEY_H;
  KeyCodeLookupTable[GLFW_KEY_I] = KEY_I;
  KeyCodeLookupTable[GLFW_KEY_J] = KEY_J;
  KeyCodeLookupTable[GLFW_KEY_K] = KEY_K;
  KeyCodeLookupTable[GLFW_KEY_L] = KEY_L;
  KeyCodeLookupTable[GLFW_KEY_M] = KEY_M;
  KeyCodeLookupTable[GLFW_KEY_N] = KEY_N;
  KeyCodeLookupTable[GLFW_KEY_O] = KEY_O;
  KeyCodeLookupTable[GLFW_KEY_P] = KEY_P;
  KeyCodeLookupTable[GLFW_KEY_Q] = KEY_Q;
  KeyCodeLookupTable[GLFW_KEY_R] = KEY_R;
  KeyCodeLookupTable[GLFW_KEY_S] = KEY_S;
  KeyCodeLookupTable[GLFW_KEY_T] = KEY_T;
  KeyCodeLookupTable[GLFW_KEY_U] = KEY_U;
  KeyCodeLookupTable[GLFW_KEY_V] = KEY_V;
  KeyCodeLookupTable[GLFW_KEY_W] = KEY_W;
  KeyCodeLookupTable[GLFW_KEY_X] = KEY_X;
  KeyCodeLookupTable[GLFW_KEY_Y] = KEY_Y;
  KeyCodeLookupTable[GLFW_KEY_Z] = KEY_Z;
  KeyCodeLookupTable[GLFW_KEY_0] = KEY_0;
  KeyCodeLookupTable[GLFW_KEY_1] = KEY_1;
  KeyCodeLookupTable[GLFW_KEY_2] = KEY_2;
  KeyCodeLookupTable[GLFW_KEY_3] = KEY_3;
  KeyCodeLookupTable[GLFW_KEY_4] = KEY_4;
  KeyCodeLookupTable[GLFW_KEY_5] = KEY_5;
  KeyCodeLookupTable[GLFW_KEY_6] = KEY_6;
  KeyCodeLookupTable[GLFW_KEY_7] = KEY_7;
  KeyCodeLookupTable[GLFW_KEY_8] = KEY_8;
  KeyCodeLookupTable[GLFW_KEY_9] = KEY_9;
  KeyCodeLookupTable[GLFW_KEY_SPACE] = KEY_SPACE;
  KeyCodeLookupTable[GLFW_KEY_APOSTROPHE] = KEY_TICK;
  KeyCodeLookupTable[GLFW_KEY_MINUS] = KEY_MINUS;
  KeyCodeLookupTable[GLFW_KEY_EQUAL] = KEY_EQUAL;
  KeyCodeLookupTable[GLFW_KEY_LEFT_BRACKET] = KEY_LEFT_BRACKET;
  KeyCodeLookupTable[GLFW_KEY_RIGHT_BRACKET] = KEY_RIGHT_BRACKET;
  KeyCodeLookupTable[GLFW_KEY_SEMICOLON] = KEY_SEMICOLON;
  KeyCodeLookupTable[GLFW_KEY_APOSTROPHE] = KEY_QUOTE;
  KeyCodeLookupTable[GLFW_KEY_COMMA] = KEY_COMMA;
  KeyCodeLookupTable[GLFW_KEY_PERIOD] = KEY_PERIOD;
  KeyCodeLookupTable[GLFW_KEY_SLASH] = KEY_FORWARD_SLASH;
  KeyCodeLookupTable[GLFW_KEY_BACKSLASH] = KEY_BACKWARD_SLASH;
  KeyCodeLookupTable[GLFW_KEY_TAB] = KEY_TAB;
  KeyCodeLookupTable[GLFW_KEY_ESCAPE] = KEY_ESCAPE;
  KeyCodeLookupTable[GLFW_KEY_PAUSE] = KEY_PAUSE;
  KeyCodeLookupTable[GLFW_KEY_UP] = KEY_UP;
  KeyCodeLookupTable[GLFW_KEY_DOWN] = KEY_DOWN;
  KeyCodeLookupTable[GLFW_KEY_LEFT] = KEY_LEFT;
  KeyCodeLookupTable[GLFW_KEY_RIGHT] = KEY_RIGHT;
  KeyCodeLookupTable[GLFW_KEY_BACKSPACE] = KEY_BACKSPACE;
  KeyCodeLookupTable[GLFW_KEY_ENTER] = KEY_RETURN;
  KeyCodeLookupTable[GLFW_KEY_DELETE] = KEY_DELETE;
  KeyCodeLookupTable[GLFW_KEY_INSERT] = KEY_INSERT;
  KeyCodeLookupTable[GLFW_KEY_HOME] = KEY_HOME;
  KeyCodeLookupTable[GLFW_KEY_END] = KEY_END;
  KeyCodeLookupTable[GLFW_KEY_PAGE_UP] = KEY_PAGE_UP;
  KeyCodeLookupTable[GLFW_KEY_PAGE_DOWN] = KEY_PAGE_DOWN;
  KeyCodeLookupTable[GLFW_KEY_CAPS_LOCK] = KEY_CAPS_LOCK;
  KeyCodeLookupTable[GLFW_KEY_NUM_LOCK] = KEY_NUM_LOCK;
  KeyCodeLookupTable[GLFW_KEY_SCROLL_LOCK] = KEY_SCROLL_LOCK;
  KeyCodeLookupTable[GLFW_KEY_LEFT_SHIFT] = KEY_SHIFT;
  KeyCodeLookupTable[GLFW_KEY_RIGHT_SHIFT] = KEY_SHIFT;
  KeyCodeLookupTable[GLFW_KEY_LEFT_CONTROL] = KEY_CONTROL;
  KeyCodeLookupTable[GLFW_KEY_RIGHT_CONTROL] = KEY_CONTROL;
  KeyCodeLookupTable[GLFW_KEY_LEFT_ALT] = KEY_ALT;
  KeyCodeLookupTable[GLFW_KEY_RIGHT_ALT] = KEY_ALT;
  KeyCodeLookupTable[GLFW_KEY_F1] = KEY_F1;
  KeyCodeLookupTable[GLFW_KEY_F2] = KEY_F2;
  KeyCodeLookupTable[GLFW_KEY_F3] = KEY_F3;
  KeyCodeLookupTable[GLFW_KEY_F4] = KEY_F4;
  KeyCodeLookupTable[GLFW_KEY_F5] = KEY_F5;
  KeyCodeLookupTable[GLFW_KEY_F6] = KEY_F6;
  KeyCodeLookupTable[GLFW_KEY_F7] = KEY_F7;
  KeyCodeLookupTable[GLFW_KEY_F8] = KEY_F8;
  KeyCodeLookupTable[GLFW_KEY_F9] = KEY_F9;
  KeyCodeLookupTable[GLFW_KEY_F10] = KEY_F10;
  KeyCodeLookupTable[GLFW_KEY_F11] = KEY_F11;
  KeyCodeLookupTable[GLFW_KEY_F12] = KEY_F12;
  KeyCodeLookupTable[GLFW_KEY_KP_0] = KEY_NUMPAD_0;
  KeyCodeLookupTable[GLFW_KEY_KP_1] = KEY_NUMPAD_1;
  KeyCodeLookupTable[GLFW_KEY_KP_2] = KEY_NUMPAD_2;
  KeyCodeLookupTable[GLFW_KEY_KP_3] = KEY_NUMPAD_3;
  KeyCodeLookupTable[GLFW_KEY_KP_4] = KEY_NUMPAD_4;
  KeyCodeLookupTable[GLFW_KEY_KP_5] = KEY_NUMPAD_5;
  KeyCodeLookupTable[GLFW_KEY_KP_6] = KEY_NUMPAD_6;
  KeyCodeLookupTable[GLFW_KEY_KP_7] = KEY_NUMPAD_7;
  KeyCodeLookupTable[GLFW_KEY_KP_8] = KEY_NUMPAD_8;
  KeyCodeLookupTable[GLFW_KEY_KP_9] = KEY_NUMPAD_9;
}

bool platform_create_window(int width, int height, char* title)
{
  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
    return false;

  glfwWindowHint(GLFW_SAMPLES, 0); // Disable anti-aliasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  input->screenSize.x = width;
  input->screenSize.y = height;

  window = glfwCreateWindow(width, height, windowName, nullptr, nullptr);

  if (!window)
  {
    glfwTerminate();
    return false;
  }

  // Get the actual window size
  int screenWidth, screenHeight;
  glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
  glfwMakeContextCurrent(window);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetWindowCloseCallback(window, window_close_callback);
  // Set the cursor position callback.
  glfwSetCursorPosCallback(window, cursor_position_callback);

  glfwSwapInterval(1);
  glClearColor(0.2f, 0.02f, 0.2f, 1.0f);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);

  glfwSetCursor(window, glfwCreateStandardCursor(GLFW_ARROW_CURSOR));
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  input->screenSize.x = screenWidth;
  input->screenSize.y = screenHeight;
  return true;
}

void platform_update_window()
{
  glfwPollEvents();
}

void* platform_load_gl_function(char* funName)
{
  return (void*)glfwGetProcAddress(funName);
}

void platform_swap_buffers()
{
  glfwSwapBuffers(window);
}

void platform_set_vsync(bool vSync)
{
  glfwSwapInterval(vSync ? 1 : 0);
}

void* platform_load_dynamic_library(const char* dll)
{
  void* handle = dlopen(dll, RTLD_NOW);
  SM_ASSERT(handle, "Failed to load dynamic library: %s", dll);

  return handle;
}

void* platform_load_dynamic_function(void* dll, const char* funName)
{
  void* proc = dlsym(dll, funName);
  SM_ASSERT(proc, "Failed to load function: %s from dynamic library", funName);

  return proc;
}

bool platform_free_dynamic_library(void* dll)
{
  SM_ASSERT(dll, "No dynamic library supplied!");
  int freeResult = dlclose(dll);
  SM_ASSERT(freeResult == 0, "Failed to dlclose");

  return (freeResult == 0);
}

bool platform_init_audio()
{
  return true;
}

void platform_update_audio(float dt)
{
  soundState->playingSounds.clear();
}

void platform_sleep(unsigned int ms)
{
  sleep(ms);
}