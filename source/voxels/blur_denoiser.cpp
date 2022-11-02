#include "blur_denoiser.hpp"

#include <glm/gtx/functions.hpp>
#include <glm/glm.hpp>
#include "engine/commands/command_util.hpp"
#include "voxels/voxel_sdf_renderer.hpp"

struct DenoiserParams
{
    float phiColor;
    float phiNormal;
    float phiPos;
    float stepWidth;
};

BlurDenoiser::BlurDenoiser(const std::shared_ptr<Engine>& engine, const std::shared_ptr<VoxelRenderSettings>&, glm::uvec2 renderRes, float phiColor0, float phiNormal0, float phiPos0, float stepWidth) : AResource(engine), renderRes(renderRes)
{
    // Denoise parameters
    _denoiseParamsBuffer.reserve(MAX_DENOISER_PASSES);
    for (uint32_t i = 0; i < MAX_DENOISER_PASSES; i++)
    {
        _denoiseParamsBuffer.push_back(Buffer(engine, sizeof(DenoiserParams), vk::BufferUsageFlagBits::eUniformBuffer, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU));
    }
    setParameters(phiColor0, phiNormal0, phiPos0, stepWidth);
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        for (const Buffer& buffer : _denoiseParamsBuffer)
            buffer.destroy();
    });

    // Kernel offsets
    glm::vec2 offsets[DENOISER_KERNEL_SIZE];
    for (int i = 0, y = -DENOISER_KERNEL_RADIUS; y <= DENOISER_KERNEL_RADIUS; y++)
    {
        for (int x = -DENOISER_KERNEL_RADIUS; x <= DENOISER_KERNEL_RADIUS; x++, i++)
        {
            offsets[i] = glm::vec2(x, y);
        }
    }
    _denoiseOffsetBuffer = Buffer(engine, sizeof(offsets), vk::BufferUsageFlagBits::eUniformBuffer, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU);
    _denoiseOffsetBuffer->copyData(offsets, _denoiseOffsetBuffer->size);
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        _denoiseOffsetBuffer->destroy();
    });

    // Kernel weights
    float weights[DENOISER_KERNEL_SIZE];
    for (int i = 0, y = -DENOISER_KERNEL_RADIUS; y <= DENOISER_KERNEL_RADIUS; y++)
    {
        for (int x = -DENOISER_KERNEL_RADIUS; x <= DENOISER_KERNEL_RADIUS; x++, i++)
        {
            weights[i] = glm::gauss(glm::vec2(x, y), glm::vec2(0, 0), glm::vec2(2.0, 2.0));
        }
    }
    _denoiseKernelBuffer = Buffer(engine, sizeof(weights), vk::BufferUsageFlagBits::eUniformBuffer, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU);
    _denoiseKernelBuffer->copyData(weights, _denoiseOffsetBuffer->size);
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        _denoiseKernelBuffer->destroy();
    });

    // Descriptor sets
    _denoiseDescriptors.reserve(MAX_DENOISER_PASSES);
    for (uint32_t i = 0; i < MAX_DENOISER_PASSES; i++)
    {
        _denoiseDescriptors.push_back(DescriptorSetBuilder(engine)
            .image(0, vk::ShaderStageFlagBits::eFragment)
            .image(1, vk::ShaderStageFlagBits::eFragment)
            .image(2, vk::ShaderStageFlagBits::eFragment)
            .buffer(3, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer)
            .buffer(4, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer)
            .buffer(5, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer)
            .build());
    }
    for (uint32_t i = 0; i < MAX_DENOISER_PASSES; i++)
    {
        _denoiseDescriptors[i].initBuffer(3, _denoiseParamsBuffer[i].buffer, _denoiseParamsBuffer[i].size, vk::DescriptorType::eUniformBuffer);
        _denoiseDescriptors[i].initBuffer(4, _denoiseKernelBuffer->buffer, _denoiseKernelBuffer->size, vk::DescriptorType::eUniformBuffer);
        _denoiseDescriptors[i].initBuffer(5, _denoiseOffsetBuffer->buffer, _denoiseOffsetBuffer->size, vk::DescriptorType::eUniformBuffer);
    }
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        for (const DescriptorSet& set : _denoiseDescriptors)
            set.destroy();
    });

    _denoiseColorTargets = ResourceRing<RenderImage>::fromArgs(2, engine, renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Unorm,
                                                               vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        _denoiseColorTargets.destroy([&](const RenderImage& image) {
            image.destroy();
        });
    });

    _denoisePasses = ResourceRing<RenderPass>::fromFunc(2, [&](uint32_t i) {
        return RenderPassBuilder(engine)
            .color(0, _denoiseColorTargets[i].format, glm::vec4(0.0f))
            .build();
    });
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        _denoisePasses.destroy([&](const RenderPass& pass) {
            pass.destroy();
        });
    });

    _denoiseFramebuffers = ResourceRing<Framebuffer>::fromFunc(2, [&](uint32_t n) {
        return FramebufferBuilder(engine, _denoisePasses[n].renderPass, renderRes)
            .color(_denoiseColorTargets[n].imageView)
            .build();
    });
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        _denoiseFramebuffers.destroy([&](const Framebuffer& framebuffer) {
            framebuffer.destroy();
        });
    });

    _denoisePipeline = DenoiserPipeline(DenoiserPipeline::build(engine, _denoisePasses[0].renderPass));
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        _denoisePipeline->destroy();
    });
}

