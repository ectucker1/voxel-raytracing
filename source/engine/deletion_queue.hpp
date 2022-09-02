#pragma once

#include <functional>
#include <unordered_map>
#include <deque>
#include "util/id_generator.hpp"

// A queue of functions to call to destroy Vulkan objects
class DeletionQueue
{
private:
    IdGenerator _groupIdGen;
    std::deque<uint32_t> _groups;
    std::unordered_map<uint32_t, std::deque<std::function<void()>>> _deletors;

public:
    uint32_t next_group();

    void push_deletor(uint32_t group, const std::function<void()>& deletor);
    uint32_t push_group(const std::function<void()>& deletor);

    void destroy_group(uint32_t group);
    void destroy_all();
};
