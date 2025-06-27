#include "application.hpp"
#include <iostream>

int main()
{
    Application app;

    if (!app.initialize())
    {
        std::cerr << "Application initialization failed. Exiting.\n";
        return -1;
    }

    app.run();

    return 0;
}