#include "resource.hpp"

#include "engine/engine.hpp"

AResource::AResource(const std::shared_ptr<Engine>& engine) : engine(engine)
{
    deletorGroup = engine->deletionQueue.next_group();
}

void AResource::pushDeletor(std::function<void(const std::shared_ptr<Engine>&)> deletor)
{
    std::shared_ptr<Engine> localEngine = engine;
    engine->deletionQueue.push_deletor(deletorGroup, [=]() {
        deletor(localEngine);
    });
}

void AResource::destroy()
{
    engine->deletionQueue.destroy_group(deletorGroup);
}
