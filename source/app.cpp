#include "app.hpp"

#include <iostream>
#include "engine/engine.hpp"
#include "demo/voxel_sdf_renderer.hpp"
#include "demo/triangle_renderer.hpp"

int run() {
    std::shared_ptr<Engine> engine = std::make_shared<Engine>();
    std::shared_ptr<ARenderer> renderer = std::make_shared<VoxelSDFRenderer>();

    try {
        engine->init(renderer);
        engine->run();
        engine->destroy();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
