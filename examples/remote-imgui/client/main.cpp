#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "simple_opengl_client.hpp"


int main(int, char** argv) {
    SimpleOpenGLClient client;

    // Initialize client
    if (!client.Initialize(1280, 720, "Remote ImGui Client")) {
        std::cerr << "Failed to Initialize remote ImGui client!" << std::endl;
        return -1;
    }

    // Run main loop
    client.Run();

    return 0;
}