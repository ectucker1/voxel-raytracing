#version 450

layout (location = 0) in vec2 vScreenPos;

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform constants
{
    vec4 camPos;
    vec4 camDir;
    vec4 camRight;
    vec4 camUp;
    uvec3 volumeBounds;
    float time;
    ivec2 screenSize;
} pushConstants;

struct Material
{
    vec4 diffuse;
};

struct RayHit
{
    bool hit;
    uint material;
    vec3 pos;
    vec3 normal;
};

layout (set = 0, binding = 0) uniform usampler3D scene;
layout (set = 1, binding = 1) uniform Palette {
    Material materials[256];
};

const uint MAX_RAY_STEPS = 64;
const uint AO_SAMPLES = 2;

// Scene definition from volume
uint getVoxel(ivec3 pos)
{
    if (pos.x > pushConstants.volumeBounds.x || pos.y > pushConstants.volumeBounds.y || pos.z > pushConstants.volumeBounds.z || pos.x < 0 || pos.y < 0 || pos.z < 0)
        return 0;
    uint val = texture(scene, vec3(pos) / vec3(pushConstants.volumeBounds)).r;
    return val;
}

// Generates a random number unique to this fragment
// Based on https://stackoverflow.com/questions/5149544/can-i-generate-a-random-number-inside-a-pixel-shader
// TODO replace with blue noise
float fragmentRand(vec2 offset)
{
    vec2 p = gl_FragCoord.xy / vec2(1920, 1080) + offset;
    vec2 K1 = vec2(
        23.14069263277926,
        2.665144142690225
    );
    return fract(cos(dot(p,K1)) * 12345.6789);
}

// Generates a random direction within the unit sphere
vec3 randomDir(float sampleOffset)
{
    float retryOffet;
    vec3 dir;
    for (uint i = 0; i < 5; i++)
    {
        vec3 dir = vec3(
            fragmentRand(vec2(retryOffet, sampleOffset)),
            fragmentRand(vec2(retryOffet, sampleOffset + 0.01)),
            fragmentRand(vec2(retryOffet, sampleOffset + 0.02))
        );
        if (length(dir) <= 1)
        {
            return dir;
        }
        retryOffet += 0.01;
    }

    return vec3(0, 1, 0);
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
    // Screen position from -1.0 to 1.0
    vec2 screenPos = vScreenPos * 2.0 - 1.0;

    // Camera planes
    vec3 cameraPlaneU = pushConstants.camRight.xyz;
    vec3 cameraPlaneV = pushConstants.camUp.xyz * pushConstants.screenSize.y / pushConstants.screenSize.x;

    // Ray direction
    vec3 rayDir = normalize(normalize(pushConstants.camDir.xyz) + screenPos.x * cameraPlaneU + screenPos.y * cameraPlaneV);

    // Ray starting position
    vec3 rayStart = pushConstants.camPos.xyz;

    // Trace the ray
    RayHit result = traceRay(rayStart, rayDir, MAX_RAY_STEPS);

    if (result.hit)
    {
        // Get diffuse color
        vec3 diffuseColor = materials[result.material].diffuse.rgb;

        // Ambient color (to be calculated)
        vec4 ambientColor = vec4(0.0);

        // For each ambient occulsion sample
        float sampleOffset = 0.0;
        float sampleFrac = 1.0f / AO_SAMPLES;
        for (uint i = 0; i < AO_SAMPLES; i++)
        {
            // Generate a random direction around the normal
            vec3 dir = result.normal + randomDir(sampleOffset);
            // Trace ray
            RayHit hit = traceRay(result.pos + dir * 0.01, dir, MAX_RAY_STEPS);
            // Add ambient color if hit
            if (!hit.hit)
                ambientColor += sampleFrac * skyColor(result.normal);
            // Offset RNG for next sample
            sampleOffset += 0.1;
        }

        outColor.rgb = diffuseColor * ambientColor.rgb;
    }
    else
    {
        outColor.rgb = skyColor(rayDir).rgb;
    }
}