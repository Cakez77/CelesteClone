// Input
layout (location = 0) in vec2 textureCoords;
layout (location = 1) flat in int renderOptions;
layout (location = 2) flat in int materialIdx;

// Output
layout (location = 0) out vec4 fragColor;

// Uniforms
uniform sampler2D textureAtlas;
uniform sampler2D fontAtlas;

// Uniform Block to replace TBO
layout (std140) uniform MaterialBlock
{
    vec4 materials[1024]; 
} materialData;

void main()
{
  // Fetch the material from our UBO using the material index
  vec4 materialColor = materialData.materials[materialIdx];

  if (bool(renderOptions & RENDERING_OPTION_FONT))
  { 
    vec4 textureColor = texelFetch(fontAtlas, ivec2(textureCoords), 0);
    if (textureColor.r == 0.0)
    {
      discard;
    }
    fragColor = textureColor.r * materialColor;
  }
  else
  {
    vec4 textureColor = texelFetch(textureAtlas, ivec2(textureCoords), 0);
    if (textureColor.a == 0.0)
    {
      discard;
    }
    fragColor = textureColor * materialColor;
  }
}
