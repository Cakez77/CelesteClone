// THESE ARE NOT NEEDED, VSCODE IS DOOOOOOOOOOGGGG SHIT
#include "input.h"
#include "platform.h"

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <dlfcn.h>  // for loading the so (DLL) file
#include <unistd.h> // for sleep

// #############################################################################
//                           Linux Defines
// #############################################################################
static constexpr int BUTTONS_KEYCODE_OFFSET = 250;

// #############################################################################
//                           Linux Globals
// #############################################################################
extern bool running;  // <- declare, but do not define
static PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT_ptr;
static Display* display;
static Atom wmDeleteWindow;
static Window window;

// #############################################################################
//                           Platform Implementations
// #############################################################################
bool platform_create_window(int width, int height, char* title)
{
  display = XOpenDisplay(NULL);
  window = XCreateSimpleWindow(display, 
                               DefaultRootWindow(display),
                               10,      // xPos
                               10,      // yPos
                               width, 
                               height,
                               0,       // border width
                               0,       // border
                               0);       // background

  int pixelAttribs[] = 
  {
    // GLX_RGBA,           True, // Aparently this doesn't work on all Linux Systems, have to test
    GLX_DOUBLEBUFFER,   True,
    GLX_RED_SIZE,          8,
    GLX_GREEN_SIZE,        8,
    GLX_BLUE_SIZE,         8,
    None
  };

  int fbcCount = 0;
  GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display),
                                       pixelAttribs, &fbcCount);

  if (!fbc) 
  {
    SM_ASSERT(0, "glXChooseFBConfig() failed");
    return false;
  }

  PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
    (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
  glXSwapIntervalEXT_ptr = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");

  // Set desired minimum OpenGL version
  int contextAttribs[] = 
  {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB, 
    None
  };

  // Create modern OpenGL context
  GLXContext rc = glXCreateContextAttribsARB(display, fbc[0], NULL, true, contextAttribs);

  // Set the input mask for our window on the current display
  // ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | 
  // PointerMotionMask | ButtonMotionMask | FocusChangeMask
  long event_mask = ExposureMask
                  | KeyPressMask | KeyReleaseMask
                  | ButtonPressMask | ButtonReleaseMask;
  XSelectInput(display, window, event_mask);

  XMapWindow(display, window);
  glXMakeCurrent(display, window, rc);

  // Tell the server to notify us when the window manager attempts to destroy the window
  wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteWindow, 1);

  return true;
}

void platform_update_window()
{
  Window root;
  Window child;
  int root_x;
  int root_y;
  int win_x;
  int win_y;
  unsigned int mask_return;
  XQueryPointer(display, window, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask_return);

  input->mousePos = IVec2{win_x, win_y};

  // Mouse Position World
  input->mousePosWorld = screen_to_world(input->mousePos);

  // XPending doesn't block
  while (XPending(display))
  {
    XEvent event;
    XNextEvent(display, &event);

    switch(event.type)
    {
      case Expose:
      {
        input->screenSize.x = event.xexpose.width;
        input->screenSize.y = event.xexpose.height;

        break;
      }

      case KeyPress:
      case KeyRelease:
      {
        bool isDown = event.type == KeyPress;
        KeyCodeID keyCode = KeyCodeLookupTable[event.xkey.keycode];
        Key* key = &input->keys[keyCode];

        key->justPressed = !key->justPressed && !key->isDown && isDown;
        key->justReleased = !key->justReleased && key->isDown && !isDown;
        key->isDown = isDown;
        key->halfTransitionCount++;

        break;
      }

      case ButtonPress:
      case ButtonRelease:
      {
        bool isDown = event.type == ButtonPress;
        KeyCodeID keyCode = KeyCodeLookupTable[BUTTONS_KEYCODE_OFFSET + event.xbutton.button];
        Key* key = &input->keys[keyCode];

        key->justPressed = !key->justPressed && !key->isDown && isDown;
        key->justReleased = !key->justReleased && key->isDown && !isDown;
        key->isDown = isDown;
        key->halfTransitionCount++;

        break;
      }

      case ClientMessage:
      {
        // Client message event: Window close event
        Atom wmProtocol = XInternAtom(display, "WM_PROTOCOLS", False);
        if (event.xclient.message_type == wmProtocol &&
            event.xclient.data.l[0] == wmDeleteWindow)
        {
            // Window close event received
            running = false;
            break;
        }
      }
    }
  }
}

void* platform_load_gl_function(char* funName)
{
  void* proc = (void*)glXGetProcAddress((const GLubyte*)funName);
  if(!proc)
  {
    SM_ASSERT(0, "Failed to load OpenGL Function: %s", funName);
  }

  return proc;
}

void platform_swap_buffers()
{
  glXSwapBuffers(display, window);
}

void platform_set_vsync(bool vSync)
{
  glXSwapIntervalEXT_ptr(display, window, vSync);
}

void* platform_load_dynamic_library(const char* dll)
{    
  char path[256] = {};
  sprintf(path, "./%s", dll);
  void* lib = dlopen(path, RTLD_NOW);
  char *errstr = dlerror(); 
  if (errstr != NULL) 
  {
    SM_ASSERT(false, "A dynamic linking error occurred: (%s)\n", errstr);
  }
  SM_ASSERT(lib, "Failed to load lib: %s", dll);

  return lib;
}

void* platform_load_dynamic_function(void* dll, const char* funName)
{
  void* proc = dlsym(dll, funName);
  SM_ASSERT(proc, "Failed to load function: %s from lib", funName);

  return proc;
}

