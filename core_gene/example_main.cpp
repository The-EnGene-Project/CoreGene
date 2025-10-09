#include "EnGene.h"
#include "basic_input_handler.h" // Your specific input handler
#include "scene.h"               // Your scene graph
#include "error.h"

int main() {
    // Define the user's one-time initialization logic
    auto on_init = []() {
        // This is the code that runs once at the start.
        // It's the equivalent of your old `initialize()` function.
        scene::graph()->initializeBaseShader("../shaders/vertex.glsl", "../shaders/fragment.glsl");
        // ... set up your initial scene, load models, etc.
    };

    // Define the user's per-frame update and drawing logic
    auto on_update = []() {
        // This code runs every frame.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // ... update object positions, handle animations ...
        
        scene::graph()->draw();
        
        Error::Check("update");
    };

    try {
        // 1. Create your input handler instance.
        auto* handler = new input::BasicInputHandler();

        // 2. Create the EnGene instance, passing in all your configurations.
        engene::EnGene app(
            800, 800,                           // Window dimensions
            "My Awesome EnGene App",            // Window title
            handler,                            // The input handler
            on_init,                            // Your init function
            on_update,                          // Your update function
            60                                  // Max framerate
        );

        // 3. Run the application. This call blocks until the window is closed.
        app.run();

    } catch (const std::runtime_error& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
