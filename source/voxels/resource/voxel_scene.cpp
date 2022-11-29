#include "voxel_scene.hpp"

#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.h"
#include "material.hpp"
#include "engine/resource/texture_2d.hpp"
#include <fstream>

static glm::ivec3 calc_vox_pivot(const ogt_vox_model* model)
{
    return {
        (int)glm::floor(model->size_x / 2.0f),
        (int)glm::floor(model->size_y / 2.0f),
        (int)glm::floor(model->size_z / 2.0f)
    };
}

static glm::ivec3 apply_transform(const glm::mat4& transform, const glm::ivec3& pos, const glm::ivec3& pivot = glm::ivec3(0))
{
    return glm::floor(transform * (glm::vec4((float)pos.x + 0.5f, (float)pos.y + 0.5f, (float)pos.z + 0.5f, 1.0f) - glm::vec4(pivot, 0.0f)));
}

static glm::ivec3 apply_vox_transform(const ogt_vox_transform& ogtTransform, const glm::ivec3& pivot, const glm::ivec3& pos)
{
    const glm::vec4 ogtCol0(ogtTransform.m00, ogtTransform.m01, ogtTransform.m02, ogtTransform.m03);
    const glm::vec4 ogtCol1(ogtTransform.m10, ogtTransform.m11, ogtTransform.m12, ogtTransform.m13);
    const glm::vec4 ogtCol2(ogtTransform.m20, ogtTransform.m21, ogtTransform.m22, ogtTransform.m23);
    const glm::vec4 ogtCol3(ogtTransform.m30, ogtTransform.m31, ogtTransform.m32, ogtTransform.m33);
    const glm::mat4 ogtMat = glm::mat4{ogtCol0, ogtCol1, ogtCol2, ogtCol3};
    return apply_transform(ogtMat, pos, pivot);
}

VoxelScene::VoxelScene(const std::shared_ptr<Engine>& engine, const std::string& filename, const std::string& skyboxFilename) : AResource(engine)
{
    // Read in .vox file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> fileBuffer(fileSize);
    if (!file.read((char*) &fileBuffer[0], fileSize))
    {
        throw std::runtime_error("Failed to read voxel scene");
    }
    const ogt_vox_scene* voxScene = ogt_vox_read_scene(fileBuffer.data(), static_cast<uint32_t>(fileBuffer.size()));
    if (voxScene == nullptr)
        throw std::runtime_error("Could not parse voxel scene");
    fileBuffer.clear();

    if (voxScene->num_instances < 1)
        throw std::runtime_error("Voxel scene does not contain an instance.");

    // Calculate size of all instances in scene combined
    glm::ivec3 min = glm::ivec3(100000);
    glm::ivec3 max = glm::ivec3(-100000);;
    for (size_t i = 0; i < voxScene->num_instances; i++)
    {
        const ogt_vox_instance* instance = &voxScene->instances[i];
        const ogt_vox_model* model = voxScene->models[instance->model_index];

        ogt_vox_transform xform = ogt_vox_sample_instance_transform(instance, 0, voxScene);
        glm::ivec3 pivot = calc_vox_pivot(model);
        glm::ivec3 corner1 = apply_vox_transform(xform, pivot, glm::ivec3(0));
        glm::ivec3 corner2 = apply_vox_transform(xform, pivot, glm::ivec3(model->size_x, model->size_y, model->size_z));

        min.x = glm::min(min.x, glm::min(corner1.x, corner2.x));
        min.y = glm::min(min.y, glm::min(corner1.y, corner2.y));
        min.z = glm::min(min.z, glm::min(corner1.z, corner2.z));
        max.x = glm::max(max.x, glm::max(corner1.x, corner2.x));
        max.y = glm::max(max.y, glm::max(corner1.y, corner2.y));
        max.z = glm::max(max.z, glm::max(corner1.z, corner2.z));
    }
    width = max.x - min.x;
    height = max.z - min.z;
    depth = max.y - min.y;

    // Allocate buffer
    size_t size = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(depth);
    std::vector<uint8_t> sceneData = std::vector<uint8_t>(size);

    // Copy scene data to our buffer
    for (size_t i = 0; i < voxScene->num_instances; i++)
    {
        const ogt_vox_instance* instance = &voxScene->instances[i];
        const ogt_vox_model* model = voxScene->models[instance->model_index];
        glm::ivec3 pivot = calc_vox_pivot(model);

        for (uint32_t x = 0; x < model->size_x; x++)
        {
            for (uint32_t y = 0; y < model->size_y; y++)
            {
                for (uint32_t z = 0; z < model->size_z; z++)
                {
                    size_t voxPos = x + (y * model->size_x) + (z * model->size_x * model->size_y);
                    uint8_t vox = model->voxel_data[voxPos];
                    if (vox != 0)
                    {
                        ogt_vox_transform xform = ogt_vox_sample_instance_transform(instance, 0, voxScene);
                        glm::ivec3 transformed = apply_vox_transform(xform, pivot, glm::ivec3(x, y, z)) - min;
                        size_t scenePos = transformed.x + (transformed.z * width) + (transformed.y * width * height);
                        sceneData[scenePos] = vox;
                    }
                }
            }
        }
    }

    // Copy scene palette to our buffer
    std::array<Material, 256> paletteMaterials = {};
    for (size_t m = 0; m < paletteMaterials.size(); m++)
    {
        const ogt_vox_rgba color = voxScene->palette.color[m];
        const float metallic = voxScene->materials.matl[m].metal;

        paletteMaterials[m].diffuse = glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        paletteMaterials[m].diffuse = glm::pow(paletteMaterials[m].diffuse, glm::vec4(2.2f));
        paletteMaterials[m].metallic = metallic;
    }

    ogt_vox_destroy_scene(voxScene);

    // Copy scene data and palette onto GPU
    sceneTexture = Texture3D(engine, sceneData.data(), width, height, depth, 1, vk::Format::eR8Uint);
    paletteBuffer = Buffer(engine, paletteMaterials.size() * sizeof(Material), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    paletteBuffer->copyData(paletteMaterials.data(), paletteMaterials.size() * sizeof(Material));

    // Copy light to buffer
    Light light = {};
    lightBuffer = Buffer(engine, sizeof(Light), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    lightBuffer->copyData(&light, sizeof(Light));

    // Load skybox texture
    skyboxTexture = std::make_unique<Texture2D>(engine, skyboxFilename, 4, vk::Format::eR32G32B32A32Sfloat);
}
