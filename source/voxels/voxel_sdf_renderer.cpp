#include "voxel_sdf_renderer.hpp"

#include "engine/engine.hpp"
#include "engine/commands/command_util.hpp"
#include <glm/gtx/transform.hpp>
#include "voxels/screen_quad_push.hpp"
#include "voxels/material.hpp"
#include "engine/resource/buffer.hpp"

VoxelSDFRenderer::VoxelSDFRenderer(const std::shared_ptr<Engine>& engine) : ARenderer(engine)
{
    camera = CameraController(glm::vec3(8, 8, -50), 90.0, 0.0f, 1 / glm::tan(glm::radians(55.0f / 2)));

    _renderColorTarget = ResourceRing<RenderImage>::fromArgs(engine->swapchain.size(), engine,
                                                             renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Unorm,
                                                             vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::ImageAspectFlagBits::eColor);

    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = _renderColorTarget[0].format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::eReadOnlyOptimal;
    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    _renderColorPass = engine->device.createRenderPass(renderPassInfo);
    engine->deletionQueue.push_deletor(deletorGroup, [&]() {
        engine->device.destroyRenderPass(_renderColorPass);
    });

    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = _renderColorPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = renderRes.x;
    framebufferInfo.height = renderRes.y;
    framebufferInfo.layers = 1;
    _renderColorFramebuffer = ResourceRing<vk::Framebuffer>::fromFunc(engine->swapchain.size(), [&](size_t i) {
        framebufferInfo.pAttachments = &_renderColorTarget[i].imageView;
        return engine->device.createFramebuffer(framebufferInfo);
    });
    engine->deletionQueue.push_deletor(deletorGroup, [&]() {
        _renderColorFramebuffer.destroy([&](const vk::Framebuffer& framebuffer) {
            engine->device.destroy(framebuffer);
        });
    });

    uint8_t sceneData[16 * 16 * 16 * 1];
    for (int i = 0; i < 16 * 16 * 16 * 1; i++)
    {
        bool present = rand() % 2;
        uint8_t material = rand() % 16 + 1;
        if (present)
            sceneData[i] = material;
        else
            sceneData[i] = 0;
    }
    _sceneTexture = std::make_shared<Texture3D>(engine, sceneData, 16, 16, 16, 1, vk::Format::eR8Uint);

    std::array<Material, 256> paletteMaterials = {};
    for (size_t m = 0; m < paletteMaterials.size(); m++)
    {
        paletteMaterials[m].diffuse = glm::vec4(float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, 1.0f);
    }
    _paletteBuffer = std::make_shared<Buffer>(engine, sizeof(paletteMaterials), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    _paletteBuffer->copyData(paletteMaterials.data(), 256 * sizeof(Material));

    _pipeline = std::make_unique<VoxelSDFPipeline>(engine, _renderColorPass, _sceneTexture, _paletteBuffer);
    _pipeline->init();
}

void VoxelSDFRenderer::update(float delta)
{
    _time += delta;
    camera.update(engine->window, delta);
}

void VoxelSDFRenderer::recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t flightFrame)
{
    // Create clear color for this frame
    vk::ClearValue clearValue;
    float flash = abs(sin(_time));
    clearValue.color = vk::ClearColorValue(std::array<float, 4> {0.0f, 0.0f, flash, 1.0f});

    // Start color renderpass
    vk::RenderPassBeginInfo renderpassInfo = {};
    renderpassInfo.renderPass = _renderColorPass;
    renderpassInfo.renderArea.offset = 0;
    renderpassInfo.renderArea.offset = 0;
    renderpassInfo.renderArea.extent = vk::Extent2D(renderRes.x, renderRes.y);
    renderpassInfo.framebuffer = _renderColorFramebuffer[flightFrame];
    renderpassInfo.clearValueCount = 1;
    renderpassInfo.pClearValues = &clearValue;
    commandBuffer.beginRenderPass(renderpassInfo, vk::SubpassContents::eInline);

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
    constants.volumeBounds = glm::uvec3(16, 16, 16);
    constants.camPos = glm::vec4(camera.position, 1);
    constants.camDir = glm::vec4(camera.direction, 0);
    constants.camUp = glm::vec4(camera.up, 0);
    constants.camRight = glm::vec4(camera.right, 0);
    constants.time = _time;
    commandBuffer.pushConstants(_pipeline->layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(ScreenQuadPush), &constants);

    // Bind descriptor sets
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->layout,
                                     0, 2, _pipeline->descriptorSets.data(),
                                     0, nullptr);

    commandBuffer.draw(6, 1, 0, 0);

    // End color renderpass
    commandBuffer.endRenderPass();

    // Copy color target into swapchain image
    cmdutil::blit(commandBuffer, _renderColorTarget[flightFrame].image, { 0, 0 }, { 1920, 1080 },
                  engine->swapchain.images[flightFrame], { 0, 0 }, engine->windowSize);

    // Return swapchain image to present layout
    cmdutil::imageMemoryBarrier(
            commandBuffer,
            engine->swapchain.images[flightFrame],
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::ImageAspectFlagBits::eColor);
}
