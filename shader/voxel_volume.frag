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

layout (set = 0, binding = 0) uniform usampler3D scene;
layout (set = 1, binding = 1) uniform Palette {
    Material materials[256];
};

const int MAX_RAY_STEPS = 64;

// Scene definition from volume
uint getVoxel(ivec3 pos)
{
    if (pos.x > pushConstants.volumeBounds.x || pos.y > pushConstants.volumeBounds.y || pos.z > pushConstants.volumeBounds.z || pos.x < 0 || pos.y < 0 || pos.z < 0)
        return 0;
    uint val = texture(scene, vec3(pos) / vec3(pushConstants.volumeBounds)).r;
    return val;
}

void main()
{
    vec3 lightPos = vec3(20.0, 20.0, -20.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    // Screen position from -1.0 to 1.0
    vec2 screenPos = vScreenPos * 2.0 - 1.0;

    // Camera planes
    vec3 cameraPlaneU = pushConstants.camRight.xyz;
    vec3 cameraPlaneV = pushConstants.camUp.xyz * pushConstants.screenSize.y / pushConstants.screenSize.x;

    // Ray direction
    vec3 rayDir = normalize(normalize(pushConstants.camDir.xyz) + screenPos.x * cameraPlaneU + screenPos.y * cameraPlaneV);

    // TODO ray-box intersection to get start pos

    // Ray starting position
    vec3 rayPos = pushConstants.camPos.xyz;

    // First voxel to sample
    ivec3 mapPos = ivec3(floor(rayPos));

    // Portion of ray needed for ray to traverse a voxel in each direction
    vec3 deltaDist = abs(1.0 / rayDir);

    // Integer position steps along ray
    ivec3 rayStep = ivec3(sign(rayDir));

    // Distance ray can travel in each direction before crossing a boundary
    vec3 sideDist = (sign(rayDir) * (vec3(mapPos) - rayPos) + (sign(rayDir) * 0.5) + 0.5) * deltaDist;

    bool hit = false;
    uint mat = 0;
    bvec3 mask;

    for (int i = 0; i < MAX_RAY_STEPS; i++)
    {
        // If we hit a voxel, break
        mat = getVoxel(mapPos);
        if (mat != 0)
        {
            hit = true;
            break;
        }

        // Determine which direction has minimum travel distance before hitting a boundary
        mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));

        // Advance the distance needed for that axis
        sideDist += vec3(mask) * deltaDist;

        // Advance the integer map positon
        mapPos += ivec3(vec3(mask)) * rayStep;
    }

    if (hit)
    {
        float d = length(vec3(mask) * (sideDist - deltaDist));
        vec3 pos = rayPos + d * rayDir;
        vec3 normal = normalize(vec3(mask) * -vec3(rayStep));
        vec3 lightDir = normalize(lightPos - pos);

        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * materials[mat].diffuse.rgb;
        outColor.rgb = diffuse + vec3(0.1);
    }
    else
    {
        outColor.rgb = abs(rayDir);
    }
}