bool platform_free_dynamic_library(void* dll)
{
  SM_ASSERT(dll, "No lib supplied!");
  int freeResult = dlclose(dll);
  SM_ASSERT(!freeResult, "Failed to dlclose");

  return (bool)freeResult;
}

void platform_fill_keycode_lookup_table()
{
  // Mouse
  KeyCodeLookupTable[BUTTONS_KEYCODE_OFFSET + Button1] = KEY_MOUSE_LEFT;
  KeyCodeLookupTable[BUTTONS_KEYCODE_OFFSET + Button2] = KEY_MOUSE_MIDDLE;
  KeyCodeLookupTable[BUTTONS_KEYCODE_OFFSET + Button3] = KEY_MOUSE_RIGHT;

  // A - Z
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_A)] = KEY_A;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_B)] = KEY_B;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_C)] = KEY_C;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_D)] = KEY_D;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_E)] = KEY_E;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F)] = KEY_F;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_G)] = KEY_G;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_H)] = KEY_H;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_I)] = KEY_I;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_J)] = KEY_J;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_K)] = KEY_K;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_L)] = KEY_L;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_M)] = KEY_M;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_N)] = KEY_N;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_O)] = KEY_O;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_P)] = KEY_P;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Q)] = KEY_Q;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_R)] = KEY_R;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_S)] = KEY_S;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_T)] = KEY_T;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_U)] = KEY_U;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_V)] = KEY_V;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_W)] = KEY_W;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_X)] = KEY_X;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Y)] = KEY_Y;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Z)] = KEY_Z;

  // a - z
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_a)] = KEY_A;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_b)] = KEY_B;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_c)] = KEY_C;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_d)] = KEY_D;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_e)] = KEY_E;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_f)] = KEY_F;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_g)] = KEY_G;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_h)] = KEY_H;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_i)] = KEY_I;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_j)] = KEY_J;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_k)] = KEY_K;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_l)] = KEY_L;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_m)] = KEY_M;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_n)] = KEY_N;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_o)] = KEY_O;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_p)] = KEY_P;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_q)] = KEY_Q;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_r)] = KEY_R;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_s)] = KEY_S;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_t)] = KEY_T;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_u)] = KEY_U;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_v)] = KEY_V;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_w)] = KEY_W;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_x)] = KEY_X;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_y)] = KEY_Y;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_z)] = KEY_Z;

  KeyCodeLookupTable[XKeysymToKeycode(display, XK_0)] = KEY_0;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_1)] = KEY_1;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_2)] = KEY_2;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_3)] = KEY_3;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_4)] = KEY_4;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_5)] = KEY_5;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_6)] = KEY_6;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_7)] = KEY_7;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_8)] = KEY_8;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_9)] = KEY_9;

  KeyCodeLookupTable[XKeysymToKeycode(display, XK_space)] = KEY_SPACE;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_grave)] = KEY_TICK;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_minus)] = KEY_MINUS;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_equal)] = KEY_EQUAL;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_bracketleft)] = KEY_LEFT_BRACKET;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_bracketright)] = KEY_RIGHT_BRACKET;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_semicolon)] = KEY_SEMICOLON;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_quotedbl)] = KEY_QUOTE;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_comma)] = KEY_COMMA;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_period)] = KEY_PERIOD;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_slash)] = KEY_FORWARD_SLASH;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_backslash)] = KEY_BACKWARD_SLASH;

  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Tab)] = KEY_TAB;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Escape)] = KEY_ESCAPE;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Pause)] = KEY_PAUSE;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Up)] = KEY_UP;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Down)] = KEY_DOWN;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Left)] = KEY_LEFT;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Right)] = KEY_RIGHT;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_BackSpace)] = KEY_BACKSPACE;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Return)] = KEY_RETURN;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Delete)] = KEY_DELETE;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Insert)] = KEY_INSERT;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Home)] = KEY_HOME;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_End)] = KEY_END;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Page_Up)] = KEY_PAGE_UP;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Page_Down)] = KEY_PAGE_DOWN;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Caps_Lock)] = KEY_CAPS_LOCK;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Num_Lock)] = KEY_NUM_LOCK;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Scroll_Lock)] = KEY_SCROLL_LOCK;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Menu)] = KEY_MENU;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Shift_L)] = KEY_SHIFT;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Shift_R)] = KEY_SHIFT;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Control_L)] = KEY_CONTROL;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Control_R)] = KEY_CONTROL;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Alt_L)] = KEY_ALT;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_Alt_R)] = KEY_ALT;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F1)] = KEY_F1;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F2)] = KEY_F2;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F3)] = KEY_F3;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F4)] = KEY_F4;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F5)] = KEY_F5;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F6)] = KEY_F6;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F7)] = KEY_F7;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F8)] = KEY_F8;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F9)] = KEY_F9;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F10)] = KEY_F10;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F11)] = KEY_F11;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_F12)] = KEY_F12;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_0)] = KEY_NUMPAD_0;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_1)] = KEY_NUMPAD_1;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_2)] = KEY_NUMPAD_2;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_3)] = KEY_NUMPAD_3;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_4)] = KEY_NUMPAD_4;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_5)] = KEY_NUMPAD_5;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_6)] = KEY_NUMPAD_6;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_7)] = KEY_NUMPAD_7;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_8)] = KEY_NUMPAD_8;
  KeyCodeLookupTable[XKeysymToKeycode(display, XK_KP_9)] = KEY_NUMPAD_9;
}

bool platform_init_audio()
{
  // TODO: Linux Audio monakS
  return true;
}

void platform_update_audio(float dt)
{
  // TODO: Please I don't want to do this!!!!!
  soundState->playingSounds.clear();
}

void platform_sleep(unsigned int ms)
{
  sleep(ms);
}