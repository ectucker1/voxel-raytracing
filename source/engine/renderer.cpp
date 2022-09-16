#include "renderer.hpp"

#include "engine/engine.hpp"

void ARenderer::init(const std::shared_ptr<Engine>& engine)
{
    _engine = engine;
    _initialized = true;

    initWindowRenderPass();
    initWindowFramebuffers();
}

void ARenderer::initWindowRenderPass()
{
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = _engine->swapchain.imageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

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

    _windowRenderPass = _engine->logicalDevice.createRenderPass(renderPassInfo);
    _engine->mainDeletionQueue.push_group([&]() {
        _engine->logicalDevice.destroyRenderPass(_windowRenderPass);
    });
}

void ARenderer::initWindowFramebuffers()
{
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = _windowRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = _engine->windowSize.x;
    framebufferInfo.height = _engine->windowSize.y;
    framebufferInfo.layers = 1;

    size_t imageCount = _engine->swapchain.imageViews.size();
    _windowFramebuffers = ResourceRing<vk::Framebuffer>(imageCount);
    _windowFramebuffers.create(imageCount, [&](size_t i) {
        framebufferInfo.pAttachments = &_engine->swapchain.imageViews[i];
        return _engine->logicalDevice.createFramebuffer(framebufferInfo);
    });

    uint32_t framebufferGroup = _engine->mainDeletionQueue.push_group([&]() {
        _windowFramebuffers.destroy([&](const vk::Framebuffer& framebuffer) {
            _engine->logicalDevice.destroy(framebuffer);
        });
    });

    _engine->resizeListeners.push([=]() { _engine->mainDeletionQueue.destroy_group(framebufferGroup); },
                                  [=]() { initWindowFramebuffers(); });
}
