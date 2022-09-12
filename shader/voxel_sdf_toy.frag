#version 450

layout (location = 0) in vec2 screenUV;

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform constants
{
    ivec2 screenSize;
} pushConstants;

const int MAX_RAY_STEPS = 64;

// Signed distance field of a sphere
float sdSphere(vec3 p, float d)
{
    return length(p) - d;
}

// Scene definition as union of SDFs
bool getVoxel(ivec3 coord)
{
    vec3 pos = vec3(coord);
    float dist = min(sdSphere(pos + vec3(5.0, 2.0, 0.0), 5.0), sdSphere(pos + vec3(-5.0, -7.0, 0.0), 5.0));
    return dist < 0.0;
}

void main()
{
    // Screen position from -1.0 to 1.0
    vec2 screenPos = screenUV * 2.0 - 1.0;

    // Camera direction and focal length
    vec3 cameraDir = vec3(0.0, 0.0, 1.0);

    // Camera planes
    vec3 cameraPlaneU = vec3(1.0, 0.0, 0.0);
    vec3 cameraPlaneV = vec3(0.0, 1.0, 0.0) * pushConstants.screenSize.y / pushConstants.screenSize.x;

    // Ray direction
    vec3 rayDir = cameraDir + screenPos.x * cameraPlaneU + screenPos.y * cameraPlaneV;
    // Ray starting position
    vec3 rayPos = vec3(0.0, 0.0, -20.0);

    // First voxel to sample
    ivec3 mapPos = ivec3(floor(rayPos));

    // Portion of ray needed for ray to traverse a voxel in each direction
    vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);

    // Integer position steps along ray
    ivec3 rayStep = ivec3(sign(rayDir));

    // Distance ray can travel in each direction before crossing a boundary
    vec3 sideDist = (vec3(mapPos) - rayPos) / rayDir;

    bool hit = false;
    bvec3 mask;

    for (int i = 0; i < MAX_RAY_STEPS; i++)
    {
        // If we hit a voxel, break
        if (getVoxel(mapPos))
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
        outColor.rgb = vec3(mask);
    }
    else
    {
        outColor.rgb = vec3(0.0);
    }
}