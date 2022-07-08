#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUVPos;

layout (binding = 0) uniform UBO
{
    mat4 projectionMatrix;
    mat4 modelMatrix;
    mat4 viewMatrix;
} ubo;

layout (push_constant) uniform PushContant{
    float LUT_ITEM_WIDTH;
    float WINDOW_WIDTH;
} lutContant;

layout (constant_id = 0) const float LUT_ITEM_INDEX = 0.0f;

layout (location = 0) out vec2 texturePos;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    texturePos = inUVPos;
    float x_offset = (LUT_ITEM_INDEX * lutContant.LUT_ITEM_WIDTH) * 2.0f /lutContant.WINDOW_WIDTH;
    vec4 pos = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * inPos;
    gl_Position = vec4(pos.x + x_offset, pos.yzw);
}