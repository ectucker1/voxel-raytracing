#include "recreation_queue.hpp"

#include "engine/engine.hpp"

RecreationQueue::RecreationQueue(const std::shared_ptr<Engine>& engine) : engine(engine) {}

uint32_t RecreationQueue::push(RecreationEventFlags flags, const CreatorFunc& creator) {
    uint32_t id = _recreatorIdGen.next();

    DeletorFunc deletor = creator();

    _creators.emplace_back(id, flags, creator);
    _deletors.emplace_back(id, flags, deletor);

    return id;
}

void RecreationQueue::fire(RecreationEventFlags flags)
{
    engine->device.waitIdle();

    // Fire relevant deletors
    for (size_t i = _deletors.size() - 1; i != -1; i--)
    {
        const auto& pair = _deletors[i];
        if (std::get<1>(pair) & flags)
            std::get<2>(pair)(engine);
    }

    // Call relevant creators and replace their deletors
    for (size_t i = 0; i < _creators.size(); i++)
    {
        const auto& pair = _creators[i];
        if (std::get<1>(pair) & flags)
        {
            DeletorFunc deletor = std::get<2>(pair)();
            _deletors[i] = std::make_tuple(std::get<0>(pair), std::get<1>(pair), deletor);
        }
    }
}

void RecreationQueue::remove(uint32_t id)
{
    int creatorIdx = -1;
    for (int i = 0; i < _creators.size(); i++)
    {
        if (std::get<0>(_creators[i]) == id)
            creatorIdx = i;
    }

    int deletorIdx = -1;
    for (int i = 0; i < _deletors.size(); i++)
    {
        if (std::get<0>(_deletors[i]) == id)
            deletorIdx = i;
    }

    if (creatorIdx != -1)
        _creators.erase(_creators.begin() + creatorIdx);

    if (deletorIdx != -1)
        _deletors.erase(_deletors.begin() + deletorIdx);

    _recreatorIdGen.reclaim(id);
}
