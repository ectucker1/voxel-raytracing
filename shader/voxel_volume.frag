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

struct RayHitInternal
{
    vec3 pos;
    vec3 sideDist;
    vec3 deltaDist;
    ivec3 rayStep;
    uint material;
    bvec3 mask;
};

struct RayHit
{
    uint material;
    vec3 pos;
    vec3 normal;
    vec3 dir;
};

layout (set = 0, binding = 0) uniform usampler3D scene;
layout (set = 0, binding = 1) uniform Palette {
    Material materials[256];
};
layout (set = 0, binding = 2) uniform sampler2D blueNoise;
layout (set = 0, binding = 3) uniform sampler2D oldPos;
layout (set = 0, binding = 4) uniform Parameters {
    uint aoSamples;
    float ambientIntensity;
};
layout (set = 0, binding = 5) uniform Light {
    vec3 lightDir;
    float lightIntensity;
    vec4 lightColor;
};
layout (set = 0, binding = 6) uniform sampler2D skybox;

const uint MAX_RAY_STEPS = 512;
const uint MAX_REFLECTIONS = 5;
const uvec2 NOISE_SIZE = uvec2(512, 512);

// Scene definition from volume
uint getVoxel(ivec3 pos)
{
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

// Sky color sampled from skybox
const vec2 invAtan = vec2(0.1591, 0.3183);
vec4 skyColor(vec3 rayDir)
{
    vec2 uv = vec2(atan(rayDir.z, rayDir.x), asin(-rayDir.y));
    uv *= invAtan;
    uv += 0.5;
    return texture(skybox, uv);
}

// Calculate the point where a ray intersects the scene box
// Design inspired by https://tavianator.com/2011/ray_box.html
vec3 boxIntersection(vec3 start, vec3 dir) {
    vec3 invDir = 1.0 / dir;

    vec3 t1 = (-start) * invDir;
    vec3 t2 = (vec3(pushConstants.volumeBounds) - start) * invDir;
    vec3 tminDir = min(t1, t2);
    vec3 tmaxDir = max(t1, t2);

    float tmin = max(tminDir.x, max(tminDir.y, tminDir.z));
    float tmax = min(tmaxDir.x, min(tmaxDir.y, tmaxDir.z));

    if (tmin >= 0 && tmax >= tmin) {
        return start + (tmin + 0.1) * dir;
    } else {
        return start;
    }
}

RayHitInternal traceRayInt(vec3 start, vec3 dir, uint maxSteps)
{
    RayHitInternal result;

    // Ray starting position
    result.pos = boxIntersection(start, dir);

    // First voxel to sample
    ivec3 mapPos = ivec3(floor(result.pos));

    // Portion of ray needed for ray to traverse a voxel in each direction
    result.deltaDist = abs(1.0 / dir);

    // Integer position steps along ray
    result.rayStep = ivec3(sign(dir));

    // Distance ray can travel in each direction before crossing a boundary
    result.sideDist = (sign(dir) * (vec3(mapPos) - result.pos) + (sign(dir) * 0.5) + 0.5) * result.deltaDist;

    for (uint i = 0; i < maxSteps; i++)
    {
        // If we're out of bounds, break
        if (mapPos.x < 0 || mapPos.x >= pushConstants.volumeBounds.x
        || mapPos.y < 0 || mapPos.y >= pushConstants.volumeBounds.y
        || mapPos.z < 0 || mapPos.z >= pushConstants.volumeBounds.z)
        {
            break;
        }

        // If we hit a voxel, break
        result.material = getVoxel(mapPos);
        if (result.material != 0)
        {
            break;
        }

        // Determine which direction has minimum travel distance before hitting a boundary
        result.mask = lessThanEqual(result.sideDist.xyz, min(result.sideDist.yzx, result.sideDist.zxy));

        // Advance the distance needed for that axis
        result.sideDist += vec3(result.mask) * result.deltaDist;

        // Advance the integer map positon
        mapPos += ivec3(vec3(result.mask)) * result.rayStep;
    }

    return result;
}

RayHit traceRay(vec3 start, vec3 dir, uint maxSteps)
{
    // Internal trace, common between this and simplified versions
    RayHitInternal interal = traceRayInt(start, dir, maxSteps);

    RayHit result;
    result.material = interal.material;
    result.dir = dir;

    if (result.material != 0)
    {
        // Calculate normal direction from final mask
        result.normal = normalize(vec3(interal.mask) * -vec3(interal.rayStep));

        // Calculate the ending position from distance traveled
        float d = length(vec3(interal.mask) * (interal.sideDist - interal.deltaDist));
        result.pos = interal.pos + d * dir;
    }

    return result;
}

bool traceRayHit(vec3 start, vec3 dir, uint maxSteps)
{
    RayHitInternal interal = traceRayInt(start, dir, maxSteps);
    return interal.material != 0;
}

// Take ambient occlusion samples and calculate a total ambient occlusion factor
vec3 calcAmbient(RayHit hit, uint depth)
{
    float ambient = 0.0;

    if (aoSamples == 0) {
        ambient = 1.0;
    } else {
        // For each ambient occulsion sample
        float sampleFrac = 1.0f / aoSamples;
        for (uint i = 0; i < aoSamples; i++)
        {
            // Generate a random direction around the normal
            vec3 dir = hit.normal + randomDir(i + depth * aoSamples);
            // Trace ray
            bool hit = traceRayHit(hit.pos + dir * 0.01, dir, 64);
            // Add ambient color if hit
            if (hit)
            ambient += sampleFrac;
        }
    }

    return ambient * ambientIntensity * skyColor(hit.normal).rgb;
}

// Cast a ray and determine if the given hit is in the shadows
bool isShadowed(RayHit hit)
{
    return traceRayHit(hit.pos + hit.normal * 0.01, lightDir, MAX_RAY_STEPS);
}

// Calculate final material color using blending parameters
vec3 color(vec3 normal, Material mat, vec3 ambient, vec3 reflection, bool shadowed)
{
    vec3 diffuse = vec3(0.0);
    if (!shadowed)
    {
        float diff = max(dot(normal, lightDir), 0.0);
        diffuse = diff * lightColor.rgb * lightIntensity;
    }

    vec3 specular = reflection * mat.metallic;

    return (diffuse + specular + ambient) * mat.diffuse.rgb;
}


// Color a ray hit using the previously calculated reflection color
vec3 colorHit(RayHit hit, vec3 reflection, uint depth)
{
    if (hit.material != 0)
    {
        vec3 ambient = calcAmbient(hit, depth);
        bool shadowed = isShadowed(hit);
        return color(hit.normal, materials[hit.material], ambient, reflection, shadowed) * 1.0 / float(depth + 1);
    }
    else
    {
        return skyColor(hit.dir).rgb;
    }
}

// Color the main ray, including the reflection stack
vec3 colorMainRay(RayHit hit)
{
    // Get hit material
    Material mat = materials[hit.material];

    // Calculate a sum of reflection lighting
    vec3 reflection = vec3(0.0);
    if (mat.metallic > 0)
    {
        // Stack of reflection bounces
        RayHit bounces[MAX_REFLECTIONS];

        // Continue taking reflection bounces until we reach our maximum
        RayHit lastHit = hit;
        int lastIdx = -1;
        for (int i = 0; i < MAX_REFLECTIONS; i++)
        {
            vec3 reflectDir = reflect(lastHit.dir, lastHit.normal);
            RayHit reflectHit = traceRay(lastHit.pos + lastHit.normal * 0.01, reflectDir, MAX_RAY_STEPS);

            // Store reflection bounce in stack
            bounces[i] = reflectHit;
            lastHit = reflectHit;

            // Terminate early if material isn't metallic or we hit sky
            if (lastHit.material == 0 || materials[lastHit.material].metallic <= 0)
            {
                lastIdx = i;
                break;
            }
        }

        // Sum backwards up the stack
        for (int i = lastIdx; i >= 0; i--)
        {
            reflection += colorHit(bounces[i], reflection, i);
        }
    }

    return colorHit(hit, reflection, 0);
}

void main()
{
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

    if (result.material != 0)
    {
        outColor.rgb = colorMainRay(result).rgb;
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