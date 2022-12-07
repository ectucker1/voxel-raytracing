#include "voxel_settings_gui.hpp"

#include "voxel_render_settings.hpp"
#include <imgui.h>
#include <cpp/imgui_stdlib.h>
#include <fmt/format.h>
#include "voxels/voxel_renderer.hpp"

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

RecreationEventFlags VoxelSettingsGui::draw(const std::shared_ptr<VoxelRenderSettings>& settings)
{
    RecreationEventFlags flags;

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
                    flags |= RecreationEventFlags::TARGET_RESIZE;
                    flags |= RecreationEventFlags::RENDER_RESIZE;
                }

                if (settings->targetResolution == resolution)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::Checkbox("Enable FSR", &settings->fsrSetttings.enable))
        {
            flags |= RecreationEventFlags::RENDER_RESIZE;
        }

        if (ImGui::BeginCombo("FSR Quality", scalingName(settings->fsrSetttings.scaling).c_str()))
        {
            for (const FsrScaling scaling: scalingOptions)
            {
                if (ImGui::Selectable(scalingName(scaling).c_str(), settings->fsrSetttings.scaling == scaling))
                {
                    settings->fsrSetttings.scaling = scaling;
                    flags |= RecreationEventFlags::RENDER_RESIZE;
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
            flags |= RecreationEventFlags::DENOISER_SETTINGS;
        }
    }

    if (ImGui::CollapsingHeader("Ambient Occlusion", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Occlusion Samples", &settings->occlusionSettings.numSamples, 0, 16);
    }

    if (ImGui::CollapsingHeader("Directional Light"), ImGuiTreeNodeFlags_DefaultOpen)
    {
        ImGui::SliderFloat3("Light Direction", reinterpret_cast<float*>(&settings->lightSettings.direction), -1.0f, 1.0f);
        ImGui::ColorEdit4("Light Color", reinterpret_cast<float*>(&settings->lightSettings.color), ImGuiColorEditFlags_HDR);
        ImGui::SliderFloat("Light Intensity", &settings->lightSettings.intensity, 0.0f, 5.0f);
        settings->lightSettings.direction = glm::normalize(settings->lightSettings.direction);
    }

    ImGui::End();

    return flags;
}
