#include "voxel_sdf_renderer.hpp"

#include "engine/engine.hpp"
#include "engine/commands/command_util.hpp"
#include <glm/gtx/transform.hpp>
#include "voxels/screen_quad_push.hpp"

void VoxelSDFRenderer::init(const std::shared_ptr<Engine>& engine)
{
    ARenderer::init(engine);

    camera = CameraController(glm::vec3(8, 8, -50), 90.0, 0.0f, 1 / glm::tan(glm::radians(55.0f / 2)));

    _renderColorTarget = ResourceRing<RenderImage>(_engine->swapchain.imageViews.size());
    _renderColorTarget.createEmplace(_engine->swapchain.imageViews.size(), engine, renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Unorm,
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
    _renderColorPass = _engine->logicalDevice.createRenderPass(renderPassInfo);
    _engine->mainDeletionQueue.push_group([&]() {
        _engine->logicalDevice.destroyRenderPass(_renderColorPass);
    });

    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = _renderColorPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = renderRes.x;
    framebufferInfo.height = renderRes.y;
    framebufferInfo.layers = 1;
    _renderColorFramebuffer = ResourceRing<vk::Framebuffer>(_engine->swapchain.imageViews.size());
    _renderColorFramebuffer.create(_engine->swapchain.imageViews.size(), [&](size_t i) {
        framebufferInfo.pAttachments = &_renderColorTarget[i].imageView;
        return _engine->logicalDevice.createFramebuffer(framebufferInfo);
    });
    _engine->mainDeletionQueue.push_group([&]() {
        _renderColorFramebuffer.destroy([&](const vk::Framebuffer& framebuffer) {
            _engine->logicalDevice.destroy(framebuffer);
        });
    });

    _sceneTexture = std::make_shared<Texture3D>();
    uint8_t sceneData[16 * 16 * 16 * 1];
    for (int i = 0; i < 16 * 16 * 16 * 1; i++)
    {
        sceneData[i] = rand() % 2;
    }
    _sceneTexture->init(_engine, sceneData, 16, 16, 16, 1, vk::Format::eR8Uint);

    _pipeline = VoxelSDFPipeline();
    _pipeline.sceneData = _sceneTexture;
    _pipeline.init(_engine, _renderColorPass);
    _engine->mainDeletionQueue.push_group([&]() {
       _engine->logicalDevice.destroy(_pipeline.layout);
       _engine->logicalDevice.destroy(_pipeline.pipeline);
    });
}

void VoxelSDFRenderer::update(float delta)
{
    _time += delta;
    camera.update(_engine->window, delta);
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
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline.pipeline);

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
    commandBuffer.pushConstants(_pipeline.layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(ScreenQuadPush), &constants);

    // Bind texture descriptor set
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline.layout,
                                     0, 1, &_pipeline.sceneDataDescriptorSet,
                                     0, nullptr);

    commandBuffer.draw(6, 1, 0, 0);

    // End color renderpass
    commandBuffer.endRenderPass();

    // Copy color target into swapchain image
    cmdutil::blit(commandBuffer, _renderColorTarget[flightFrame].image, { 0, 0 }, { 1920, 1080 },
                  _engine->swapchain.images[flightFrame], { 0, 0 }, _engine->windowSize);

    // Return swapchain image to present layout
    cmdutil::imageMemoryBarrier(
            commandBuffer,
            _engine->swapchain.images[flightFrame],
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::ImageAspectFlagBits::eColor);
}
