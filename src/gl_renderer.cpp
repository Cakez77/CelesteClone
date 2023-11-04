#include "gl_renderer.h"
#include "render_interface.h"

// To Load PNG Files
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// To Load TTF Files
#include <ft2build.h>
#include FT_FREETYPE_H

// #############################################################################
//                           OpenGL Constants
// #############################################################################
constexpr int MAX_TRANSFORMS = 1024;
const char* const FONT_PATH = "assets/fonts/AtariClassic-gry3.ttf";
const char* const TEXTURE_PATH = "assets/textures/TEXTURE_ATLAS.png";
const char* const SHADER_HEADER_PATH = "src/shader_header.h";

#ifdef LEGACY_OPENGL
const char* const VERTEX_SHADER_PATH = "assets/shaders/quad_41.vert";
const char* const FRAGMENT_SHADER_PATH = "assets/shaders/quad_41.frag";
const char* const OPENGL_HEADER_VERSION = "#version 410 core\r\n";
#else
const char* const VERTEX_SHADER_PATH = "assets/shaders/quad.vert";
const char* const FRAGMENT_SHADER_PATH = "assets/shaders/quad.frag";
const char* const OPENGL_HEADER_VERSION = "#version 430 core\r\n";
#endif

// #############################################################################
//                           OpenGL Structs
// #############################################################################
struct GLContext
{
  GLuint programID;
  GLuint textureID;
  GLuint vaoID;
#ifdef LEGACY_OPENGL
  GLuint transformUBOID;
  GLuint materialUBOID;
#else
  GLuint transformSBOID;
  GLuint materialSBOID;
#endif
  GLuint screenSizeID;
  GLuint orthoProjectionID;
  GLuint fontAtlasID;

  long long textureTimestamp;
  long long shaderTimestamp;
};

// #############################################################################
//                           OpenGL Globals
// #############################################################################
static GLContext glContext;

// #############################################################################
//                           OpenGL Functions
// #############################################################################
#ifndef LEGACY_OPENGL
static void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                       const GLchar* message, const void* user)
{
  if (severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_HIGH)
  {
    SM_ASSERT(false, "OpenGL Error: %s", message);
  }
  else
  {
    SM_TRACE((char*)message);
  }
}
#endif

inline void gl_clear_errors()
{
  while (glGetError() != GL_NO_ERROR)
    ;
}

const char* get_gl_error_string(GLenum error)
{
  load_gl_functions();
  switch (error)
  {
    case GL_NO_ERROR:
      return "No error";
    case GL_INVALID_ENUM:
      return "Invalid enum";
    case GL_INVALID_VALUE:
      return "Invalid value";
    case GL_INVALID_OPERATION:
      return "Invalid operation";
    case GL_STACK_OVERFLOW:
      return "Stack overflow";
    case GL_STACK_UNDERFLOW:
      return "Stack underflow";
    case GL_OUT_OF_MEMORY:
      return "Out of memory";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "Invalid framebuffer operation";
    default:
      return "Unknown error";
  }
}

GLuint gl_create_shader(int shaderType, const char* shaderPath, BumpAllocator* transientStorage)
{
  int fileSize = 0;
  char* shaderHeader = read_file(SHADER_HEADER_PATH, &fileSize, transientStorage);
  char* shaderSource = read_file(shaderPath, &fileSize, transientStorage);
  if (!shaderHeader || !shaderSource)
  {
    SM_ASSERT(false, "Failed to load shader: %s", shaderPath);
    return 0;
  }

  const char* shaderSources[] = {OPENGL_HEADER_VERSION, shaderHeader, shaderSource};

  GLuint shaderID = glCreateShader(shaderType);
  glShaderSource(shaderID, ArraySize(shaderSources), shaderSources, 0);
  glCompileShader(shaderID);

  // Test if Shader compiled successfully
  {
    int success;
    char shaderLog[2048] = {};

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
      glGetShaderInfoLog(shaderID, 2048, 0, shaderLog);
      SM_ASSERT(false, "Failed to compile %s Shader, Error: %s", shaderPath, shaderLog);
      return 0;
    }
  }

  return shaderID;
}

