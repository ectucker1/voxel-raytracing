#pragma once

#include <memory>

class Engine;

// Represents a wrapper around a Vulkan (or other) resource.
// The constructor for these should maintain the following:
// - Will be fully created w/ valid Vulkan handles after the constructor finishes.
// - Will queue independent deletors for all created Vulkan handles under the deletorGroup.
class AResource
{
private:
    uint32_t deletorGroup;

protected:
    std::shared_ptr<Engine> engine;

public:
    // Creates a new resource.
    AResource(const std::shared_ptr<Engine>& engine);

    // Pushes a deletor to this resource's group, which includes a local pointer to the engine.
    // This wrapper is necessary so we don't lose the reference to the engine.
    void pushDeletor(std::function<void(const std::shared_ptr<Engine>& engine)> deletor);

    // Destroys this resource.
    // This relies on the engine's deletion queue for the deletor group.
    void destroy() const;

    // Destroys this resource, and then acquires a new deletor group.
    void resetDestroy();

    AResource(const AResource&) = default;
    AResource(AResource&&) = default;
    AResource& operator=(const AResource&) = default;
    AResource& operator=(AResource&&) = default;
    virtual ~AResource() = default;
};
