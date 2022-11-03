#include "voxel_renderer.hpp"

#include "engine/engine.hpp"
#include "voxels/voxel_settings_gui.hpp"
#include "engine/gui/imgui_renderer.hpp"
#include "engine/resource/texture_2d.hpp"
#include "voxels/stages/geometry_stage.hpp"
#include "voxels/stages/denoiser_stage.hpp"
#include "voxels/stages/upscaler_stage.hpp"
#include "voxels/stages/blit_stage.hpp"
#include "engine/commands/command_util.hpp"
#include "voxels/resource/screen_quad_push.hpp"
#include "engine/resource/render_image.hpp"

VoxelRenderer::VoxelRenderer(const std::shared_ptr<Engine>& engine) : ARenderer(engine)
{
    _settings = std::make_shared<VoxelRenderSettings>();

    _camera = std::make_unique<CameraController>(engine, glm::vec3(8, 8, -50), 90.0f, 0.0f, static_cast<float>(1 / glm::tan(glm::radians(55.0f / 2))));

    _scene = std::make_shared<VoxelScene>(engine, "../resource/treehouse.vox");
    _noiseTexture = std::make_shared<Texture2D>(engine, "../resource/blue_noise_rgba.png", 4, vk::Format::eR8G8B8A8Unorm);

    // TODO initialize stages
    _geometryStage = std::make_unique<GeometryStage>(engine, _settings, _scene, _noiseTexture);
    _denoiserStage = std::make_unique<DenoiserStage>(engine, _settings);
    _upscalerStage = std::make_unique<UpscalerStage>(engine, _settings);
    _blitStage = std::make_unique<BlitStage>(engine, _settings, *_windowRenderPass);

    _imguiRenderer = std::make_unique<ImguiRenderer>(engine, _windowRenderPass->renderPass);
}

void VoxelRenderer::update(float delta)
{
    _time += delta;

    _camera->update(delta);

    _upscalerStage->update(delta);

    _imguiRenderer->beginFrame();
    RecreationEventFlags flags = VoxelSettingsGui::draw(_settings);
    engine->recreationQueue->fire(flags);
}

void VoxelRenderer::recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame)
{
    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_settings->renderResolution().x);
    viewport.height = static_cast<float>(_settings->renderResolution().y);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = vk::Extent2D(_settings->renderResolution().x, _settings->renderResolution().y);
    commandBuffer.setScissor(0, 1, &scissor);

    // Set push constants
    ScreenQuadPush constants;
    constants.screenSize = glm::ivec2(_settings->renderResolution().x, _settings->renderResolution().y);
    constants.volumeBounds = glm::uvec3(_scene->width, _scene->height, _scene->depth);
    constants.camPos = glm::vec4(_camera->position, 1);
    constants.camDir = glm::vec4(_camera->direction, 0);
    constants.camUp = glm::vec4(_camera->up, 0);
    constants.camRight = glm::vec4(_camera->right, 0);
    constants.frame = glm::uvec1(_upscalerStage->frameCount);
    constants.cameraJitter.x = _upscalerStage->jitterX;
    constants.cameraJitter.y = _upscalerStage->jitterY;

    commandBuffer.pushConstants(_geometryStage->getPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(ScreenQuadPush), &constants);

    const GeometryBuffer& gBuffer = _geometryStage->record(commandBuffer, flightFrame);

    const RenderImage& denoisedColor = _settings->denoiserSettings.enable ? _denoiserStage->record(commandBuffer, flightFrame,
        gBuffer.color, gBuffer.normal, gBuffer.position) : gBuffer.color.get();

    const RenderImage& upscaled = _settings->fsrSetttings.enable ? _upscalerStage->record(commandBuffer,
        denoisedColor, gBuffer.depth, gBuffer.motion, gBuffer.mask) : denoisedColor;

    _blitStage->record(commandBuffer, flightFrame, upscaled, _windowFramebuffers[swapchainImage], *_windowRenderPass, [=](const vk::CommandBuffer& cmd) {_imguiRenderer->draw(cmd);});

    cmdutil::imageMemoryBarrier(
        commandBuffer,
        engine->swapchain.images[swapchainImage],
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eNone,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::ImageAspectFlagBits::eColor);
}
