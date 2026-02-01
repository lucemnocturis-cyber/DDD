/**
 * Dungeon Dice Duelists
 * A tactical roguelike strategy game
 * 
 * Main entry point
 */

#include "Core/Engine.h"
#include "Utils/Logger.h"

#include <cstdlib>
#include <exception>

int main(int argc, char* argv[]) {
    // Initialize logging
    DDD::Logger::Initialize();
    DDD::Logger::Info("Dungeon Dice Duelists starting...");
    
    try {
        // Create and run the engine
        DDD::Engine engine;
        
        if (!engine.Initialize()) {
            DDD::Logger::Error("Failed to initialize engine");
            return EXIT_FAILURE;
        }
        
        // Run the main game loop
        engine.Run();
        
        // Clean shutdown
        engine.Shutdown();
        
    } catch (const std::exception& e) {
        DDD::Logger::Error("Fatal exception: {}", e.what());
        return EXIT_FAILURE;
    }
    
    DDD::Logger::Info("Dungeon Dice Duelists shutdown complete");
    return EXIT_SUCCESS;
}
