#include "voxel_settings_gui.hpp"

#include "voxels/resource/voxel_render_settings.hpp"
#include <imgui.h>
#include <cpp/imgui_stdlib.h>
#include <fmt/format.h>
#include "voxels/voxel_sdf_renderer.hpp"

const std::vector<FsrScaling> scalingOptions = {
    FsrScaling::NONE,
    FsrScaling::QUALITY,
    FsrScaling::BALANCED,
    FsrScaling::PERFORMANCE,
    FsrScaling::ULTRA_PERFORMANCE
};

static std::string scalingName(FsrScaling scaling)
{
    switch (scaling)
    {
        case FsrScaling::NONE:
            return "None (1.0x)";
        case FsrScaling::QUALITY:
            return "Quality (1.5x)";
        case FsrScaling::BALANCED:
            return "Balanced (1.7x)";
        case FsrScaling::PERFORMANCE:
            return "Performance (2x)";
        case FsrScaling::ULTRA_PERFORMANCE:
            return "Ultra Performance (3x)";
        default:
            return "Invalid";
    }
}

const std::vector<glm::uvec2> resolutionOptions = {
    glm::uvec2(3840, 2160),
    glm::uvec2(2560, 1440),
    glm::uvec2(1920, 1080)
};

static std::string resolutionName(glm::uvec2 resolution)
{
    return fmt::format("{}x{}", resolution.x, resolution.y);
}

void VoxelSettingsGui::draw(const VoxelSDFRenderer& renderer, const std::shared_ptr<VoxelRenderSettings>& settings)
{
    ImGui::Begin("Render Settings");

    if (ImGui::CollapsingHeader("Resolution", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("Target Resolution", resolutionName(settings->targetResolution).c_str()))
        {
            for (const glm::uvec2 resolution : resolutionOptions)
            {
                if (ImGui::Selectable(resolutionName(resolution).c_str(), settings->targetResolution == resolution))
                {
                    settings->targetResolution = resolution;
                    settings->targetResListeners.fireListeners();
                    settings->renderResListeners.fireListeners();
                }

                if (settings->targetResolution == resolution)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::Checkbox("Enable FSR", &settings->fsrSetttings.enable))
        {
            settings->renderResListeners.fireListeners();
        }

        if (ImGui::BeginCombo("FSR Quality", scalingName(settings->fsrSetttings.scaling).c_str()))
        {
            for (const FsrScaling scaling: scalingOptions)
            {
                if (ImGui::Selectable(scalingName(scaling).c_str(), settings->fsrSetttings.scaling == scaling))
                {
                    settings->fsrSetttings.scaling = scaling;
                    settings->renderResListeners.fireListeners();
                }

                if (settings->fsrSetttings.scaling == scaling)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    if (ImGui::CollapsingHeader("Denoiser", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable Denoiser", &settings->denoiserSettings.enable);

        ImGui::SliderInt("Denoiser Iterations", &settings->denoiserSettings.iterations, 1, 10);

        bool denoiserParamsChanged = false;
        denoiserParamsChanged |= ImGui::SliderFloat("Phi Color", &settings->denoiserSettings.phiColor0, 0.0f, 100.0f, "%.6f");
        denoiserParamsChanged |= ImGui::SliderFloat("Phi Normal", &settings->denoiserSettings.phiNormal0, 0.0f, 0.5f, "%.6f");
        denoiserParamsChanged |= ImGui::SliderFloat("Phi Position", &settings->denoiserSettings.phiPos0, 0.0f, 0.5f, "%.6f");
        denoiserParamsChanged |= ImGui::SliderFloat("Step Width", &settings->denoiserSettings.stepWidth, 0.0f, 5.0f, "%.6f");
        if (denoiserParamsChanged)
        {
            renderer.denoiser->setParameters(settings->denoiserSettings.phiColor0, settings->denoiserSettings.phiNormal0, settings->denoiserSettings.phiPos0, settings->denoiserSettings.stepWidth);
        }
    }

    if (ImGui::CollapsingHeader("Ambient Occlusion", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Occlusion Samples", &settings->occlusionSettings.numSamples, 1, 16);
    }

    ImGui::End();
}
