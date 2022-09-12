#include "app.hpp"

#include <iostream>
#include "engine/engine.hpp"
#include "demo/voxel_sdf_pipeline.hpp"

int run() {
    std::shared_ptr<Engine> engine = std::make_shared<Engine>();

    try {
        auto buildPipeline = [&]()
        {
            VoxelSDFPipleineBuilder builder(engine);
            return builder.build(engine->renderPass);
        };

        engine->init(buildPipeline);
        engine->run();
        engine->destroy();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
