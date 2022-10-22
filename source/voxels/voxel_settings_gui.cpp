#include "voxel_settings_gui.hpp"

#include "voxels/resource/voxel_render_settings.hpp"
#include <imgui.h>
#include <cpp/imgui_stdlib.h>
#include <fmt/format.h>

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

void VoxelSettingsGui::draw(const std::shared_ptr<VoxelRenderSettings>& settings)
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

        if (ImGui::BeginCombo("FSR Quality", scalingName(settings->scaling).c_str()))
        {
            for (const FsrScaling scaling : scalingOptions)
            {
                if (ImGui::Selectable(scalingName(scaling).c_str(), settings->scaling == scaling))
                {
                    settings->scaling = scaling;
                    settings->renderResListeners.fireListeners();
                }

                if (settings->scaling == scaling)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::End();
}