void hot_reload_shader(BumpAllocator* transientStorage)
{
  long long timestampVert = get_timestamp(VERTEX_SHADER_PATH);
  long long timestampFrag = get_timestamp(FRAGMENT_SHADER_PATH);

  if (timestampVert > glContext.shaderTimestamp || timestampFrag > glContext.shaderTimestamp)
  {
    GLuint vertShaderID = gl_create_shader(GL_VERTEX_SHADER, VERTEX_SHADER_PATH, transientStorage);
    GLuint fragShaderID = gl_create_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_PATH, transientStorage);
    if (!vertShaderID || !fragShaderID)
    {
      SM_ASSERT(false, "Failed to create Shaders")
      return;
    }
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertShaderID);
    glAttachShader(programID, fragShaderID);
    glLinkProgram(programID);

    glDetachShader(programID, vertShaderID);
    glDetachShader(programID, fragShaderID);
    glDeleteShader(vertShaderID);
    glDeleteShader(fragShaderID);

    GLint logLength = 0;
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
      char* programInfoLog = (char*)malloc(logLength);
      if (programInfoLog != NULL)
      {
        glGetProgramInfoLog(programID, logLength, NULL, programInfoLog);
        SM_ASSERT(0, "Failed to link program: %s", programInfoLog);
        free(programInfoLog);
      }
    }
    glDeleteProgram(glContext.programID);
    glContext.programID = programID;
    glUseProgram(programID);

    glContext.shaderTimestamp = max(timestampVert, timestampFrag);
  }
}

GLuint create_shaders(BumpAllocator* transientStorage)
{
  GLuint vertShaderID;
  GLuint fragShaderID;

  vertShaderID = gl_create_shader(GL_VERTEX_SHADER, VERTEX_SHADER_PATH, transientStorage);
  fragShaderID = gl_create_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_PATH, transientStorage);

  if (!vertShaderID || !fragShaderID)
  {
    SM_ASSERT(false, "Failed to create Shaders")
    return 0;
  }

  long long timestampVert = get_timestamp(VERTEX_SHADER_PATH);
  long long timestampFrag = get_timestamp(FRAGMENT_SHADER_PATH);
  glContext.shaderTimestamp = max(timestampVert, timestampFrag);

  GLuint programID = glCreateProgram();
  glAttachShader(programID, vertShaderID);
  glAttachShader(programID, fragShaderID);
  glLinkProgram(programID);

  // Validate if program works
  {
    GLint programSuccess;
    glGetProgramiv(programID, GL_LINK_STATUS, &programSuccess);

    if (!programSuccess)
    {
      GLint logLength = 0;
      glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);

      if (logLength > 0)
      {
        char* programInfoLog = (char*)malloc(logLength);
        if (programInfoLog == NULL)
        {
          SM_ASSERT(0, "Failed to allocate memory for program log");
           return 0;
        }

        glGetProgramInfoLog(programID, logLength, NULL, programInfoLog);
        SM_ASSERT(0, "Failed to link program: %s", programInfoLog);
        free(programInfoLog);
      }
      else
      {
        SM_ASSERT(0, "Failed to link program: No info log available");
      }
    }
  }

  glDetachShader(programID, vertShaderID);
  glDetachShader(programID, fragShaderID);
  glDeleteShader(vertShaderID);
  glDeleteShader(fragShaderID);

  return programID;
}

void create_game_texture()
{
  int width, height, channels;
  char* data = (char*)stbi_load(TEXTURE_PATH, &width, &height, &channels, 4);
  if (!data)
  {
    SM_ASSERT(false, "Failed to load texture");
  }
  int textureSizeInBytes = 4 * width * height;

  if (!data)
  {
    SM_ASSERT(0, "Failed to load Texture!");
    return;
  }

  glGenTextures(1, &glContext.textureID);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, glContext.textureID);
  glUniform1i(glGetUniformLocation(glContext.programID, "textureAtlas"),
              0); // Corresponds to GL_TEXTURE0

  // set the texture wrapping/filtering options (on the currently bound texture object)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  // This setting only matters when using the GLSL texture() function
  // When you use texelFetch() this setting has no effect,
  // because texelFetch is designed for this purpose
  // See: https://interactiveimmersive.io/blog/glsl/glsl-data-tricks/
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glContext.textureTimestamp = get_timestamp(TEXTURE_PATH);

  stbi_image_free(data);
}

void hot_reload_texture()
{
  long long currentTimestamp = get_timestamp(TEXTURE_PATH);

  if (currentTimestamp > glContext.textureTimestamp)
  {
    glActiveTexture(GL_TEXTURE0);
    int width, height, nChannels;
    char* data = (char*)stbi_load(TEXTURE_PATH, &width, &height, &nChannels, 4);
    if (data)
    {
      glContext.textureTimestamp = currentTimestamp;
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    }
  }
}

