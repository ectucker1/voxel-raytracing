#pragma once

#include <vector>
#include <functional>
#include <stdexcept>

// Defines a circular ring for a given kind of resource.
// This is useful for resources which are unique for each frame in flight,
// for each swapchain image, etc.
template<typename T>
class ResourceRing
{
private:
    std::vector<T> _ring;

public:
    // Creates a new empty resource ring.
    ResourceRing() {}

    // Creates a new ring filled with n resources.
    // These resources are created by the given create function.
    static ResourceRing fromFunc(uint32_t n, const std::function<T(uint32_t i)>& create)
    {
        ResourceRing ring;
        ring._ring.reserve(n);
        for (uint32_t i = 0; i < n; i++)
        {
            ring._ring.push_back(create(i));
        }
        return ring;
    }

    // Creates a new ring filled with n resources.
    // These resources are created by constructor of the type, with the same arguments for each.
    template<class... Args>
    static ResourceRing fromArgs(uint32_t n, Args&&... args)
    {
        ResourceRing ring;
        ring._ring.reserve(n);
        for (uint32_t i = 0; i < n; i++)
        {
            ring._ring.emplace_back(std::forward<Args>(args)...);
        }
        return ring;
    }

    // Destroys all resources in the ring, using the given destructor function.
    void destroy(const std::function<void(T)>& destroy)
    {
        for (T obj : _ring)
        {
            destroy(obj);
        }
        _ring.clear();
    }

    // Gets the number of objects in the ring.
    uint32_t size() const
    {
        return static_cast<uint32_t>(_ring.size());
    }

    // Gets the resource at the given index in the ring.
    const T& operator[](uint32_t i) const
    {
        return _ring[i];
    }
};
