#include "game_application.hpp"
#include <iostream>

int main()
{
    std::cout << "Starting GenesisForge...\n";
    GameApplication gameApp;
    std::cout << "Setting up game dependencies...\n";
    if (!gameApp.setupDependencies())
    {
        std::cerr << "Failed to set up game dependencies. Exiting.\n";
        return -1;
    }
    std::cout << "Dependencies set up successfully. Running game.\n";
    int result = gameApp.run();
    std::cout << "Game exited with code " << result << ".\n";
    return result;
}