#version 450

layout (location = 0) out vec2 vScreenPos;

layout (push_constant) uniform constants
{
    vec4 camPos;
    vec4 camDir;
    uvec3 volumeBounds;
    float time;
    ivec2 screenSize;
} pushConstants;

void main()
{
    const vec2 pos[3] = vec2[3](
        vec2(-1.0f, -1.0f),
        vec2(3.0f, -1.0f),
        vec2(-1.0f, 3.0f)
    );

    const vec2 uvs[3] = vec2[3](
        vec2(0.0f, 0.0f),
        vec2(2.0f, 0.0f),
        vec2(0.0f, 2.0f)
    );

    vScreenPos = uvs[gl_VertexIndex];
    gl_Position = vec4(pos[gl_VertexIndex], 0.0f, 1.0f);
}
