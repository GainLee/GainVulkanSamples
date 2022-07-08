#version 450
layout (binding = 1) uniform sampler2D yuvImgs[3];
layout (binding = 2) uniform sampler3D lutImg;
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
   r = clamp(r, 0.0, 1.0);
   g = clamp(g, 0.0, 1.0);
   b = clamp(b, 0.0, 1.0);

   // 这里需要注意，如果你的LUT图是63*63*63(64*64*64)的，那么它能表示的颜色数量只有二十多万种，
   // 远远低于人眼能感知的一千多万种，这时候我们是能看到有明显的"色彩断层"问题的。所以为了提高精度，
   // sampler的采样方式我们设置为linear，这样经过差值后256*256*256种颜色的图经过LUT映射后依然
   // 是256*256*256的颜色精度，这样就不会有"色彩断层"问题了。
   outColor = texture(lutImg, vec3(r, g, b), 0.0);
}