void BlurDenoiser::initRenderResResources() {

}

void BlurDenoiser::setParameters(float phiColor0, float phiNormal0, float phiPos0, float stepWidth) const
{
    for (size_t i = 0; i < MAX_DENOISER_PASSES; i++)
    {
        DenoiserParams params = {};
        params.phiColor = 1.0f / i * phiColor0;
        params.phiNormal = 1.0f / i * phiNormal0;
        params.phiPos = 1.0f / i * phiPos0;
        params.stepWidth = i * stepWidth + 1.0f;
        _denoiseParamsBuffer[i].copyData(&params, sizeof(DenoiserParams));
    }
}

const RenderImage& BlurDenoiser::render(const vk::CommandBuffer& cmd, uint32_t flightFrame, int iterations,
                                        const RenderImage& colorInput, const RenderImage& normalInput, const RenderImage& posInput) const
{
    // Set viewport
    vk::Viewport renderResViewport;
    renderResViewport.x = 0.0f;
    renderResViewport.y = 0.0f;
    renderResViewport.width = static_cast<float>(renderRes.x);
    renderResViewport.height = static_cast<float>(renderRes.y);
    renderResViewport.minDepth = 0.0f;
    renderResViewport.maxDepth = 1.0f;

    // Set scissor
    vk::Rect2D renderResScissor;
    renderResScissor.offset = vk::Offset2D(0, 0);
    renderResScissor.extent = vk::Extent2D(renderRes.x, renderRes.y);

    int lastOutput = 0;
    for (int i = 0; i < iterations; i++)
    {
        int ping = i % 2;

        // Use color input on first iteration, last denoiser pass otherwise
        if (i == 0)
            _denoiseDescriptors[i].writeImage(0, flightFrame, colorInput.imageView, colorInput.sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        else
            _denoiseDescriptors[i].writeImage(0, flightFrame, _denoiseColorTargets[lastOutput].imageView, _denoiseColorTargets[lastOutput].sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        // Normal and position inputs
        _denoiseDescriptors[i].writeImage(1, flightFrame, normalInput.imageView, normalInput.sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        _denoiseDescriptors[i].writeImage(2, flightFrame, posInput.imageView, posInput.sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        // Parameters
        _denoiseDescriptors[i].writeBuffer(3, flightFrame, _denoiseParamsBuffer[i].buffer, _denoiseParamsBuffer[i].size, vk::DescriptorType::eUniformBuffer);

        // Start denoise renderpass
        _denoisePasses[ping].recordBegin(cmd, _denoiseFramebuffers[ping]);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _denoisePipeline->pipeline);
        cmd.setViewport(0, 1, &renderResViewport);
        cmd.setScissor(0, 1, &renderResScissor);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _denoisePipeline->layout,
                               0, 1,
                               _denoiseDescriptors[i].getSet(flightFrame),
                               0, nullptr);
        cmd.draw(3, 1, 0, 0);
        cmd.endRenderPass();

        cmdutil::imageMemoryBarrier(
                cmd,
                _denoiseColorTargets[ping].image,
                vk::AccessFlagBits::eColorAttachmentWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::ImageAspectFlagBits::eColor);

        lastOutput = ping;
    }

    return _denoiseColorTargets[lastOutput];
}
