#version 450

layout (location = 0) in vec2 vScreenPos;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D inputImage;

const uvec2 SCREEN_SIZE = uvec2(1920, 1080);
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
            vec3 colorSample = texture(inputImage, (gl_FragCoord.xy + vec2(float(i), float(j))) / SCREEN_SIZE).rgb;
            float factor = gauss(i, j);
            colorTotal += factor * colorSample;
        }
    }

    outColor = vec4(colorTotal / factorTotal, 1.0);
}