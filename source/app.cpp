#include "app.hpp"

#include <iostream>
#include "engine/engine.hpp"

int run() {
    Engine engine;

    try {
        engine.init();
        engine.run();
        engine.destroy();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
