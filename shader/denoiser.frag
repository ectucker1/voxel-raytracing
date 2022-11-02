#version 450

layout (location = 0) in vec2 vScreenPos;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D inputColor;
layout (set = 0, binding = 1) uniform sampler2D inputNormal;
layout (set = 0, binding = 2) uniform sampler2D inputPos;
layout (set = 0, binding = 3) uniform denoiserParams
{
    float phiColor;
    float phiNormal;
    float phiPos;
    float stepWidth;
} params;
layout (set = 0, binding = 4) uniform Kernel {
    float kernel[9];
};
layout (set = 0, binding = 5) uniform Offsets {
    vec2 offset[9];
};

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

// Edge-avoiding A-Trous filter [Dammertz et al. 2010]
// https://jo.dreggn.org/home/2010_atrous.pdf
void main(void)
{
    vec4 sum = vec4(0.0);
    vec2 step = vec2(1.0 / pushConstants.screenSize.x, 1.0 / pushConstants.screenSize.y);
    
    vec4 sampleColor = texture(inputColor, vScreenPos);
    vec4 sampleNormal = texture(inputNormal, vScreenPos);
    vec4 samplePos = texture(inputPos, vScreenPos);
    
    float totalWeight = 0.0;
    for (int i = 0; i < 25; i++)
    {
        vec2 uv = vScreenPos + offset[i] * step * params.stepWidth;
        
        vec4 offsetColor = texture(inputColor, uv);
        vec4 t = sampleColor - offsetColor;
        float dist2 = dot(t, t);
        float colorWeight = min(exp(-(dist2) / params.phiColor), 1.0);
        
        vec4 offsetNormal = texture(inputNormal, uv);
        t = sampleNormal - offsetNormal;
        dist2 = max(dot(t, t) / (params.stepWidth * params.stepWidth), 0.0);
        float normalWeight = min(exp(-(dist2) / params.phiNormal), 1.0);
        
        vec4 offsetPos = texture(inputPos, uv);
        t = samplePos - offsetPos;
        dist2 = dot(t, t);
        float posWeight = min(exp(-(dist2) / params.phiPos), 1.0);
        
        float weight = colorWeight * normalWeight * posWeight;
        sum += offsetColor * weight * kernel[i];
        totalWeight += weight * kernel[i];
    }
    
    outColor = sum / totalWeight;
}
