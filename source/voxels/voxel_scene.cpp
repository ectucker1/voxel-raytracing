#include "voxel_scene.hpp"

#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.h"
#include "voxels/material.hpp"
#include <fstream>

VoxelScene::VoxelScene(const std::shared_ptr<Engine>& engine, const std::string& filename) : AResource(engine)
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

    // Get the first model in the file
    // MagicaVoxel supports multiple models in a scene, but we're only doing 1
    if (voxScene->num_models < 1)
        throw std::runtime_error("Voxel scene does not contain model.");
    const ogt_vox_model* model = voxScene->models[0];

    // Copy scene data to our buffer
    width = model->size_x;
    height = model->size_z;
    depth = model->size_y;
    size_t size = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(depth);
    std::vector<uint8_t> sceneData = std::vector<uint8_t>(size);
    for (uint32_t x = 0; x < width; x++)
    {
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t z = 0; z < depth; z++)
            {
                size_t voxPos = x + (z * width) + (y * width * depth);
                size_t scenePos = x + (y * width) + (z * width * depth);
                sceneData[scenePos] = model->voxel_data[voxPos];
            }
        }
    }

    // Copy scene palette to our buffer
    std::array<Material, 256> paletteMaterials = {};
    for (size_t m = 0; m < paletteMaterials.size(); m++)
    {
        const ogt_vox_rgba color = voxScene->palette.color[m];
        paletteMaterials[m].diffuse = glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        paletteMaterials[m].diffuse = glm::pow(paletteMaterials[m].diffuse, glm::vec4(2.2f));
    }

    ogt_vox_destroy_scene(voxScene);

    // Copy scene data and palette onto GPU
    sceneTexture = Texture3D(engine, sceneData.data(), width, height, depth, 1, vk::Format::eR8Uint);
    paletteBuffer = Buffer(engine, paletteMaterials.size() * sizeof(Material), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    paletteBuffer->copyData(paletteMaterials.data(), paletteMaterials.size() * sizeof(Material));
}
