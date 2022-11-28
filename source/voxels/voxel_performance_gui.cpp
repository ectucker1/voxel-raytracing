#include "voxel_performance_gui.hpp"

#include <imgui.h>
#include <fmt/format.h>
#include <algorithm>

void VoxelPerformanceGui::draw(float delta)
{
    static float history[25];
    std::rotate(std::begin(history), std::next(std::begin(history)), std::end(history));
    history[0] = delta;

    ImGui::Begin("Performance");
    ImGui::LabelText("Frame Time", "%s", fmt::format("{}", delta * 1000).c_str());
    ImGui::PlotHistogram("Frame Time History", history, 25, 0, nullptr, 0, 1.0f / 30.0f, ImVec2(0, 80));
    ImGui::End();
}
