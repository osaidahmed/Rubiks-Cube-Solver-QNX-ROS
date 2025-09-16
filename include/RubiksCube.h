#pragma once

#include <SDL.h>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <vector>
#include <array>

// 3D Vector structure
struct Vector3D {
    float x, y, z;
    
    Vector3D() : x(0), y(0), z(0) {}
    Vector3D(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vector3D operator+(const Vector3D& other) const {
        return Vector3D(x + other.x, y + other.y, z + other.z);
    }
    
    Vector3D operator-(const Vector3D& other) const {
        return Vector3D(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3D operator*(float scalar) const {
        return Vector3D(x * scalar, y * scalar, z * scalar);
    }
};

// 2D Point for screen coordinates
struct Point2D {
    int x, y;
    Point2D() : x(0), y(0) {}
    Point2D(int x, int y) : x(x), y(y) {}
};

// Rubik's cube colors
enum CubeColor {
    WHITE = 0,
    RED = 1, 
    BLUE = 2,
    ORANGE = 3,
    GREEN = 4,
    YELLOW = 5
};

// Cube faces
enum CubeFace {
    FRONT = 0,
    BACK = 1,
    LEFT = 2,
    RIGHT = 3,
    TOP = 4,
    BOTTOM = 5
};

class RubiksCube {
public:
    RubiksCube();
    ~RubiksCube();
    
    bool initialize(int width, int height);
    void run();
    void cleanup();
    
    void solve();
    void scanColors();
    void startTimer();
    void stopTimer();
    void resetTimer();

    void scramble();
    void reset();
    void rotateFace(CubeFace face, bool clockwise);
    
private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    bool m_running;
    
    cv::VideoCapture m_camera;
    bool m_isScanning;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
    std::chrono::duration<double> m_elapsedTime;
    bool m_isTiming;

    CubeColor m_state[6][3][3];
    
    // Kociemba algorithm coordinates
    struct CubeState {
        std::array<int, 8> cornerPerm;      // Corner permutation (0-7)
        std::array<int, 8> cornerOrient;    // Corner orientation (0-2)
        std::array<int, 12> edgePerm;       // Edge permutation (0-11)
        std::array<int, 12> edgeOrient;     // Edge orientation (0-1)
    };
    
    CubeState m_cubeState;
    
    // Move definitions for Kociemba
    enum Move {
        U1, U2, U3, D1, D2, D3, L1, L2, L3, 
        R1, R2, R3, F1, F2, F3, B1, B2, B3
    };
    
    // Move tables
    static const int cornerPermTable[18][8];
    static const int cornerOrientTable[18][8];
    static const int edgePermTable[18][12];
    static const int edgeOrientTable[18][12];
    
    // Pruning tables
    static std::vector<int> cornerOrientPruning;
    static std::vector<int> edgeOrientPruning;
    static std::vector<int> UDSlicePruning;
    static std::vector<int> cornerPermPruning;
    static std::vector<int> UDSliceSortedPruning;
    
    // Helper functions for Kociemba
    void initializeKociemba();
    void convertToCoordinates();
    void convertFromCoordinates();
    void applyMove(Move move);
    int getCornerOrientCoord() const;
    int getEdgeOrientCoord() const;
    int getUDSliceCoord() const;
    int getCornerPermCoord() const;
    int getUDSliceSortedCoord() const;
    std::vector<Move> solveKociemba();
    std::vector<Move> phase1Search(int co, int eo, int uds, int depth, int maxDepth);
    std::vector<Move> phase2Search(int cp, int udss, int depth, int maxDepth);
    bool isPhase1Solved(int co, int eo, int uds) const;
    bool isPhase2Solved(int cp, int udss) const;

    // 3D rotation angles
    float m_rotationX;
    float m_rotationY;
    float m_rotationZ;
    
    // Screen dimensions for centering
    int m_screenWidth;
    int m_screenHeight;
    
    // Mouse tracking for rotation
    bool m_mouseDown;
    int m_lastMouseX;
    int m_lastMouseY;
    
    void render();
    void handleEvents();
    
    void updateTimer();
    void renderTimer();

    // 3D rendering methods
    void draw3DCube();
    void draw3DFace(CubeFace face, const Vector3D vertices[4], CubeColor faceColors[3][3]);
    Point2D project3DTo2D(const Vector3D& point3D);
    Vector3D rotatePoint(const Vector3D& point, float angleX, float angleY, float angleZ);
    void drawQuad(const Point2D points[4], CubeColor faceColors[3][3]);
    
    SDL_Color getSDLColor(CubeColor color);
};
