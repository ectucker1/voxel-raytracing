#pragma once

#include <memory>

class Engine;

// Represents a wrapper around a Vulkan (or other) resource.
// The constructor for these should maintain the following:
// - Will be fully created w/ valid Vulkan handles after the constructor finishes.
// - Will queue independent deletors for all created Vulkan handles under the deletorGroup.
class AResource
{
protected:
    std::shared_ptr<Engine> engine;
    uint32_t deletorGroup;

public:
    // Creates a new resource.
    AResource(const std::shared_ptr<Engine>& engine);
    // Destroys this resource.
    // This relies on the engine's deletion queue for the deletor group.
    void destroy();

    AResource(const AResource&) = default;
    AResource(AResource&&) = default;
    AResource& operator=(const AResource&) = default;
    AResource& operator=(AResource&&) = default;
    virtual ~AResource() = default;
};
