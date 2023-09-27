#include "gl_renderer.h"

// To Load PNG Files
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "render_interface.h"

// #############################################################################
//                           OpenGL Constants
// #############################################################################
const char* TEXTURE_PATH = "assets/textures/TEXTURE_ATLAS.png";


// #############################################################################
//                           OpenGL Structs
// #############################################################################
struct GLContext
{
  GLuint programID;
  GLuint textureID;
  GLuint transformSBOID;
  GLuint screenSizeID;
  GLuint orthoProjectionID;

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
static void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                         GLsizei length, const GLchar* message, const void* user)
{
  if(severity == GL_DEBUG_SEVERITY_LOW || 
     severity == GL_DEBUG_SEVERITY_MEDIUM ||
     severity == GL_DEBUG_SEVERITY_HIGH)
  {
    SM_ASSERT(false, "OpenGL Error: %s", message);
  }
  else
  {
    SM_TRACE((char*)message);
  }
}

GLuint gl_create_shader(int shaderType, char* shaderPath, BumpAllocator* transientStorage)
{
  int fileSize = 0;
  char* vertShader = read_file(shaderPath, &fileSize, transientStorage);
  if(!vertShader)
  {
    SM_ASSERT(false, "Failed to load shader: %s",shaderPath);
    return 0;
  }

  GLuint shaderID = glCreateShader(shaderType);
  glShaderSource(shaderID, 1, &vertShader, 0);
  glCompileShader(shaderID);

  // Test if Shader compiled successfully 
  {
    int success;
    char shaderLog[2048] = {};

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if(!success)
    {
      glGetShaderInfoLog(shaderID, 2048, 0, shaderLog);
      SM_ASSERT(false, "Failed to compile %s Shader, Error: %s", shaderPath, shaderLog);
      return 0;
    }
  }

  return shaderID;
}


bool gl_init(BumpAllocator* transientStorage)
{
  load_gl_functions();

  glDebugMessageCallback(&gl_debug_callback, nullptr);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glEnable(GL_DEBUG_OUTPUT);

  GLuint vertShaderID = gl_create_shader(GL_VERTEX_SHADER, 
                                         "assets/shaders/quad.vert", transientStorage);
  GLuint fragShaderID = gl_create_shader(GL_FRAGMENT_SHADER, 
                                         "assets/shaders/quad.frag", transientStorage);
  if(!vertShaderID || !fragShaderID)
  {
    SM_ASSERT(false, "Failed to create Shaders")
    return false;
  }

  long long timestampVert = get_timestamp("assets/shaders/quad.vert");
  long long timestampFrag = get_timestamp("assets/shaders/quad.frag");
  glContext.shaderTimestamp = max(timestampVert, timestampFrag);

  glContext.programID = glCreateProgram();
  glAttachShader(glContext.programID, vertShaderID);
  glAttachShader(glContext.programID, fragShaderID);
  glLinkProgram(glContext.programID);

  glDetachShader(glContext.programID, vertShaderID);
  glDetachShader(glContext.programID, fragShaderID);
  glDeleteShader(vertShaderID);
  glDeleteShader(fragShaderID);

  // This has to be done, otherwise OpenGL will not draw anything
  GLuint VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // Texture Loading using STBI
  {
    int width, height, channels;
    char* data = (char*)stbi_load(TEXTURE_PATH, &width, &height, &channels, 4);
    if(!data)
    {
      SM_ASSERT(false, "Failed to load texture");
      return false;
    }

    glGenTextures(1, &glContext.textureID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glContext.textureID);

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // This setting only matters when using the GLSL texture() function
    // When you use texelFetch() this setting has no effect,
    // because texelFetch is designed for this purpose
    // See: https://interactiveimmersive.io/blog/glsl/glsl-data-tricks/
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glContext.textureTimestamp = get_timestamp(TEXTURE_PATH);

    stbi_image_free(data);
  }

  // Transform Storage Buffer
  {
    glGenBuffers(1, &glContext.transformSBOID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, glContext.transformSBOID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Transform) * renderData->transforms.maxElements,
                 renderData->transforms.elements, GL_DYNAMIC_DRAW);
  }

  // Uniforms
  {
    glContext.screenSizeID = glGetUniformLocation(glContext.programID, "screenSize");
    glContext.orthoProjectionID = glGetUniformLocation(glContext.programID, "orthoProjection");
  }
  
  // sRGB output (even if input texture is non-sRGB -> don't rely on texture used)
  // Your font is not using sRGB, for example (not that it matters there, because no actual color is sampled from it)
  // But this could prevent some future bug when you start mixing different types of textures
  // Of course, you still need to correctly set the image file source format when using glTexImage2D()
  glEnable(GL_FRAMEBUFFER_SRGB);
  glDisable(0x809D); // disable multisampling

  // Depth Tesing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);

  // Use Program
  glUseProgram(glContext.programID);

  return true;
}

