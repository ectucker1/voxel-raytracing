#include "denoiser_stage.hpp"

#include <glm/gtx/functions.hpp>
#include <glm/glm.hpp>
#include "engine/resource/buffer.hpp"
#include "engine/commands/command_util.hpp"
#include "engine/pipeline/descriptor_set.hpp"
#include "engine/pipeline/render_pass.hpp"
#include "voxels/pipeline/denoiser_pipeline.hpp"
#include "engine/resource/render_image.hpp"
#include "engine/pipeline/framebuffer.hpp"
#include "engine/engine.hpp"

struct DenoiserParams
{
    float phiColor;
    float phiNormal;
    float phiPos;
    float stepWidth;
};

DenoiserStage::DenoiserStage(const std::shared_ptr<Engine>& engine, const std::shared_ptr<VoxelRenderSettings>& settings) : AVoxelRenderStage(engine, settings)
{
    // Denoise parameters
    _iterationParamsBuffers.reserve(MAX_DENOISER_PASSES);
    for (uint32_t i = 0; i < MAX_DENOISER_PASSES; i++)
    {
        _iterationParamsBuffers.push_back(Buffer(engine, sizeof(DenoiserParams), vk::BufferUsageFlagBits::eUniformBuffer, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU));
    }
    updateParameters();
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        for (const Buffer& buffer : _iterationParamsBuffers)
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
    _offsetBuffer = std::make_unique<Buffer>(engine, sizeof(offsets), vk::BufferUsageFlagBits::eUniformBuffer, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU);
    _offsetBuffer->copyData(offsets, _offsetBuffer->size);
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        _offsetBuffer->destroy();
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
    _kernelBuffer = std::make_unique<Buffer>(engine, sizeof(weights), vk::BufferUsageFlagBits::eUniformBuffer, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU);
    _kernelBuffer->copyData(weights, _kernelBuffer->size);
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        _kernelBuffer->destroy();
    });

    // Descriptor sets
    _iterationDescriptors.reserve(MAX_DENOISER_PASSES);
    for (uint32_t i = 0; i < MAX_DENOISER_PASSES; i++)
    {
        _iterationDescriptors.push_back(DescriptorSetBuilder(engine)
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
        _iterationDescriptors[i].initBuffer(3, _iterationParamsBuffers[i].buffer, _iterationParamsBuffers[i].size, vk::DescriptorType::eUniformBuffer);
        _iterationDescriptors[i].initBuffer(4, _kernelBuffer->buffer, _kernelBuffer->size, vk::DescriptorType::eUniformBuffer);
        _iterationDescriptors[i].initBuffer(5, _offsetBuffer->buffer, _offsetBuffer->size, vk::DescriptorType::eUniformBuffer);
    }
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        for (const DescriptorSet& set : _iterationDescriptors)
            set.destroy();
    });

    engine->recreationQueue->push(RecreationEventFlags::RENDER_RESIZE, [&]() {
        _colorTargets = ResourceRing<RenderImage>::fromArgs(2, engine, _settings->renderResolution().x, _settings->renderResolution().y, vk::Format::eR8G8B8A8Unorm,
                                                                   vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);

        return [=](const std::shared_ptr<Engine>&) {
            _colorTargets.destroy([&](const RenderImage& image) {
                image.destroy();
            });
        };
    });

    engine->recreationQueue->push(RecreationEventFlags::RENDER_RESIZE, [&]() {
        _renderPasses = ResourceRing<RenderPass>::fromFunc(2, [&](uint32_t i) {
            return RenderPassBuilder(engine)
                .color(0, _colorTargets[i].format, glm::vec4(0.0f))
                .build();
        });

        return [=](const std::shared_ptr<Engine>&) {
            _renderPasses.destroy([&](const RenderPass& pass) {
                pass.destroy();
            });
        };
    });

    engine->recreationQueue->push(RecreationEventFlags::RENDER_RESIZE, [&]() {
        _framebuffers = ResourceRing<Framebuffer>::fromFunc(2, [&](uint32_t n) {
            return FramebufferBuilder(engine, _renderPasses[n].renderPass, _settings->renderResolution())
                .color(_colorTargets[n].imageView)
                .build();
        });

        return [=](const std::shared_ptr<Engine>&) {
            _framebuffers.destroy([&](const Framebuffer& framebuffer) {
                framebuffer.destroy();
            });
        };
    });

    _pipeline = std::make_unique<DenoiserPipeline>(DenoiserPipeline::build(engine, _renderPasses[0].renderPass));
    pushDeletor([&](const std::shared_ptr<Engine>&) {
        _pipeline->destroy();
    });

    engine->recreationQueue->push(RecreationEventFlags::DENOISER_SETTINGS, [&]() {
        updateParameters();

        return [=](const std::shared_ptr<Engine>&) {};
    });
}

void DenoiserStage::updateParameters() const
{
    for (size_t i = 0; i < MAX_DENOISER_PASSES; i++)
    {
        DenoiserParams params = {};
        params.phiColor = 1.0f / i * _settings->denoiserSettings.phiColor0;
        params.phiNormal = 1.0f / i * _settings->denoiserSettings.phiNormal0;
        params.phiPos = 1.0f / i * _settings->denoiserSettings.phiPos0;
        params.stepWidth = i * _settings->denoiserSettings.stepWidth + 1.0f;
        _iterationParamsBuffers[i].copyData(&params, sizeof(DenoiserParams));
    }
}

const RenderImage& DenoiserStage::record(const vk::CommandBuffer& cmd, uint32_t flightFrame,
                                         const RenderImage& colorInput, const RenderImage& normalInput, const RenderImage& posInput) const
{
    cmdutil::imageMemoryBarrier(
        cmd,
        posInput.image,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::ImageAspectFlagBits::eColor);

    cmdutil::imageMemoryBarrier(
        cmd,
        normalInput.image,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::ImageAspectFlagBits::eColor);

    int lastOutput = 0;
    for (int i = 0; i < _settings->denoiserSettings.iterations; i++)
    {
        int ping = i % 2;

        // Use color input on first iteration, last denoiser pass otherwise
        if (i == 0)
            _iterationDescriptors[i].writeImage(0, flightFrame, colorInput.imageView, colorInput.sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        else
            _iterationDescriptors[i].writeImage(0, flightFrame, _colorTargets[lastOutput].imageView, _colorTargets[lastOutput].sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        // Normal and position inputs
        _iterationDescriptors[i].writeImage(1, flightFrame, normalInput.imageView, normalInput.sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        _iterationDescriptors[i].writeImage(2, flightFrame, posInput.imageView, posInput.sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        // Parameters
        _iterationDescriptors[i].writeBuffer(3, flightFrame, _iterationParamsBuffers[i].buffer, _iterationParamsBuffers[i].size, vk::DescriptorType::eUniformBuffer);

        // Start denoise renderpass
        _renderPasses[ping].recordBegin(cmd, _framebuffers[ping]);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline->pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->layout,
           0, 1,
           _iterationDescriptors[i].getSet(flightFrame),
           0, nullptr);
        cmd.draw(3, 1, 0, 0);
        cmd.endRenderPass();

        cmdutil::imageMemoryBarrier(
            cmd,
            _colorTargets[ping].image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);

        lastOutput = ping;
    }

    return _colorTargets[lastOutput];
}
