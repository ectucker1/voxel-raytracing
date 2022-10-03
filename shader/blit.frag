#version 450

layout (location = 0) in vec2 vScreenPos;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D inputImage;
layout (set = 0, binding = 1) uniform BlitOffsets
{
    uvec2 sourceSize;
    uvec2 targetSize;
} offsets;

void main()
{
    outColor = texture(inputImage, vScreenPos);
}