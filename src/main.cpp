// src/main.cpp

#include "game_application.hpp"
#include <iostream>

int main() {
    std::cout << "--- main function started. ---\n";

    GameApplication gameApp;
    return gameApp.run();
}