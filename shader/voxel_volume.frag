#version 450

layout (location = 0) in vec2 vScreenPos;

layout (location = 0) out vec4 outColor;
layout (location = 1) out float outDepth;
layout (location = 2) out vec2 outMotion;
layout (location = 3) out float outMask;
layout (location = 4) out vec3 outPos;
layout (location = 5) out vec3 outNormal;

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

struct Material
{
    vec4 diffuse;
    float metallic;
    float pad1;
    float pad2;
    float pad3;
};

struct RayHit
{
    bool hit;
    uint material;
    vec3 pos;
    vec3 normal;
};

layout (set = 0, binding = 0) uniform usampler3D scene;
layout (set = 0, binding = 1) uniform Palette {
    Material materials[256];
};
layout (set = 0, binding = 2) uniform sampler2D blueNoise;
layout (set = 0, binding = 3) uniform sampler2D oldPos;
layout (set = 0, binding = 4) uniform Parameters {
    uint aoSamples;
};

const uint MAX_RAY_STEPS = 512;
const uvec2 NOISE_SIZE = uvec2(512, 512);

// Scene definition from volume
uint getVoxel(ivec3 pos)
{
    if (pos.x > pushConstants.volumeBounds.x || pos.y > pushConstants.volumeBounds.y || pos.z > pushConstants.volumeBounds.z || pos.x < 0 || pos.y < 0 || pos.z < 0)
        return 0;
    uint val = texture(scene, vec3(pos) / vec3(pushConstants.volumeBounds)).r;
    return val;
}

// Generates a random number unique to this fragment from
vec3 fragmentNoiseSeq(uint num)
{
    uint offset = num * 32 + pushConstants.frame % 32;
    // Constants inspired by http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
    const float g = 1.22074408460575947536;
    const vec3 a = vec3(1.0 / g, 1.0 / (g * g), 1.0 / (g * g * g));
    vec2 p = gl_FragCoord.xy / NOISE_SIZE + vec2(0.5, 0.5);
    vec3 noise = texture(blueNoise, p).rgb;
    return mod(noise + offset * a, 1.0);
}

// Generates a random direction within the unit sphere
vec3 randomDir(uint num)
{
    return normalize(fragmentNoiseSeq(num) * 2.0 - vec3(1.0));
}

// Sky color interpolated from white to light blue
vec4 skyColor(vec3 rayDir)
{
    float t = 0.5 * (rayDir.y + 1.0);
    return (1.0 - t) * vec4(1.0, 1.0, 1.0, 1.0) + t * vec4(0.5, 0.7, 1.0, 1.0);
}

RayHit traceRay(vec3 start, vec3 dir, uint maxSteps)
{
    // Ray starting position
    // TODO initial ray positon from box intersection
    vec3 pos = start;

    // First voxel to sample
    ivec3 mapPos = ivec3(floor(pos));

    // Portion of ray needed for ray to traverse a voxel in each direction
    vec3 deltaDist = abs(1.0 / dir);

    // Integer position steps along ray
    ivec3 rayStep = ivec3(sign(dir));

    // Distance ray can travel in each direction before crossing a boundary
    vec3 sideDist = (sign(dir) * (vec3(mapPos) - pos) + (sign(dir) * 0.5) + 0.5) * deltaDist;

    RayHit result;
    result.hit = false;
    bvec3 mask;
    for (uint i = 0; i < maxSteps; i++)
    {
        // If we hit a voxel, break
        // TODO terminate early if out of bounds
        result.material = getVoxel(mapPos);
        if (result.material != 0)
        {
            result.hit = true;
            break;
        }

        // Determine which direction has minimum travel distance before hitting a boundary
        mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));

        // Advance the distance needed for that axis
        sideDist += vec3(mask) * deltaDist;

        // Advance the integer map positon
        mapPos += ivec3(vec3(mask)) * rayStep;
    }

    // Calculate normal direction from final mask
    result.normal = normalize(vec3(mask) * -vec3(rayStep));

    // Calculate the ending position from distance traveled
    float d = length(vec3(mask) * (sideDist - deltaDist));
    result.pos = pos + d * dir;

    return result;
}

void main()
{
    // Last frame's position
    vec3 lastPos = texture(oldPos, vScreenPos).xyz;

    // Screen position from -1.0 to 1.0
    vec2 screenPos = vScreenPos * 2.0 - 1.0;

    // Camera planes
    vec3 cameraPlaneU = pushConstants.camRight.xyz;
    vec3 cameraPlaneV = pushConstants.camUp.xyz * pushConstants.screenSize.y / pushConstants.screenSize.x;

    // Ray direction
    vec3 rayDir = normalize(normalize(pushConstants.camDir.xyz) + screenPos.x * cameraPlaneU + screenPos.y * cameraPlaneV + vec3(pushConstants.cameraJitter / pushConstants.screenSize * vec2(-2.0, 2.0), 0));

    // Ray starting position
    vec3 rayStart = pushConstants.camPos.xyz;

    // Trace the ray
    RayHit result = traceRay(rayStart, rayDir, MAX_RAY_STEPS);

    if (result.hit)
    {
        // Get diffuse color
        vec4 diffuseColor = materials[result.material].diffuse;

        // Ambient color (to be calculated)
        vec4 ambientColor = vec4(0.0);

        // For each ambient occulsion sample
        float sampleFrac = 1.0f / aoSamples;
        for (uint i = 0; i < aoSamples; i++)
        {
            // Generate a random direction around the normal
            vec3 dir = result.normal + randomDir(i);
            // Trace ray
            RayHit hit = traceRay(result.pos + dir * 0.01, dir, 64);
            // Add ambient color if hit
            if (!hit.hit)
                ambientColor += sampleFrac * skyColor(result.normal);
        }

        float metallic = materials[result.material].metallic;
        vec4 reflectColor = vec4(0);
        if (metallic > 0)
        {
            vec3 dir = reflect(rayDir, result.normal);
            RayHit reflectHit = traceRay(result.pos + result.normal * 0.01, dir, MAX_RAY_STEPS);
            if (reflectHit.hit)
            {
                reflectColor = materials[reflectHit.material].diffuse;
            }
            else
            {
                reflectColor = skyColor(result.normal);
            }
        }

        outColor.rgb = (reflectColor * metallic + diffuseColor * ambientColor * (1.0 - metallic)).rgb;

        outDepth = length(result.pos - pushConstants.camPos.xyz);
        outMask = 0.9;
        // TODO use inverse of camera matrix to reproject old position and calculate motion vectors
        outMotion = vec2(0);
        outPos = result.pos;
        outNormal = result.normal;
    }
    else
    {
        outColor.rgb = skyColor(rayDir).rgb;
        outDepth = 0.0;
        outMask = 0.0;
        outMotion = vec2(0);
        outPos = result.pos;
        outNormal = vec3(0, 0, 0);
    }
}