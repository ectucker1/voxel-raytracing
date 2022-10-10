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
    vec2 targetPos = vScreenPos * offsets.targetSize;
    float scale = min(float(offsets.sourceSize.x) / offsets.targetSize.x, float(offsets.sourceSize.y) / offsets.targetSize.y);
    vec2 scaledTarget = offsets.targetSize * scale;
    vec2 sourcePos = targetPos * scale + (offsets.sourceSize - scaledTarget) / 2;
    vec2 sourceUV = sourcePos / offsets.sourceSize;
    outColor = texture(inputImage, sourceUV);
}