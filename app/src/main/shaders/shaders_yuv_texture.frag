#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) in vec2 inUV;
layout (binding = 1) uniform sampler2D yuvSampler[3];
layout (location = 0) out vec4 outColor;
void main() {
   float y, u, v, r, g, b;
   y = texture(yuvSampler[0], inUV).r;
   u = texture(yuvSampler[1], inUV).r;
   v = texture(yuvSampler[2], inUV).r;
   u = u - 0.5;
   v = v - 0.5;
   r = y + 1.403 * v;
   g = y - 0.344 * u - 0.714 * v;
   b = y + 1.770 * u;
   outColor = vec4(r, g, b, 1.0);
//   outColor = vec4(0.0, g, 0.0, 1.0);
}