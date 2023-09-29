#pragma once

#include <stdio.h>

// This is to get malloc
#include <stdlib.h>

// This is to get memset
#include <string.h>

// Used to get the edit timestamp of files
#include <sys/stat.h>

// Obvious right?
#include <math.h>

// #############################################################################
//                           Defines
// #############################################################################
#ifdef _WIN32
#define DEBUG_BREAK() __debugbreak()
#define EXPORT_FN __declspec(dllexport)
#elif __linux__
#define DEBUG_BREAK() __builtin_debugtrap()
#define EXPORT_FN
#elif __APPLE__
#define DEBUG_BREAK() __builtin_trap()
#define EXPORT_FN
#endif

#define b8 char
#define BIT(x) 1 << (x)
#define KB(x) ((unsigned long long)1024 * x)
#define MB(x) ((unsigned long long)1024 * KB(x))
#define GB(x) ((unsigned long long)1024 * MB(x))

// #############################################################################
//                           Logging
// #############################################################################
enum TextColor
{  
  TEXT_COLOR_BLACK,
  TEXT_COLOR_RED,
  TEXT_COLOR_GREEN,
  TEXT_COLOR_YELLOW,
  TEXT_COLOR_BLUE,
  TEXT_COLOR_MAGENTA,
  TEXT_COLOR_CYAN,
  TEXT_COLOR_WHITE,
  TEXT_COLOR_BRIGHT_BLACK,
  TEXT_COLOR_BRIGHT_RED,
  TEXT_COLOR_BRIGHT_GREEN,
  TEXT_COLOR_BRIGHT_YELLOW,
  TEXT_COLOR_BRIGHT_BLUE,
  TEXT_COLOR_BRIGHT_MAGENTA,
  TEXT_COLOR_BRIGHT_CYAN,
  TEXT_COLOR_BRIGHT_WHITE,
  TEXT_COLOR_COUNT
};

template <typename ...Args>
void _log(char* prefix, char* msg, TextColor textColor, Args... args)
{
  static char* TextColorTable[TEXT_COLOR_COUNT] = 
  {    
    "\x1b[30m", // TEXT_COLOR_BLACK
    "\x1b[31m", // TEXT_COLOR_RED
    "\x1b[32m", // TEXT_COLOR_GREEN
    "\x1b[33m", // TEXT_COLOR_YELLOW
    "\x1b[34m", // TEXT_COLOR_BLUE
    "\x1b[35m", // TEXT_COLOR_MAGENTA
    "\x1b[36m", // TEXT_COLOR_CYAN
    "\x1b[37m", // TEXT_COLOR_WHITE
    "\x1b[90m", // TEXT_COLOR_BRIGHT_BLACK
    "\x1b[91m", // TEXT_COLOR_BRIGHT_RED
    "\x1b[92m", // TEXT_COLOR_BRIGHT_GREEN
    "\x1b[93m", // TEXT_COLOR_BRIGHT_YELLOW
    "\x1b[94m", // TEXT_COLOR_BRIGHT_BLUE
    "\x1b[95m", // TEXT_COLOR_BRIGHT_MAGENTA
    "\x1b[96m", // TEXT_COLOR_BRIGHT_CYAN
    "\x1b[97m", // TEXT_COLOR_BRIGHT_WHITE
  };

  char formatBuffer[8192] = {};
  sprintf(formatBuffer, "%s %s %s \033[0m", TextColorTable[textColor], prefix, msg);

  char textBuffer[8912] = {};
  sprintf(textBuffer, formatBuffer, args...);

  puts(textBuffer);
}

#define SM_TRACE(msg, ...) _log("TRACE: ", msg, TEXT_COLOR_GREEN, ##__VA_ARGS__);
#define SM_WARN(msg, ...) _log("WARN: ", msg, TEXT_COLOR_YELLOW, ##__VA_ARGS__);
#define SM_ERROR(msg, ...) _log("ERROR: ", msg, TEXT_COLOR_RED, ##__VA_ARGS__);

