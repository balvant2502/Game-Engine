#pragma once
#include <iostream>
#include "game_engine.hpp"


int main() {
    try
    {
        Engine::GameEngine gameEngine;
        gameEngine.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}