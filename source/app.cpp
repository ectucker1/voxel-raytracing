#include "app.hpp"

#include <iostream>
#include "engine/engine.hpp"
#include "voxels/voxel_renderer.hpp"
#include "demo/triangle_renderer.hpp"

int run()
{
    try
    {
        std::shared_ptr<Engine> engine = std::make_shared<Engine>();
        engine->init();

        std::shared_ptr<ARenderer> renderer = std::make_shared<VoxelRenderer>(engine);
        engine->setRenderer(renderer);

        engine->run();
        engine->destroy();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