#define SM_ASSERT(x, msg, ...)    \
{                                 \
  if(!(x))                        \
  {                               \
    SM_ERROR(msg, ##__VA_ARGS__); \
    DEBUG_BREAK();                \
    SM_ERROR("Assertion HIT!")    \
  }                               \
}

// #############################################################################
//                           Array
// #############################################################################
template<typename T, int N>
struct Array
{
  static constexpr int maxElements = N;
  int count = 0;
  T elements[N];

  T& operator[](int idx)
  {
    SM_ASSERT(idx >= 0, "idx negative!");
    SM_ASSERT(idx < count, "Idx out of bounds!");
    return elements[idx];
  }

  int add(T element)
  {
    SM_ASSERT(count < maxElements, "Array Full!");
    elements[count] = element;
    return count++;
  }

  void remove_idx_and_swap(int idx)
  {
    SM_ASSERT(idx >= 0, "idx negative!");
    SM_ASSERT(idx < count, "idx out of bounds!");
    elements[idx] = elements[--count];
  }

  void clear()
  {
    count = 0;
  }

  bool is_full()
  {
    return count == N;
  }
};

// #############################################################################
//                           Bump Allocator
// #############################################################################
struct BumpAllocator
{
  size_t capacity;
  size_t used;
  char* memory;
};

BumpAllocator make_bump_allocator(size_t size)
{
  BumpAllocator ba = {};
  
  ba.memory = (char*)malloc(size);
  if(ba.memory)
  {
    ba.capacity = size;
    memset(ba.memory, 0, size); // Sets the memory to 0
  }
  else
  {
    SM_ASSERT(false, "Failed to allocate Memory!");
  }

  return ba;
}

char* bump_alloc(BumpAllocator* bumpAllocator, size_t size)
{
  char* result = nullptr;

  size_t allignedSize = (size + 7) & ~ 7; // This makes sure the first 4 bits are 0 
  if(bumpAllocator->used + allignedSize <= bumpAllocator->capacity)
  {
    result = bumpAllocator->memory + bumpAllocator->used;
    bumpAllocator->used += allignedSize;
  }
  else
  {
    SM_ASSERT(false, "BumpAllocator is full");
  }

  return result;
}

// #############################################################################
//                           File I/O
// #############################################################################
long long get_timestamp(const char* file)
{
  struct stat file_stat = {};
  stat(file, &file_stat);
  return file_stat.st_mtime;
}

bool file_exists(char* filePath)
{
  SM_ASSERT(filePath, "No filePath supplied!");

  auto file = fopen(filePath, "rb");
  if(!file)
  {
    return false;
  }
  fclose(file);

  return true;
}

long get_file_size(char* filePath)
{
  SM_ASSERT(filePath, "No filePath supplied!");

  long fileSize = 0;
  auto file = fopen(filePath, "rb");
  if(!file)
  {
    SM_ERROR("Failed opening File: %s", filePath);
    return 0;
  }

  fseek(file, 0, SEEK_END);
  fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  fclose(file);

  return fileSize;
}

/*
* Reads a file into a supplied buffer. We manage our own
* memory and therefore want more control over where it 
* is allocated
*/
char* read_file(char* filePath, int* fileSize, char* buffer)
{
  SM_ASSERT(filePath, "No filePath supplied!");
  SM_ASSERT(fileSize, "No fileSize supplied!");
  SM_ASSERT(buffer, "No buffer supplied!");

  *fileSize = 0;
  auto file = fopen(filePath, "rb");
  if(!file)
  {
    SM_ERROR("Failed opening File: %s", filePath);
    return nullptr;
  }

  fseek(file, 0, SEEK_END);
  *fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  memset(buffer, 0, *fileSize + 1);
  fread(buffer, sizeof(char), *fileSize, file);

  fclose(file);

  return buffer;
}

char* read_file(char* filePath, int* fileSize, BumpAllocator* bumpAllocator)
{
  char* file = nullptr;
  long fileSize2 = get_file_size(filePath);

  if(fileSize2)
  {
    char* buffer = bump_alloc(bumpAllocator, fileSize2 + 1);

    file = read_file(filePath, fileSize, buffer);
  }

  return file; 
}

void write_file(char* filePath, char* buffer, int size)
{
  SM_ASSERT(filePath, "No filePath supplied!");
  SM_ASSERT(buffer, "No buffer supplied!");
  auto file = fopen(filePath, "wb");
  if(!file)
  {
    SM_ERROR("Failed opening File: %s", filePath);
    return;
  }

  fwrite(buffer, sizeof(char), size, file);
  fclose(file);
}

bool copy_file(char* fileName, char* outputName, char* buffer)
{
  int fileSize = 0;
  char* data = read_file(fileName, &fileSize, buffer);

  auto outputFile = fopen(outputName, "wb");
  if(!outputFile)
  {
    SM_ERROR("Failed opening File: %s", outputName);
    return false;
  }

  int result = fwrite(data, sizeof(char), fileSize, outputFile);
  if(!result)
  {
    SM_ERROR("Failed opening File: %s", outputName);
    return false;
  }
  
  fclose(outputFile);

  return true;
}

bool copy_file(char* fileName, char* outputName, BumpAllocator* bumpAllocator)
{
  char* file = 0;
  long fileSize2 = get_file_size(fileName);

  if(fileSize2)
  {
    char* buffer = bump_alloc(bumpAllocator, fileSize2 + 1);

    return copy_file(fileName, outputName, buffer);
  }

  return false;
}

// #############################################################################
//                           Math stuff
// #############################################################################
int sign(int x)
{
  return (x >= 0)? 1 : -1;
}

float sign(float x)
{
  return (x >= 0.0f)? 1.0f : -1.0f;
}

long long max(long long a, long long b)
{
  if(a > b)
  {
    return a;
  }

  return b;
}

float max(float a, float b)
{
  if(a > b)
  {
    return a;
  }

  return b;
}

float min(float a, float b)
{
  if(a < b)
  {
    return a;
  }

  return b;
}

float approach(float current, float target, float increase)
{
  if(current < target)
  {
    return min(current + increase, target);
  }
  return max(current - increase, target);
}

float lerp(float a, float b, float t)
{
  return a + (b - a) * t;
}

struct Vec2
{
  float x;
  float y;

  Vec2 operator/(float scalar)
  {
    return {x / scalar, y / scalar};
  }

  Vec2 operator-(Vec2 other)
  {
    return {x - other.x, y - other.y};
  }
};

struct IVec2
{
  int x;
  int y;

  IVec2 operator-(IVec2 other)
  {
    return {x - other.x, y - other.y};
  }
};

Vec2 vec_2(IVec2 v)
{
  return Vec2{(float)v.x, (float)v.y};
}

Vec2 lerp(Vec2 a, Vec2 b, float t)
{
  Vec2 result;
  result.x = lerp(a.x, b.x, t);
  result.y = lerp(a.y, b.y, t);
  return result;
}

IVec2 lerp(IVec2 a, IVec2 b, float t)
{
  IVec2 result;
  result.x = (int)floorf(lerp((float)a.x, (float)b.x, t));
  result.y = (int)floorf(lerp((float)a.y, (float)b.y, t));
  return result;
}

struct Vec4
{
  union
  {
    float values[4];
    struct
    {
      float x;
      float y;
      float z;
      float w;
    };
    
    struct
    {
      float r;
      float g;
      float b;
      float a;
    };
  };

  float& operator[](int idx)
  {
    return values[idx];
  }
};

struct Mat4
{
  union 
  {
    Vec4 values[4];
    struct
    {
      float ax;
      float bx;
      float cx;
      float dx;

      float ay;
      float by;
      float cy;
      float dy;

      float az;
      float bz;
      float cz;
      float dz;
      
      float aw;
      float bw;
      float cw;
      float dw;
    };
  };

  Vec4& operator[](int col)
  {
    return values[col];
  }
};

Mat4 orthographic_projection(float left, float right, float top, float bottom)
{
  Mat4 result = {};
  result.aw = -(right + left) / (right - left);
  result.bw = (top + bottom) / (top - bottom);
  result.cw = 0.0f; // Near Plane
  result[0][0] = 2.0f / (right - left);
  result[1][1] = 2.0f / (top - bottom); 
  result[2][2] = 1.0f / (1.0f - 0.0f); // Far and Near
  result[3][3] = 1.0f;

  return result;
}

struct Rect
{
  Vec2 pos;
  Vec2 size;
};

struct IRect
{
  IVec2 pos;
  IVec2 size;
};

bool point_in_rect(Vec2 point, Rect rect)
{
  return (point.x >= rect.pos.x &&
          point.x <= rect.pos.x + rect.size.x &&
          point.y >= rect.pos.y &&
          point.y <= rect.pos.y + rect.size.y);
}

bool point_in_rect(Vec2 point, IRect rect)
{
  return (point.x >= rect.pos.x &&
          point.x <= rect.pos.x + rect.size.x &&
          point.y >= rect.pos.y &&
          point.y <= rect.pos.y + rect.size.y);
}

bool rect_collision(IRect a, IRect b)
{
  return a.pos.x < b.pos.x  + b.size.x && // Collision on Left of a and right of b
         a.pos.x + a.size.x > b.pos.x  && // Collision on Right of a and left of b
         a.pos.y < b.pos.y  + b.size.y && // Collision on Bottom of a and Top of b
         a.pos.y + a.size.y > b.pos.y;    // Collision on Top of a and Bottom of b
}























