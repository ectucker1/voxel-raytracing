#include "render_pass.hpp"

#include "engine/engine.hpp"
#include "engine/pipeline/framebuffer.hpp"

void RenderPass::recordBegin(const vk::CommandBuffer& cmd, const Framebuffer& framebuffer) const
{
    vk::RenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.renderArea.offset = 0;
    renderPassInfo.renderArea.offset = 0;
    renderPassInfo.renderArea.extent = vk::Extent2D(framebuffer.size.x, framebuffer.size.y);
    renderPassInfo.framebuffer = framebuffer.framebuffer;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(defaultClears.size());
    renderPassInfo.pClearValues = defaultClears.data();
    cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

RenderPassBuilder& RenderPassBuilder::color(uint32_t attachment, vk::Format format, glm::vec4 clear)
{
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = attachment;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::ClearValue clearValue;
    clearValue.color = vk::ClearColorValue(std::array<float, 4> { clear.r, clear.g, clear.b, clear.a });

    colorAttachments.push_back(colorAttachment);
    colorAttachmentRefs.push_back(colorAttachmentRef);
    colorClears.push_back(clearValue);

    return *this;
}

RenderPassBuilder& RenderPassBuilder::depthStencil(uint32_t attachment, vk::Format format, float depthClear, uint32_t stencilClear)
{
    vk::AttachmentDescription depthAttachment;
    depthAttachment.format = format;
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = attachment;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::ClearValue clearValue;
    clearValue.depthStencil = vk::ClearDepthStencilValue(depthClear, stencilClear);

    depthStencilAttachment = depthAttachment;
    depthStencilAttachmentRef = depthAttachmentRef;
    depthStencilClear = clearValue;

    return *this;
}

RenderPass RenderPassBuilder::build()
{
    RenderPass renderPass(engine);

    // Create the subpass
    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments = colorAttachmentRefs.data();
    if (depthStencilAttachmentRef.has_value())
    {
        subpass.pDepthStencilAttachment = &depthStencilAttachmentRef.value();
    }

    std::vector<vk::AttachmentDescription> attachments;
    attachments.insert(attachments.end(), colorAttachments.begin(), colorAttachments.end());
    if (depthStencilAttachment.has_value())
        attachments.push_back(depthStencilAttachment.value());
    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pSubpasses = &subpass;

    vk::RenderPass createdPass = engine->device.createRenderPass(renderPassInfo);
    renderPass.renderPass = createdPass;
    renderPass.pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.destroyRenderPass(createdPass);
    });

    renderPass.defaultClears.insert(renderPass.defaultClears.end(), colorClears.begin(), colorClears.end());
    if (depthStencilClear.has_value())
        renderPass.defaultClears.push_back(depthStencilClear.value());

    return renderPass;
}
