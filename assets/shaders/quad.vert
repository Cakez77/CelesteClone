#version 430 core

void main()
{
  // Generating Vertices on the GPU
  // mostly because we have a 2D Engine

  // OpenGL Coordinates
  // -1/ 1                1/ 1
  // -1/-1                1/-1
  vec2 vertices[6] =
  {
    // Top Left
    vec2(-0.5,  0.5),

    // Bottom Left
    vec2(-0.5, -0.5),

    // Top Right
    vec2( 0.5,  0.5),

    // Top Right
    vec2( 0.5,  0.5),

    // Bottom Left
    vec2(-0.5, -0.5),

    // Bottom Right
    vec2( 0.5, -0.5)
  };

  gl_Position = vec4(vertices[gl_VertexID], 1.0, 1.0);
}