#version 450

layout (location = 0) in vec2 vScreenPos;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D inputImage;

layout (push_constant) uniform constants
{
    vec4 camPos;
    vec4 camDir;
    vec4 camRight;
    vec4 camUp;
    uvec3 volumeBounds;
    uint frame;
    ivec2 screenSize;
    vec2 cameraJitter;
} pushConstants;

const int RADIUS = 1;

const float PI = 3.14159265358979323846;
const float STD = 1.5;

float gauss(int x, int y)
{
    return exp(-float(x * x + y * y) / (2.0 * STD * STD)) / (2.0 * PI * STD * STD);
}

void main()
{
    int numSamples = (RADIUS + 1 + RADIUS) * (RADIUS + 1 + RADIUS);

    float factorTotal = 0.0;
    for (int i = -RADIUS; i <= RADIUS; i++)
    {
        for (int j = -RADIUS; j <= RADIUS; j++)
        {
            float factor = gauss(i, j);
            factorTotal += factor;
        }
    }

    vec3 colorTotal = vec3(0.0);
    for (int i = -RADIUS; i <= RADIUS; i++)
    {
        for (int j = -RADIUS; j <= RADIUS; j++)
        {
            vec3 colorSample = texture(inputImage, (gl_FragCoord.xy + vec2(float(i), float(j))) / pushConstants.screenSize).rgb;
            float factor = gauss(i, j);
            colorTotal += factor * colorSample;
        }
    }

    outColor = vec4(colorTotal / factorTotal, 1.0);
}