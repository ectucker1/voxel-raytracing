#pragma once

#include <memory>
#include "engine/resource.hpp"

class Engine;

class ImguiRenderer : public AResource
{
public:
    ImguiRenderer(const std::shared_ptr<Engine>& engine, const vk::RenderPass& renderPass);

    void beginFrame() const;
    void draw(const vk::CommandBuffer& cmd) const;
};
