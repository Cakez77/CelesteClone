
// Input
const int MAX_TRANSFORMS = 1024;

// Output
layout (location = 0) out vec2 textureCoords;
layout (location = 1) flat out int renderOptions;
layout (location = 2) flat out int materialIdx;

// Buffers
layout (std140) uniform TransformUBO
{
  Transform transforms[MAX_TRANSFORMS];
};

uniform vec2 screenSize;
uniform mat4 orthoProjection;

void main()
{
  Transform transform = transforms[gl_InstanceID];

  // Generating Vertices on the GPU
  // mostly because we have a 2D Engine

  // OpenGL Coordinates
  // -1/ 1                1/ 1
  // -1/-1                1/-1
vec2 vertices[6];
vertices[0] = transform.pos;                                        // Top Left
vertices[1] = vec2(transform.pos + vec2(0.0, transform.size.y));    // Bottom Left
vertices[2] = vec2(transform.pos + vec2(transform.size.x, 0.0));    // Top Right
vertices[3] = vec2(transform.pos + vec2(transform.size.x, 0.0));    // Top Right
vertices[4] = vec2(transform.pos + vec2(0.0, transform.size.y));    // Bottom Left
vertices[5] = transform.pos + transform.size;                       // Bottom Right


  int left = transform.atlasOffset.x;
  int top = transform.atlasOffset.y;
  int right = transform.atlasOffset.x + transform.spriteSize.x;
  int bottom = transform.atlasOffset.y + transform.spriteSize.y;

  if(bool(transform.renderOptions & RENDERING_OPTION_FLIP_X))
  {
    int tmp = left;
    left = right;
    right = tmp;
  }

  if(bool(transform.renderOptions & RENDERING_OPTION_FLIP_Y))
  {
    int tmp = top;
    top = bottom;
    bottom = tmp;
  }

   vec2 localTextureCoords[6];
    localTextureCoords[0] = vec2(left, top);
    localTextureCoords[1] = vec2(left, bottom);
    localTextureCoords[2] = vec2(right, top);
    localTextureCoords[3] = vec2(right, top);
    localTextureCoords[4] = vec2(left, bottom);
    localTextureCoords[5] = vec2(right, bottom);
 
  // Normalize Position
  {
    vec2 vertexPos = vertices[gl_VertexID];
    gl_Position = orthoProjection * vec4(vertexPos, transform.layer, 1.0);
  }

    textureCoords = localTextureCoords[gl_VertexID];
    renderOptions = transform.renderOptions;
    materialIdx = transform.materialIdx;
}