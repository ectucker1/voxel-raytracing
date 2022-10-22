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
        std::vector<std::function<void()>> foundDeletors;
        foundDeletors.insert(foundDeletors.end(), groupIt->second.begin(), groupIt->second.end());
        _deletors.erase(group);
        for (const auto& deletor : foundDeletors)
        {
            deletor();
        }
    }
}


void DeletionQueue::destroy_all()
{
    while (!_groups.empty())
    {
        uint32_t group = _groups.front();
        _groups.pop_front();
        destroy_group(group);
    }

    _groups.clear();
    _deletors.clear();
}
