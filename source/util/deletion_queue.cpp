#include "deletion_queue.hpp"

uint32_t DeletionQueue::next_group()
{
    uint32_t group = _groupIdGen.next();
    _groups.push_front(group);
    return group;
}

void DeletionQueue::push_deletor(uint32_t group, const std::function<void()>& deletor)
{
    auto groupIt = _deletors.find(group);
    if (groupIt != _deletors.end())
    {
        groupIt->second.push_front(deletor);
    }
    else
    {
        _deletors.emplace(group, std::deque<std::function<void()>>());
        _deletors[group].push_front(deletor);
    }
}

uint32_t DeletionQueue::push_group(const std::function<void()>& deletor)
{
    uint32_t group = next_group();
    push_deletor(group, deletor);
    return group;
}


void DeletionQueue::destroy_group(uint32_t group)
{
    auto groupIt = _deletors.find(group);
    if (groupIt != _deletors.end())
    {
        for (const auto& deletor : groupIt->second)
        {
            deletor();
        }
    }
    _deletors.erase(group);
}


void DeletionQueue::destroy_all()
{
    for (const uint32_t group: _groups)
    {
        auto groupIt = _deletors.find(group);
        if (groupIt != _deletors.end())
        {
            for (const auto& deletor : groupIt->second)
            {
                deletor();
            }
        }
    }

    _groups.clear();
    _deletors.clear();
}