void gl_render(BumpAllocator* transientStorage)
{
  // Texture Hot Reloading
  {
    long long currentTimestamp = get_timestamp(TEXTURE_PATH);

    if(currentTimestamp > glContext.textureTimestamp)
    {    
      glActiveTexture(GL_TEXTURE0);
      int width, height, nChannels;
      char* data = (char*)stbi_load(TEXTURE_PATH, &width, &height, &nChannels, 4);
      if(data)
      {
        glContext.textureTimestamp = currentTimestamp;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
      }
    }
  }

  // Shader Hot Reloading
  {
    long long timestampVert = get_timestamp("assets/shaders/quad.vert");
    long long timestampFrag = get_timestamp("assets/shaders/quad.frag");
    
    if(timestampVert > glContext.shaderTimestamp ||
       timestampFrag > glContext.shaderTimestamp)
    {
      GLuint vertShaderID = gl_create_shader(GL_VERTEX_SHADER, 
                                              "assets/shaders/quad.vert", transientStorage);
      GLuint fragShaderID = gl_create_shader(GL_FRAGMENT_SHADER, 
                                              "assets/shaders/quad.frag", transientStorage);
      if(!vertShaderID || !fragShaderID)
      {
        SM_ASSERT(false, "Failed to create Shaders")
        return;
      }
      glAttachShader(glContext.programID, vertShaderID);
      glAttachShader(glContext.programID, fragShaderID);
      glLinkProgram(glContext.programID);

      glDetachShader(glContext.programID, vertShaderID);
      glDetachShader(glContext.programID, fragShaderID);
      glDeleteShader(vertShaderID);
      glDeleteShader(fragShaderID);

      glContext.shaderTimestamp = max(timestampVert, timestampFrag);
    }
  }

  glClearColor(119.0f / 255.0f, 33.0f / 255.0f, 111.0f / 255.0f, 1.0f);
  glClearDepth(0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, input->screenSize.x, input->screenSize.y);

  // Copy screen size to the GPU
  Vec2 screenSize = {(float)input->screenSize.x, (float)input->screenSize.y};
  glUniform2fv(glContext.screenSizeID, 1, &screenSize.x);

  // Orthographic Projection
  OrthographicCamera2D camera = renderData->gameCamera;
  Mat4 orthoProjection = orthographic_projection(camera.position.x - camera.dimensions.x / 2.0f, 
                                                 camera.position.x + camera.dimensions.x / 2.0f, 
                                                 camera.position.y - camera.dimensions.y / 2.0f, 
                                                 camera.position.y + camera.dimensions.y / 2.0f);
  glUniformMatrix4fv(glContext.orthoProjectionID, 1, GL_FALSE, &orthoProjection.ax);

  // Opaque Objects
  {
    // Copy transforms to the GPU
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Transform) * renderData->transforms.count,
                    renderData->transforms.elements);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderData->transforms.count);

    // Reset for next Frame
    renderData->transforms.count = 0;
  }
}
