void load_font(const char* filePath, int fontSize)
{
  FT_Library fontLibrary;
  FT_Init_FreeType(&fontLibrary);

  FT_Face fontFace;
  FT_New_Face(fontLibrary, filePath, 0, &fontFace);
  FT_Set_Pixel_Sizes(fontFace, 0, fontSize);

  int padding = 2;
  int row = 0;
  int col = padding;

  const int TEXTURE_WIDTH = 512;
  char textureBuffer[TEXTURE_WIDTH * TEXTURE_WIDTH];
  for (FT_ULong glyphIdx = 32; glyphIdx < 127; ++glyphIdx)
  {
    FT_UInt glyphIndex = FT_Get_Char_Index(fontFace, glyphIdx);
    FT_Load_Glyph(fontFace, glyphIndex, FT_LOAD_DEFAULT);
    FT_Error error = FT_Render_Glyph(fontFace->glyph, FT_RENDER_MODE_NORMAL);

    if (col + fontFace->glyph->bitmap.width + padding >= 512)
    {
      col = padding;
      row += fontSize;
    }

    // Font Height
    renderData->fontHeight =
        max((fontFace->size->metrics.ascender - fontFace->size->metrics.descender) >> 6, renderData->fontHeight);

    for (unsigned int y = 0; y < fontFace->glyph->bitmap.rows; ++y)
    {
      for (unsigned int x = 0; x < fontFace->glyph->bitmap.width; ++x)
      {
        textureBuffer[(row + y) * TEXTURE_WIDTH + col + x] =
            fontFace->glyph->bitmap.buffer[y * fontFace->glyph->bitmap.width + x];
      }
    }

    Glyph* glyph = &renderData->glyphs[glyphIdx];
    glyph->textureCoords = {col, row};
    glyph->size = {(int)fontFace->glyph->bitmap.width, (int)fontFace->glyph->bitmap.rows};
    glyph->advance = {(float)(fontFace->glyph->advance.x >> 6), (float)(fontFace->glyph->advance.y >> 6)};
    glyph->offset = {
        (float)fontFace->glyph->bitmap_left,
        (float)fontFace->glyph->bitmap_top,
    };

    col += fontFace->glyph->bitmap.width + padding;
  }

  FT_Done_Face(fontFace);
  FT_Done_FreeType(fontLibrary);

  // Upload OpenGL Texture
  {
    glGenTextures(1, &glContext.fontAtlasID);
    glActiveTexture(GL_TEXTURE1); // Bound to binding = 1, see quad.frag
    glBindTexture(GL_TEXTURE_2D, glContext.fontAtlasID);
    glUniform1i(glGetUniformLocation(glContext.programID, "fontAtlas"),
                1); // Corresponds to GL_TEXTURE1

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TEXTURE_WIDTH, TEXTURE_WIDTH, 0, GL_RED, GL_UNSIGNED_BYTE,
                 (char*)textureBuffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
}

#ifdef LEGACY_OPENGL
void create_gl_buffers()
{
  // Material UBO
  {
    // Generate material UBO
    glGenBuffers(1, &glContext.materialUBOID);
    glBindBuffer(GL_UNIFORM_BUFFER, glContext.materialUBOID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Material) * MAX_TRANSFORMS, renderData->materials.elements, GL_DYNAMIC_DRAW);

    // Attach the UBO to a binding point
    GLuint bindingPoint = 2; // example binding point
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, glContext.materialUBOID);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  // Transform Uniform Buffer
  {
    glGenBuffers(1, &glContext.transformUBOID);
    glBindBuffer(GL_UNIFORM_BUFFER, glContext.transformUBOID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Transform) * MAX_TRANSFORMS, renderData->transforms.elements,
                 GL_DYNAMIC_DRAW);

    // Attach the UBO to a binding point
    GLuint bindingPoint = 0; // example binding point for transforms
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, glContext.transformUBOID);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }
}
#else
void create_gl_buffers()
{

  // Transform Storage Buffer
  {
    glGenBuffers(1, &glContext.transformSBOID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, glContext.transformSBOID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Transform) * renderData->transforms.maxElements,
                 renderData->transforms.elements, GL_DYNAMIC_DRAW);
  }

  // Materials Storage Buffer
  {
    glGenBuffers(1, &glContext.materialSBOID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, glContext.materialSBOID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Material) * renderData->materials.maxElements,
                 renderData->materials.elements, GL_DYNAMIC_DRAW);
  }
}
#endif

