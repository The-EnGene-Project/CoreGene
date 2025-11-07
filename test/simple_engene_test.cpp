#include <iostream>
#include <EnGene.h>

int main() {
    std::cout << "Starting simple EnGene test..." << std::endl;
    
    auto on_init = [](engene::EnGene& app) {
        std::cout << "on_init called" << std::endl;
    };
    
    auto on_update = [](double dt) {
        // Empty
    };
    
    auto on_render = [](double alpha) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    };
    
    engene::EnGeneConfig config;
    config.title = "Simple Test";
    
    try {
        std::cout << "Creating EnGene with nullptr handler..." << std::endl;
        engene::EnGene app(on_init, on_update, on_render, config, nullptr);
        std::cout << "EnGene created successfully!" << std::endl;
        std::cout << "Starting run loop..." << std::endl;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
