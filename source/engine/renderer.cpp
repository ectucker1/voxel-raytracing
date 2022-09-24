#include "renderer.hpp"

#include "engine/engine.hpp"

ARenderer::ARenderer(const std::shared_ptr<Engine>& engine) : AResource(engine)
{
    initWindowRenderPass();
    initWindowFramebuffers();
}

void ARenderer::initWindowRenderPass()
{
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = engine->swapchain.imageFormat;
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

    _windowRenderPass = engine->device.createRenderPass(renderPassInfo);
    engine->deletionQueue.push_deletor(deletorGroup, [&]() {
        engine->device.destroyRenderPass(_windowRenderPass);
    });
}

void ARenderer::initWindowFramebuffers()
{
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = _windowRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = engine->windowSize.x;
    framebufferInfo.height = engine->windowSize.y;
    framebufferInfo.layers = 1;

    size_t imageCount = engine->swapchain.size();
    _windowFramebuffers = ResourceRing<vk::Framebuffer>::fromFunc(imageCount, [&](size_t i) {
        framebufferInfo.pAttachments = &engine->swapchain.imageViews[i];
        return engine->device.createFramebuffer(framebufferInfo);
    });

    uint32_t framebufferGroup = engine->deletionQueue.push_group([&]() {
        _windowFramebuffers.destroy([&](const vk::Framebuffer& framebuffer) {
            engine->device.destroy(framebuffer);
        });
    });

    engine->resizeListeners.push([=]() { engine->deletionQueue.destroy_group(framebufferGroup); },
                                 [=]() { initWindowFramebuffers(); });
}
