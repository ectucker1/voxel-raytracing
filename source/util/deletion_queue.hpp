#pragma once

#include <functional>
#include <unordered_map>
#include <deque>
#include "util/id_generator.hpp"

// A queue of functions to call to destroy Vulkan (or other) objects.
class DeletionQueue
{
private:
    IdGenerator _groupIdGen;
    std::deque<uint32_t> _groups;
    std::unordered_map<uint32_t, std::deque<std::function<void()>>> _deletors;

public:
    // Returns the ID of the next group of objects.
    uint32_t next_group();

    // Adds the given function as a deletor for the given group.
    void push_deletor(uint32_t group, const std::function<void()>& deletor);
    // Adds the given function to a new group on its own.
    // Returns the ID of the newly created group.
    uint32_t push_group(const std::function<void()>& deletor);

    // Calls all deletors for objects in the given group.
    void destroy_group(uint32_t group);
    // Calls all deletors for all groups.
    void destroy_all();
};
