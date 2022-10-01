#include "voxel_sdf_renderer.hpp"

#include "engine/engine.hpp"
#include "engine/commands/command_util.hpp"
#include <glm/gtx/transform.hpp>
#include "voxels/screen_quad_push.hpp"
#include "voxels/material.hpp"
#include "engine/resource/buffer.hpp"
#include "engine/resource/texture_2d.hpp"

const glm::uvec3 AREA_SIZE = glm::uvec3(16, 16, 16);

VoxelSDFRenderer::VoxelSDFRenderer(const std::shared_ptr<Engine>& engine) : ARenderer(engine)
{
    camera = CameraController(glm::vec3(8, 8, -50), 90.0, 0.0f, 1 / glm::tan(glm::radians(55.0f / 2)));

    _renderColorTarget = RenderImage(engine, renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Unorm,
                                     vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::ImageAspectFlagBits::eColor);

    _renderColorPass = RenderPassBuilder(engine)
            .color(0, _renderColorTarget->format, glm::vec4(0.0))
            .build();

    _renderColorFramebuffer = FramebufferBuilder(engine, _renderColorPass->renderPass, renderRes)
            .color(_renderColorTarget->imageView)
            .build();

    size_t size = size_t(AREA_SIZE.x) * size_t(AREA_SIZE.y) * size_t(AREA_SIZE.z);
    std::vector<uint8_t> sceneData = std::vector<uint8_t>(size);
    for (size_t i = 0; i < size; i++)
    {
        bool present = rand() % 2;
        uint8_t material = rand() % 16 + 1;
        if (present)
            sceneData[i] = material;
        else
            sceneData[i] = 0;
    }
    _sceneTexture = Texture3D(engine, sceneData.data(), AREA_SIZE.x, AREA_SIZE.y, AREA_SIZE.z, 1, vk::Format::eR8Uint);

    _noiseTexture = Texture2D(engine, "../resource/blue_noise_rgba.png", 4, vk::Format::eR8G8B8A8Unorm);

    std::array<Material, 256> paletteMaterials = {};
    for (size_t m = 0; m < paletteMaterials.size(); m++)
    {
        paletteMaterials[m].diffuse = glm::vec4(float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, 1.0f);
    }
    _paletteBuffer = Buffer(engine, sizeof(paletteMaterials), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    _paletteBuffer->copyData(paletteMaterials.data(), 256 * sizeof(Material));

    _pipeline = VoxelSDFPipeline(VoxelSDFPipeline::build(engine, _renderColorPass->renderPass));
    _pipeline->descriptorSet->initImage(0, _sceneTexture->imageView, _sceneTexture->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    _pipeline->descriptorSet->initBuffer(1, _paletteBuffer->buffer, _paletteBuffer->size, vk::DescriptorType::eUniformBuffer);
    _pipeline->descriptorSet->initImage(2, _noiseTexture->imageView, _noiseTexture->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void VoxelSDFRenderer::update(float delta)
{
    _time += delta;
    camera.update(engine->window, delta);
}

void VoxelSDFRenderer::recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame)
{
    // Start color renderpass
    _renderColorPass->recordBegin(commandBuffer, _renderColorFramebuffer.value());

    // Bind pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline->pipeline);

    // Set viewport
    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(renderRes.x);
    viewport.height = static_cast<float>(renderRes.y);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &viewport);

    // Set scissor
    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = vk::Extent2D(renderRes.x, renderRes.y);
    commandBuffer.setScissor(0, 1, &scissor);

    // Set push constants
    ScreenQuadPush constants;
    constants.screenSize = glm::ivec2(renderRes.x, renderRes.y);
    constants.volumeBounds = AREA_SIZE;
    constants.camPos = glm::vec4(camera.position, 1);
    constants.camDir = glm::vec4(camera.direction, 0);
    constants.camUp = glm::vec4(camera.up, 0);
    constants.camRight = glm::vec4(camera.right, 0);
    constants.time = _time;
    commandBuffer.pushConstants(_pipeline->layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(ScreenQuadPush), &constants);

    // Bind descriptor sets
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->layout,
                                     0, 1,
                                     _pipeline->descriptorSet->getSet(flightFrame),
                                     0, nullptr);

    commandBuffer.draw(6, 1, 0, 0);

    // End color renderpass
    commandBuffer.endRenderPass();

    // Copy color target into swapchain image
    cmdutil::blit(commandBuffer, _renderColorTarget->image, { 0, 0 }, { renderRes.x, renderRes.y },
                  engine->swapchain.images[swapchainImage], { 0, 0 }, engine->windowSize);

    // Return swapchain image to present layout
    cmdutil::imageMemoryBarrier(
            commandBuffer,
            engine->swapchain.images[swapchainImage],
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::ImageAspectFlagBits::eColor);
}