bool gl_init(BumpAllocator* transientStorage)
{
  load_gl_functions();
#ifndef LEGACY_OPENGL
  // OpenGL Debug Callback (only works with OpenGL 4.3+)
  glDebugMessageCallback(&gl_debug_callback, nullptr);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glEnable(GL_DEBUG_OUTPUT);
#endif

  gl_clear_errors();
  glContext.programID = create_shaders(transientStorage);
  if (!glContext.programID)
  {
    SM_ASSERT(false, "Failed to create Shaders");
    return false;
  }

  // Use Program
  glUseProgram(glContext.programID);

  // This has to be done, otherwise OpenGL will not draw anything
  glGenVertexArrays(1, &glContext.vaoID);
  glBindVertexArray(glContext.vaoID);

  create_gl_buffers();

  create_game_texture();

  load_font(FONT_PATH, 8);

  // Uniforms
  {
    glContext.screenSizeID = glGetUniformLocation(glContext.programID, "screenSize");
    glContext.orthoProjectionID = glGetUniformLocation(glContext.programID, "orthoProjection");

#ifdef LEGACY_OPENGL
    // Get the index of the uniform block
    GLuint materialBlockIndex = glGetUniformBlockIndex(glContext.programID, "MaterialBlock");
    // Bind the uniform block to binding point 2
    glUniformBlockBinding(glContext.programID, materialBlockIndex, 2);
#endif
  }

  // sRGB output (even if input texture is non-sRGB -> don't rely on texture used)
  // Your font is not using sRGB, for example (not that it matters there, because no
  // actual color is sampled from it) But this could prevent some future bug when you
  // start mixing different types of textures Of course, you still need to correctly set
  // the image file source format when using glTexImage2D()
  glEnable(GL_FRAMEBUFFER_SRGB);
  glDisable(0x809D); // disable multisampling

  // Depth Tesing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);

  return true;
}

template <typename T, int N> void render_ubo_chunks(Array<T, N>& elements, bool skipDraw)
{
  size_t chunkSize = MAX_TRANSFORMS; // fixed size because shader does not support dynamic array
  size_t chunkCount = (elements.count + chunkSize - 1) / chunkSize;

  for (size_t chunk = 0; chunk < chunkCount; ++chunk)
  {
    size_t offset = chunk * chunkSize;
    size_t currentSize = min(chunkSize, elements.count - offset);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, currentSize * sizeof(T), &elements[offset]);
    if (!skipDraw)
    {
      glDrawArraysInstanced(GL_TRIANGLES, 0, 6, currentSize);
    }
  }
  elements.clear();
}

void gl_render(BumpAllocator* transientStorage)
{
    hot_reload_texture();
    hot_reload_shader(transientStorage);

    glClearColor(119.0f / 255.0f, 33.0f / 255.0f, 111.0f / 255.0f, 1.0f);
    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, input->screenSize.x, input->screenSize.y);

    Vec2 screenSize = {(float)input->screenSize.x, (float)input->screenSize.y};
    glUniform2fv(glContext.screenSizeID, 1, &screenSize.x);

    Mat4 gameOrthoprojection = get_ortho_projection(renderData->gameCamera);
    Mat4 uiOrthoprojection = get_ortho_projection(renderData->uiCamera);

#ifdef LEGACY_OPENGL
    // Material UBO Draw
    //  Send material to the GPU and update the material UBO
    {
      glBindBuffer(GL_UNIFORM_BUFFER, glContext.materialUBOID);
      render_ubo_chunks(renderData->materials, true);
      glBindBuffer(GL_UNIFORM_BUFFER, glContext.transformUBOID);
    }
    
    // Transform UBO Draw
    // Set orthographic projection and update transform UBO
    {
      glUniformMatrix4fv(glContext.orthoProjectionID, 1, GL_FALSE, &gameOrthoprojection.ax);
      render_ubo_chunks(renderData->transforms, false);
    }
    // UI Transform UBO Draw
    // Set orthographic projection and update UI transform UBO
    {
      glUniformMatrix4fv(glContext.orthoProjectionID, 1, GL_FALSE, &uiOrthoprojection.ax);
      render_ubo_chunks(renderData->uiTransforms, false);
    }
#else
    // Material SBO Draw
    {
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, glContext.materialSBOID);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Material) * renderData->materials.count,
                      renderData->materials.elements);
      renderData->materials.clear();
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, glContext.transformSBOID);
    }
    // Transform UBO Draw
    {
      glUniformMatrix4fv(glContext.orthoProjectionID, 1, GL_FALSE, &gameOrthoprojection.ax);

      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Transform) * renderData->transforms.count,
                      renderData->transforms.elements);
      glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderData->transforms.count);
      renderData->transforms.clear();
    }

    // UI Transform UBO Draw
    {
      glUniformMatrix4fv(glContext.orthoProjectionID, 1, GL_FALSE, &uiOrthoprojection.ax);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Transform) * renderData->uiTransforms.count,
                      renderData->uiTransforms.elements);
      glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderData->uiTransforms.count);
      renderData->uiTransforms.clear();
    }
#endif
}
