#pragma once

#include <memory>

class Engine;

// Builder class for a resource.
// Resources should be in a finalized ready state after going through their builder.
template<typename T>
class ResourceBuilder
{
protected:
    std::shared_ptr<Engine> engine;

public:
    // Begins a new builder.
    ResourceBuilder<T>(const std::shared_ptr<Engine>& engine) : engine(engine) {}

    // Actually creates the resource to be built.
    virtual T build() = 0;

    // Creates a unique pointer to a resource.
    std::unique_ptr<T> buildUnique()
    {
        return std::make_unique<T>(build());
    }

    // Creates a shared pointer to a resource.
    std::shared_ptr<T> buildShared()
    {
        return std::make_shared<T>(build());
    }

    // Delete move operations - builders should not be stored.
    ResourceBuilder<T>(const ResourceBuilder<T>&) = delete;
    ResourceBuilder<T>(ResourceBuilder<T>&&) = delete;
    ResourceBuilder<T>& operator=(const ResourceBuilder<T>&) = delete;
    ResourceBuilder<T>& operator=(ResourceBuilder<T>&&) = delete;
    virtual ~ResourceBuilder<T>() = default;
};
