#version 450
layout (binding = 1) uniform sampler2D yuvImgs[3];
layout (location = 0) in vec2 texturePos;
layout (location = 0) out vec4 outColor;
void main() {
   float y, u, v, r, g, b;
   y = texture(yuvImgs[0], texturePos, 0.0).r;
   u = texture(yuvImgs[1], texturePos, 0.0).r;
   v = texture(yuvImgs[2], texturePos, 0.0).r;
   u = u - 0.5;
   v = v - 0.5;
   r = y + 1.403 * v;
   g = y - 0.344 * u - 0.714 * v;
   b = y + 1.770 * u;
   outColor = vec4(r, g, b, 1.0);
}