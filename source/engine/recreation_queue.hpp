#pragma once

#include <vector>
#include <functional>
#include <bitflags/bitflags.hpp>
#include "util/id_generator.hpp"

class Engine;

// The set of events a recreation queue can react to.
BEGIN_BITFLAGS(RecreationEventFlags)
    FLAG(NONE)
    FLAG(WINDOW_RESIZE)
    FLAG(RENDER_RESIZE)
    FLAG(TARGET_RESIZE)
    FLAG(DENOISER_SETTINGS)
    FLAG(SCENE_PATH)
END_BITFLAGS(RecreationEventFlags)

typedef std::function<void(const std::shared_ptr<Engine>& engine)> DeletorFunc;
typedef std::function<DeletorFunc()> CreatorFunc;

// An event queue which can call a series of creation functions,
// and then call corresponding deletion functions in the reverse order.
// This is useful for destroying and then recreating a chain of resources.
class RecreationQueue
{
private:
    IdGenerator _recreatorIdGen;

    std::shared_ptr<Engine> engine;

    std::vector<std::tuple<uint32_t, RecreationEventFlags, CreatorFunc>> _creators;
    std::vector<std::tuple<uint32_t, RecreationEventFlags, DeletorFunc>> _deletors;

public:
    explicit RecreationQueue(const std::shared_ptr<Engine>& engine);

    // Pushes a new creator onto the queue.
    // This will be called immediately, and then again each time fire is called.
    // The creator func may use reference capture ([&]),
    // but the deletor it returns must use value ([=]).
    uint32_t push(RecreationEventFlags flags, const CreatorFunc& creator);

    void remove(uint32_t id);

    // Calls all deletors with the given flags, and then calls their corresponding creators.
    void fire(RecreationEventFlags flags);
};
