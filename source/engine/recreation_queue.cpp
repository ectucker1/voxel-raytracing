#include "recreation_queue.hpp"

#include "engine/engine.hpp"

RecreationQueue::RecreationQueue(const std::shared_ptr<Engine>& engine) : engine(engine) {}

void RecreationQueue::push(RecreationEventFlags flags, const CreatorFunc& creator) {
    DeletorFunc deletor = creator();

    _creators.emplace_back(flags, creator);
    _deletors.emplace_back(flags, deletor);
}

void RecreationQueue::fire(RecreationEventFlags flags)
{
    engine->device.waitIdle();

    // Fire relevant deletors
    for (size_t i = _deletors.size() - 1; i != -1; i--)
    {
        const auto& pair = _deletors[i];
        if (pair.first & flags)
            pair.second(engine);
    }

    // Call relevant creators and replace their deletors
    for (size_t i = 0; i < _creators.size(); i++)
    {
        const auto& pair = _creators[i];
        if (pair.first & flags)
        {
            DeletorFunc deletor = pair.second();
            _deletors[i] = std::make_pair(pair.first, deletor);
        }
    }
}
