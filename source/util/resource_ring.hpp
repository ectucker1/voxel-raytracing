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
    size_t _curr = 0;

public:
    // Creates a new empty resource ring.
    ResourceRing() {}

    // Creates a new ring filled with n resources.
    // These resources are created by the given create function.
    static ResourceRing fromFunc(size_t n, const std::function<T(size_t i)>& create)
    {
        ResourceRing ring;
        ring._ring.reserve(n);
        for (size_t i = 0; i < n; i++)
        {
            ring._ring.push_back(create(i));
        }
        return ring;
    }

    // Creates a new ring filled with n resources.
    // These resources are created by constructor of the type, with the same arguments for each.
    template<class... Args>
    static ResourceRing fromArgs(size_t n, Args&&... args)
    {
        ResourceRing ring;
        ring._ring.reserve(n);
        for (size_t i = 0; i < n; i++)
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
        _curr = 0;
    }

    // Gets the number of objects in the ring.
    size_t size() const
    {
        return _ring.size();
    }

    // Gets the resource at the given index in the ring.
    const T& operator[](size_t i)
    {
        return _ring[i];
    }

    // Gets the next resource in the ring.
    // Repeatedly calling next will cycle through all resources in the loop.
    const T& next()
    {
        const T& ret = _ring[_curr];
        _curr++;
        if (_curr >= _ring.size())
            _curr = 0;
        return ret;
    }
};
