#include "framebuffer.hpp"

#include "engine/engine.hpp"

FramebufferBuilder& FramebufferBuilder::color(vk::ImageView imageView)
{
    colorAttachments.push_back(imageView);

    return *this;
}

FramebufferBuilder& FramebufferBuilder::depthStencil(vk::ImageView imageView)
{
    depthStencilAttachment = imageView;

    return *this;
}

Framebuffer FramebufferBuilder::build()
{
    Framebuffer framebuffer(engine);
    framebuffer.size = size;

    std::vector<vk::ImageView> attachments = {};
    attachments.reserve(colorAttachments.size());
    attachments.insert(attachments.end(), colorAttachments.begin(), colorAttachments.end());
    if (depthStencilAttachment.has_value())
        attachments.push_back(depthStencilAttachment.value());

    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = pass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = size.x;
    framebufferInfo.height = size.y;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();

    vk::Framebuffer createdFramebuffer = engine->device.createFramebuffer(framebufferInfo);
    framebuffer.framebuffer = createdFramebuffer;
    framebuffer.pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.destroy(createdFramebuffer);
    });

    return framebuffer;
}