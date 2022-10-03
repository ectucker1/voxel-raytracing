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

    gColorTarget = RenderImage(engine, renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Unorm,
                                     vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
    gDepthTarget = RenderImage(engine, renderRes.x, renderRes.y, vk::Format::eR32Sfloat,
                               vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
    gPass = RenderPassBuilder(engine)
            .color(0, gColorTarget->format, glm::vec4(0.0))
            .color(1, gDepthTarget->format, glm::vec4(0.0))
            .build();
    gFramebuffer = FramebufferBuilder(engine, gPass->renderPass, renderRes)
            .color(gColorTarget->imageView)
            .color(gDepthTarget->imageView)
            .build();

    denoiseColorTarget = RenderImage(engine, renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Unorm,
                               vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
    denoisePass = RenderPassBuilder(engine)
            .color(0, denoiseColorTarget->format, glm::vec4(0.0))
            .build();
    denoiseFramebuffer = FramebufferBuilder(engine, denoisePass->renderPass, renderRes)
            .color(denoiseColorTarget->imageView)
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

    _blitOffsetsBuffer = Buffer(engine, sizeof(BlitOffsets), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    _blitOffsetsBuffer->copyData(&blitOffsets, sizeof(BlitOffsets));

    geometryPipeline = VoxelSDFPipeline(VoxelSDFPipeline::build(engine, gPass->renderPass));
    geometryPipeline->descriptorSet->initImage(0, _sceneTexture->imageView, _sceneTexture->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    geometryPipeline->descriptorSet->initBuffer(1, _paletteBuffer->buffer, _paletteBuffer->size, vk::DescriptorType::eUniformBuffer);
    geometryPipeline->descriptorSet->initImage(2, _noiseTexture->imageView, _noiseTexture->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);

    denoisePipeline = DenoiserPipeline(DenoiserPipeline::build(engine, denoisePass->renderPass));
    denoisePipeline->descriptorSet->initImage(0, gColorTarget->imageView, gColorTarget->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);

    blitPipeline = BlitPipeline(BlitPipeline::build(engine, _windowRenderPass->renderPass));
    blitPipeline->descriptorSet->initImage(0, denoiseColorTarget->imageView, denoiseColorTarget->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    blitPipeline->descriptorSet->initBuffer(1, _blitOffsetsBuffer->buffer, _blitOffsetsBuffer->size, vk::DescriptorType::eUniformBuffer);
}

void VoxelSDFRenderer::update(float delta)
{
    _time += delta;
    camera.update(engine->window, delta);
}

void VoxelSDFRenderer::recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame)
{
    // Set viewport
    vk::Viewport renderResViewport;
    renderResViewport.x = 0.0f;
    renderResViewport.y = 0.0f;
    renderResViewport.width = static_cast<float>(renderRes.x);
    renderResViewport.height = static_cast<float>(renderRes.y);
    renderResViewport.minDepth = 0.0f;
    renderResViewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &renderResViewport);

    // Set scissor
    vk::Rect2D renderResScissor;
    renderResScissor.offset = vk::Offset2D(0, 0);
    renderResScissor.extent = vk::Extent2D(renderRes.x, renderRes.y);
    commandBuffer.setScissor(0, 1, &renderResScissor);

    // Start color renderpass
    gPass->recordBegin(commandBuffer, gFramebuffer.value());
    // Bind pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, geometryPipeline->pipeline);
    // Set push constants
    ScreenQuadPush constants;
    constants.screenSize = glm::ivec2(renderRes.x, renderRes.y);
    constants.volumeBounds = AREA_SIZE;
    constants.camPos = glm::vec4(camera.position, 1);
    constants.camDir = glm::vec4(camera.direction, 0);
    constants.camUp = glm::vec4(camera.up, 0);
    constants.camRight = glm::vec4(camera.right, 0);
    constants.time = _time;
    commandBuffer.pushConstants(geometryPipeline->layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(ScreenQuadPush), &constants);
    commandBuffer.setViewport(0, 1, &renderResViewport);
    // Set scissor
    commandBuffer.setScissor(0, 1, &renderResScissor);
    // Bind descriptor sets
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, geometryPipeline->layout,
                                     0, 1,
                                     geometryPipeline->descriptorSet->getSet(flightFrame),
                                     0, nullptr);
    commandBuffer.draw(3, 1, 0, 0);
    // End color renderpass
    commandBuffer.endRenderPass();

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            gColorTarget->image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);

    // Start denoise renderpass
    denoisePass->recordBegin(commandBuffer, denoiseFramebuffer.value());
    // Bind pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, denoisePipeline->pipeline);
    // Set viewport
    commandBuffer.setViewport(0, 1, &renderResViewport);
    // Set scissor
    commandBuffer.setScissor(0, 1, &renderResScissor);
    // Bind descriptor sets
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, denoisePipeline->layout,
                                     0, 1,
                                     denoisePipeline->descriptorSet->getSet(flightFrame),
                                     0, nullptr);
    commandBuffer.draw(3, 1, 0, 0);
    // End denoise renderpass
    commandBuffer.endRenderPass();

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            denoiseColorTarget->image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);

    // Set viewport
    vk::Viewport presentResViewport;
    presentResViewport.x = 0.0f;
    presentResViewport.y = 0.0f;
    presentResViewport.width = static_cast<float>(engine->windowSize.x);
    presentResViewport.height = static_cast<float>(engine->windowSize.y);
    presentResViewport.minDepth = 0.0f;
    presentResViewport.maxDepth = 1.0f;

    // Set scissor
    vk::Rect2D presentResScissor;
    presentResScissor.offset = vk::Offset2D(0, 0);
    presentResScissor.extent = vk::Extent2D(engine->windowSize.x, engine->windowSize.y);

    // Copy color target into swapchain image
    _windowRenderPass->recordBegin(commandBuffer, _windowFramebuffers[swapchainImage]);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, blitPipeline->pipeline);
    commandBuffer.setViewport(0, 1, &presentResViewport);
    commandBuffer.setScissor(0, 1, &presentResScissor);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, blitPipeline->layout,
                                     0, 1,
                                     blitPipeline->descriptorSet->getSet(flightFrame),
                                     0, nullptr);
    commandBuffer.draw(3, 1, 0, 0);
    commandBuffer.endRenderPass();

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            engine->swapchain.images[swapchainImage],
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eNone,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::ImageAspectFlagBits::eColor);
}
