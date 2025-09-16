# Rubik's Cube Solver

This is a simple Rubik's Cube solver application written in C++. It provides a 3D visualization of the cube and includes basic functionalities like solving, scrambling, and color scanning from a camera.

The project is designed to be cross-compiled for a QNX Real-Time Operating System (RTOS) environment.

## Features

*   **3D Visualization**: An interactive 3D rendering of the Rubik's Cube using the SDL2 library.
*   **Automated Solver**: Implements a two-phase algorithm to find a solution for the cube.
*   **Color Scanning**: Uses OpenCV to detect the cube's colors from a connected camera.
*   **Timer**: A simple on-screen timer to track solving time.
*   **Basic Controls**: Scramble, reset, and manual rotation of the cube.

## Technologies Used

*   **C++**: Core application logic.
*   **SDL2**: For 2D/3D graphics, windowing, and user input.
*   **OpenCV**: For camera interaction and color detection.
*   **Makefile**: For building the project.
*   **QNX**: The target operating system.

## Building and Running

This project is intended to be built within a specific Docker environment configured with the QNX toolchain.

1.  **Build the application**:
    ```sh
    make
    ```
    This command executes the build script inside the pre-configured Docker container.

2.  **Deploy and Run on Target**:
    ```sh
    make deploy run
    ```
    This will copy the compiled binary to the QNX target device and execute it. The target IP address and user can be configured in the `Makefile`.

## Controls

*   **Mouse Drag**: Rotate the cube in 3D space.
*   **`S`**: Solve the cube.
*   **`C`**: Scan colors from the camera.
*   **`T`**: Start/Stop the timer.
*   **`Y`**: Reset the timer.
*   **`R`**: Reset the cube to its solved state.
*   **`Space`**: Scramble the cube.
*   **`Q` or `ESC`**: Quit the application.
