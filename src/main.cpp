#include "schnitzel_lib.h"

// #############################################################################
//                           Platform Globals
// #############################################################################
static bool running = true;

// #############################################################################
//                           Platform Functions
// #############################################################################
bool platform_create_window(int width, int height, char* title);
void platform_update_window();


// #############################################################################
//                           Windows Platform
// #############################################################################
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>


// #############################################################################
//                           Windows Globals
// #############################################################################
static HWND window;

// #############################################################################
//                           Platform Implementations
// #############################################################################
LRESULT CALLBACK windows_window_callback(HWND window, UINT msg,
                                         WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

  switch(msg)
  {
    case WM_CLOSE:
    {
      running = false;
      break;
    }

    default:
    {
      // Let windows handle the default input for now
      result = DefWindowProcA(window, msg, wParam, lParam);
    }
  }

  return result;
}

bool platform_create_window(int width, int height, char* title)
{
  HINSTANCE instance = GetModuleHandleA(0);

  WNDCLASSA wc = {};
  wc.hInstance = instance;
  wc.hIcon = LoadIcon(instance, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);       // This means we decide the look of the cursor(arrow)
  wc.lpszClassName = title;                       // This is NOT the title, just a unique identifier(ID)
  wc.lpfnWndProc = windows_window_callback;       // Callback for Input into the Window

  if(!RegisterClassA(&wc))
  {
    return false;
  }

  // WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
  int dwStyle = WS_OVERLAPPEDWINDOW;

  window = CreateWindowExA(0, title, // This references lpszClassName from wc
                           title,    // This is the actual Title
                           dwStyle,
                           100,
                           100,
                           width,
                           height,
                           NULL,     // parent
                           NULL,     // menu
                           instance,
                           NULL);    // lpParam

  if(window == NULL)
  {
    return false;
  }

  ShowWindow(window, SW_SHOW);

  return true;
}

void platform_update_window()
{
  MSG msg;

  while(PeekMessageA(&msg, window, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessageA(&msg); // Calls the callback specified when creating the window
  }
}


#endif


int main()
{
  platform_create_window(1200, 720, "Schnitzel Motor");

  int penis = 5;

  while(running)
  {
    // Update
    platform_update_window();

    SM_TRACE("Test %d", penis);
    SM_WARN("Test");
    SM_ERROR("Test");
    SM_ASSERT(false, "Assertion Hit!");
  }

  return 0;
}










