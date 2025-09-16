#include "RubiksCube.h"
#include <cstdio>

int main(int argc, char* argv[]) {
    printf("=== Rubik's Cube Solver ===\n");
    printf("A clean, simple implementation using SDL2\n\n");
    
    RubiksCube cube;
    
    if (!cube.initialize(800, 600)) {
        printf("Failed to initialize Rubik's Cube\n");
        return -1;
    }
    
    printf("Initialization successful! Starting application...\n");
    
    cube.run();
    
    printf("Application finished gracefully\n");
    return 0;
}
