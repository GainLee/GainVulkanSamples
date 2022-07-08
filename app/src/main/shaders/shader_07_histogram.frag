#version 450

layout (binding = 0) readonly buffer InBuffer {
  int colorCount[256];
} histogramBuffer;

layout (location = 0) out vec4 outFragColor;
layout (location = 0) in vec2 texturePos;

void main()
{
  int index = int(255 * texturePos.x);
  int colorCount = histogramBuffer.colorCount[index];

  int maxHeight = 0;
  for (uint i = 0; i < 256; i++)
  {
    maxHeight = max(histogramBuffer.colorCount[i], maxHeight);
  }

  if ((1.0 - texturePos.y)*maxHeight<colorCount) {
    outFragColor = vec4(1.0, 1.0, 1.0, 0.5);
  } else {
    discard;
  }
}