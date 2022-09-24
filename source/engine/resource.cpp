#include "resource.hpp"

#include "engine/engine.hpp"

AResource::AResource(const std::shared_ptr<Engine>& engine) : engine(engine)
{
    deletorGroup = engine->deletionQueue.next_group();
}

void AResource::destroy()
{
    engine->deletionQueue.destroy_group(deletorGroup);
}
