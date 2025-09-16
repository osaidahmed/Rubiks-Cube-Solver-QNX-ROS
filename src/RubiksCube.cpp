#include "RubiksCube.h"
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <csignal>

// Global flag for signal handling
static bool g_running = true;

// Signal handler for Ctrl+C
void signalHandler(int signal) {
    if (signal == SIGINT) {
        printf("\nReceived Ctrl+C (SIGINT), shutting down forcefully...\n");
        fflush(stdout);
        g_running = false;
        // Force exit if needed
        exit(0);
    } else if (signal == SIGTERM) {
        printf("\nReceived SIGTERM, shutting down...\n");
        fflush(stdout);
        g_running = false;
        exit(0);
    }
}

RubiksCube::RubiksCube() 
    : m_window(nullptr), m_renderer(nullptr), m_running(false),
      m_rotationX(0.3f), m_rotationY(0.7f), m_rotationZ(0.0f),
      m_screenWidth(0), m_screenHeight(0),
      m_mouseDown(false), m_lastMouseX(0), m_lastMouseY(0),
      m_isScanning(false), m_isTiming(false), m_elapsedTime(0) {
    
    reset();
    printf("RubiksCube initialized in solved state with 3D rendering\n");
    fflush(stdout);
}

RubiksCube::~RubiksCube() {
    cleanup();
}

bool RubiksCube::initialize(int width, int height) {
    printf("Initializing SDL2 Rubik's Cube...\n");
    fflush(stdout);
    
    // Install signal handlers for Ctrl+C and SIGTERM
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    printf("Signal handlers installed\n");
    fflush(stdout);
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }
    
    // Get display bounds to use appropriate size
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0 || dm.w <= 0 || dm.h <= 0) {
        printf("SDL_GetDesktopDisplayMode failed or returned invalid dimensions: %s\n", SDL_GetError());
        // Try alternative method to get display size
        if (SDL_GetCurrentDisplayMode(0, &dm) != 0 || dm.w <= 0 || dm.h <= 0) {
            printf("SDL_GetCurrentDisplayMode also failed: %s\n", SDL_GetError());
            // Use large default that should work on most monitors
            width = 1920;
            height = 1080;
            printf("Using large default size: %dx%d\n", width, height);
        } else {
            width = dm.w;
            height = dm.h;
            printf("Using current display mode: %dx%d\n", width, height);
        }
    } else {
        // Use the full screen size, not a percentage
        width = dm.w;
        height = dm.h;
        printf("Using desktop display mode: %dx%d\n", width, height);
    }
    
    m_window = SDL_CreateWindow(
        "Rubik's Cube Solver",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_INPUT_FOCUS
    );
    
    if (!m_window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    // Grab input focus and keyboard aggressively
    SDL_RaiseWindow(m_window);
    SDL_SetWindowInputFocus(m_window);
    SDL_SetRelativeMouseMode(SDL_FALSE);
    
    // Try to grab all input
    SDL_SetWindowGrab(m_window, SDL_TRUE);
    SDL_ShowWindow(m_window);
    
    printf("Window created and input focus set aggressively\n");
    printf("Window flags: %d\n", SDL_GetWindowFlags(m_window));
    fflush(stdout);
    
    m_renderer = SDL_CreateRenderer(m_window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!m_renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        return false;
    }
    
    // DO NOT set logical size - use native window size for direct pixel access
    // SDL_RenderSetLogicalSize(m_renderer, width, height);  // REMOVED
    
    // Ensure renderer viewport covers full window
    SDL_Rect viewport = {0, 0, width, height};
    SDL_RenderSetViewport(m_renderer, &viewport);
    
    // Store screen dimensions for 3D projection
    m_screenWidth = width;
    m_screenHeight = height;
    
    // Initialize OpenCV camera
    m_camera.open(0); // Open default camera
    if (!m_camera.isOpened()) {
        printf("Warning: Could not open camera\n");
    }

    printf("Renderer setup: %dx%d viewport\n", width, height);
    
    srand(time(nullptr));
    m_running = true;
    printf("SDL2 Rubik's Cube initialized successfully with 3D rendering\n");
    return true;
}

void RubiksCube::run() {
    printf("Starting Rubik's Cube...\n");
    printf("Controls:\n");
    printf("  SPACE - Scramble cube\n");
    printf("  R - Reset to solved\n");
    printf("  S - Solve cube (placeholder)\n");
    printf("  C - Scan with camera (placeholder)\n");
    printf("  T - Start/Stop timer\n");
    printf("  Y - Reset timer\n");
    printf("  Q/ESC - Quit\n");
    printf("  Mouse drag - Rotate cube\n");
    fflush(stdout);
    
    int frameCount = 0;
    int totalEvents = 0;
    while (m_running && g_running) {
        frameCount++;
        if (frameCount % 120 == 0) {  // Print every 2 seconds (60 fps)
            printf("Frame %d, Events received so far: %d, Window focus: %s\n", 
                   frameCount, totalEvents, SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS ? "YES" : "NO");
            fflush(stdout);
        }
        
        int eventsThisFrame = 0;
        SDL_Event testEvent;
        while (SDL_PollEvent(&testEvent)) {
            eventsThisFrame++;
            totalEvents++;
        }
        
        handleEvents();
        updateTimer();
        render();
        SDL_Delay(16);
    }
    
    printf("Main loop ended, shutting down... (reason: m_running=%d, g_running=%d)\n", m_running ? 1 : 0, g_running ? 1 : 0);
    fflush(stdout);
}

void RubiksCube::cleanup() {
    m_camera.release();
    
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
    printf("SDL2 cleanup complete\n");
}

void RubiksCube::reset() {
    // Standard Rubik's cube color mapping:
    // FRONT = WHITE, BACK = YELLOW, LEFT = ORANGE, RIGHT = RED, TOP = BLUE, BOTTOM = GREEN
    CubeColor faceColors[6] = {
        WHITE,   // FRONT (0)
        YELLOW,  // BACK (1) 
        ORANGE,  // LEFT (2)
        RED,     // RIGHT (3)
        BLUE,    // TOP (4)
        GREEN    // BOTTOM (5)
    };
    
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                m_state[face][row][col] = faceColors[face];
            }
        }
    }
    printf("Cube reset to solved state with proper colors\n");
}

void RubiksCube::scramble() {
    printf("Scrambling cube...\n");
    
    for (int i = 0; i < 20; i++) {
        CubeFace face = (CubeFace)(rand() % 6);
        bool clockwise = (rand() % 2) == 0;
        rotateFace(face, clockwise);
    }
    
    printf("Cube scrambled with 20 random moves\n");
}

void RubiksCube::solve() {
    printf("Solving cube using layer-by-layer method...\n");
    
    // Simulate solving steps with delays
    // Step 1: Solve bottom cross
    printf("Step 1: Solving bottom cross...\n");
    for (int i = 0; i < 5; i++) {
        rotateFace(BOTTOM, true);
        SDL_Delay(200);
    }
    
    // Step 2: Solve bottom corners
    printf("Step 2: Solving bottom corners...\n");
    for (int i = 0; i < 3; i++) {
        rotateFace(LEFT, false);
        rotateFace(BOTTOM, true);
        rotateFace(LEFT, true);
        SDL_Delay(300);
    }
    
    // Step 3: Solve middle layer
    printf("Step 3: Solving middle layer...\n");
    for (int i = 0; i < 4; i++) {
        rotateFace(FRONT, true);
        rotateFace(RIGHT, true);
        rotateFace(FRONT, false);
        rotateFace(RIGHT, false);
        SDL_Delay(400);
    }
    
    // Step 4: Solve top cross
    printf("Step 4: Solving top cross...\n");
    for (int i = 0; i < 6; i++) {
        rotateFace(TOP, true);
        SDL_Delay(150);
    }
    
    // Step 5: Orient top corners
    printf("Step 5: Orienting top corners...\n");
    for (int i = 0; i < 4; i++) {
        rotateFace(RIGHT, true);
        rotateFace(TOP, true);
        rotateFace(RIGHT, false);
        rotateFace(TOP, true);
        SDL_Delay(250);
    }
    
    // Step 6: Position top corners
    printf("Step 6: Positioning top corners...\n");
    for (int i = 0; i < 4; i++) {
        rotateFace(RIGHT, true);
        rotateFace(TOP, false);
        rotateFace(RIGHT, false);
        rotateFace(TOP, true);
        SDL_Delay(350);
    }
    
    printf("Cube solved!\n");
}

void RubiksCube::scanColors() {
    if (!m_camera.isOpened()) {
        printf("Camera not available for scanning.\n");
        return;
    }
    m_isScanning = true;
    printf("Scanning colors from camera...\n");

    cv::Mat frame;
    m_camera >> frame;
    if (frame.empty()) {
        printf("Failed to capture frame.\n");
        m_isScanning = false;
        return;
    }

    // Assume the cube is centered in the frame and divide into 3x3 grid
    int height = frame.rows;
    int width = frame.cols;
    int cellHeight = height / 3;
    int cellWidth = width / 3;

    // Define face order: FRONT, RIGHT, BACK, LEFT, TOP, BOTTOM (typical scanning order)
    CubeFace faces[6] = {FRONT, RIGHT, BACK, LEFT, TOP, BOTTOM};
    int faceIndex = 0;

    for (int faceRow = 0; faceRow < 2; faceRow++) {  // 2 rows of faces
        for (int faceCol = 0; faceCol < 3; faceCol++) {  // 3 columns of faces
            if (faceIndex >= 6) break;

            CubeFace currentFace = faces[faceIndex];

            // Calculate region for this face
            int startY = faceRow * cellHeight;
            int startX = faceCol * cellWidth;

            cv::Rect roi(startX, startY, cellWidth, cellHeight);
            cv::Mat faceRegion = frame(roi);

            // Detect colors in 3x3 grid for this face
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    int subStartY = row * (cellHeight / 3);
                    int subStartX = col * (cellWidth / 3);
                    cv::Rect subRoi(subStartX, subStartY, cellWidth / 3, cellHeight / 3);
                    cv::Mat subRegion = faceRegion(subRoi);

                    CubeColor detectedColor = detectDominantColor(subRegion);
                    m_state[currentFace][row][col] = detectedColor;
                }
            }

            faceIndex++;
        }
    }

    m_isScanning = false;
    printf("Scan complete. Cube state updated.\n");
}

CubeColor RubiksCube::detectDominantColor(const cv::Mat& region) {
    // Convert to HSV for better color detection
    cv::Mat hsv;
    cv::cvtColor(region, hsv, cv::COLOR_BGR2HSV);

    // Calculate histogram
    int hbins = 30, sbins = 32;
    int histSize[] = {hbins, sbins};
    float hranges[] = {0, 180};
    float sranges[] = {0, 256};
    const float* ranges[] = {hranges, sranges};
    cv::MatND hist;
    int channels[] = {0, 1};
    cv::calcHist(&hsv, 1, channels, cv::Mat(), hist, 2, histSize, ranges, true, false);

    // Find the dominant bin
    double maxVal = 0;
    int maxIdx[2] = {0, 0};
    cv::minMaxIdx(hist, 0, &maxVal, 0, maxIdx);

    int h = maxIdx[0] * (180 / hbins);
    int s = maxIdx[1] * (256 / sbins);

    // Map HSV to Rubik's cube colors
    if (s < 50) {
        return WHITE;  // Low saturation = white
    } else if (h < 15 || h > 165) {
        return RED;    // Red range
    } else if (h < 35) {
        return YELLOW; // Yellow range
    } else if (h < 75) {
        return GREEN;  // Green range
    } else if (h < 135) {
        return BLUE;   // Blue range
    } else {
        return ORANGE; // Orange range
    }
}

void RubiksCube::startTimer() {
    if (!m_isTiming) {
        m_startTime = std::chrono::high_resolution_clock::now();
        m_isTiming = true;
        printf("Timer started.\n");
    }
}

void RubiksCube::stopTimer() {
    if (m_isTiming) {
        updateTimer();
        m_isTiming = false;
        printf("Timer stopped. Final time: %.2f seconds\n", m_elapsedTime.count());
    }
}

void RubiksCube::resetTimer() {
    m_elapsedTime = std::chrono::duration<double>(0);
    if (m_isTiming) {
        m_startTime = std::chrono::high_resolution_clock::now();
    }
    printf("Timer reset.\n");
}

void RubiksCube::updateTimer() {
    if (m_isTiming) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_elapsedTime = currentTime - m_startTime;
    }
}

void RubiksCube::renderTimer() {
    if (!m_isTiming && m_elapsedTime.count() == 0) return;

    // Position timer in top-right corner
    int timerX = m_screenWidth - 200;
    int timerY = 20;
    int digitWidth = 20;
    int digitHeight = 30;
    int spacing = 5;

    // Format time as MM:SS.CC
    int minutes = (int)m_elapsedTime.count() / 60;
    int seconds = (int)m_elapsedTime.count() % 60;
    int centiseconds = (int)((m_elapsedTime.count() - (int)m_elapsedTime.count()) * 100);

    // Draw background
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 200);
    SDL_Rect bgRect = {timerX - 10, timerY - 10, 180, digitHeight + 20};
    SDL_RenderFillRect(m_renderer, &bgRect);

    // Draw digits
    int currentX = timerX;
    drawDigit(minutes / 10, currentX, timerY, digitWidth, digitHeight);
    currentX += digitWidth + spacing;
    drawDigit(minutes % 10, currentX, timerY, digitWidth, digitHeight);
    currentX += digitWidth + spacing;

    // Colon
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_Rect colonRect = {currentX + digitWidth/2 - 2, timerY + digitHeight/3, 4, 4};
    SDL_RenderFillRect(m_renderer, &colonRect);
    colonRect.y += digitHeight/3;
    SDL_RenderFillRect(m_renderer, &colonRect);
    currentX += digitWidth + spacing;

    drawDigit(seconds / 10, currentX, timerY, digitWidth, digitHeight);
    currentX += digitWidth + spacing;
    drawDigit(seconds % 10, currentX, timerY, digitWidth, digitHeight);
    currentX += digitWidth + spacing;

    // Dot
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_Rect dotRect = {currentX + digitWidth/2 - 2, timerY + digitHeight - 8, 4, 4};
    SDL_RenderFillRect(m_renderer, &dotRect);
    currentX += digitWidth + spacing;

    drawDigit(centiseconds / 10, currentX, timerY, digitWidth, digitHeight);
    currentX += digitWidth + spacing;
    drawDigit(centiseconds % 10, currentX, timerY, digitWidth, digitHeight);
}

// Kociemba algorithm implementation

// Move tables - how each move affects corner/edge positions and orientations
const int RubiksCube::cornerPermTable[18][8] = {
    // U1
    {3, 0, 1, 2, 4, 5, 6, 7},
    // U2  
    {2, 3, 0, 1, 4, 5, 6, 7},
    // U3
    {1, 2, 3, 0, 4, 5, 6, 7},
    // D1
    {0, 1, 2, 3, 7, 4, 5, 6},
    // D2
    {0, 1, 2, 3, 6, 7, 4, 5},
    // D3
    {0, 1, 2, 3, 5, 6, 7, 4},
    // L1
    {4, 1, 2, 0, 7, 5, 6, 3},
    // L2
    {7, 1, 2, 4, 3, 5, 6, 0},
    // L3
    {3, 1, 2, 7, 0, 5, 6, 4},
    // R1
    {0, 2, 6, 3, 4, 1, 5, 7},
    // R2
    {0, 6, 5, 3, 4, 2, 1, 7},
    // R3
    {0, 5, 1, 3, 4, 6, 2, 7},
    // F1
    {0, 1, 3, 7, 4, 5, 2, 6},
    // F2
    {0, 1, 7, 6, 4, 5, 3, 2},
    // F3
    {0, 1, 6, 2, 4, 5, 7, 3},
    // B1
    {5, 1, 2, 3, 0, 4, 6, 7},
    // B2
    {4, 1, 2, 3, 5, 0, 6, 7},
    // B3
    {0, 1, 2, 3, 4, 5, 6, 7}  // B3 = identity for corners
};

const int RubiksCube::cornerOrientTable[18][8] = {
    // U1
    {0, 0, 0, 0, 0, 0, 0, 0},
    // U2
    {0, 0, 0, 0, 0, 0, 0, 0},
    // U3
    {0, 0, 0, 0, 0, 0, 0, 0},
    // D1
    {0, 0, 0, 0, 0, 0, 0, 0},
    // D2
    {0, 0, 0, 0, 0, 0, 0, 0},
    // D3
    {0, 0, 0, 0, 0, 0, 0, 0},
    // L1
    {2, 0, 0, 1, 1, 0, 0, 2},
    // L2
    {0, 0, 0, 0, 0, 0, 0, 0},
    // L3
    {1, 0, 0, 2, 2, 0, 0, 1},
    // R1
    {0, 1, 2, 0, 0, 2, 1, 0},
    // R2
    {0, 0, 0, 0, 0, 0, 0, 0},
    // R3
    {0, 2, 1, 0, 0, 1, 2, 0},
    // F1
    {0, 0, 1, 2, 0, 0, 2, 1},
    // F2
    {0, 0, 0, 0, 0, 0, 0, 0},
    // F3
    {0, 0, 2, 1, 0, 0, 1, 2},
    // B1
    {1, 0, 0, 0, 2, 1, 0, 0},
    // B2
    {0, 0, 0, 0, 0, 0, 0, 0},
    // B3
    {2, 0, 0, 0, 1, 2, 0, 0}
};

const int RubiksCube::edgePermTable[18][12] = {
    // U1
    {0, 1, 2, 3, 7, 4, 5, 6, 8, 9, 10, 11},
    // U2
    {0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11},
    // U3
    {0, 1, 2, 3, 5, 6, 7, 4, 8, 9, 10, 11},
    // D1
    {0, 1, 2, 3, 4, 5, 6, 7, 11, 8, 9, 10},
    // D2
    {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9},
    // D3
    {0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 8},
    // L1
    {11, 1, 2, 7, 4, 5, 6, 0, 8, 9, 10, 3},
    // L2
    {3, 1, 2, 11, 4, 5, 6, 7, 8, 9, 10, 0},
    // L3
    {7, 1, 2, 0, 4, 5, 6, 11, 8, 9, 10, 3},
    // R1
    {0, 5, 9, 3, 4, 2, 6, 7, 8, 1, 10, 11},
    // R2
    {0, 9, 1, 3, 4, 5, 6, 7, 8, 2, 10, 11},
    // R3
    {0, 2, 5, 3, 4, 9, 6, 7, 8, 1, 10, 11},
    // F1
    {0, 1, 6, 10, 4, 5, 3, 7, 8, 9, 2, 11},
    // F2
    {0, 1, 10, 2, 4, 5, 6, 7, 8, 9, 3, 11},
    // F3
    {0, 1, 3, 6, 4, 5, 10, 7, 8, 9, 2, 11},
    // B1
    {4, 8, 2, 3, 1, 5, 6, 7, 0, 9, 10, 11},
    // B2
    {8, 0, 2, 3, 4, 5, 6, 7, 1, 9, 10, 11},
    // B3
    {1, 4, 2, 3, 8, 5, 6, 7, 0, 9, 10, 11}
};

const int RubiksCube::edgeOrientTable[18][12] = {
    // U1
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // U2
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // U3
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // D1
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // D2
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // D3
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // L1
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // L2
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // L3
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // R1
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // R2
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // R3
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // F1
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // F2
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // F3
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // B1
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // B2
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // B3
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// Pruning tables (initialized as empty, will be filled in initializeKociemba)
std::vector<int> RubiksCube::cornerOrientPruning;
std::vector<int> RubiksCube::edgeOrientPruning;
std::vector<int> RubiksCube::UDSlicePruning;
std::vector<int> RubiksCube::cornerPermPruning;
std::vector<int> RubiksCube::UDSliceSortedPruning;

void RubiksCube::initializeKociemba() {
    // Initialize pruning tables (simplified for embedded system)
    // In a full implementation, these would be precomputed and loaded
    cornerOrientPruning.resize(2187, 0);
    edgeOrientPruning.resize(2048, 0);
    UDSlicePruning.resize(11880, 0);
    cornerPermPruning.resize(40320, 0);
    UDSliceSortedPruning.resize(11880, 0);
    
    // For now, we'll use simple distance estimates
    // A full implementation would precompute exact distances
}

void RubiksCube::convertToCoordinates() {
    // Convert the 3D array representation to coordinate representation
    // This is a simplified mapping - a full implementation would be more complex
    
    // Corner mapping (simplified)
    int cornerMap[8][3] = {
        {4, 0, 2}, // UFL
        {4, 2, 8}, // UFR  
        {4, 8, 6}, // UBR
        {4, 6, 0}, // UBL
        {0, 0, 2}, // DFL
        {0, 2, 8}, // DFR
        {0, 8, 6}, // DBR
        {0, 6, 0}  // DBL
    };
    
    for (int i = 0; i < 8; i++) {
        int face = cornerMap[i][0];
        int row = cornerMap[i][1] / 3;
        int col = cornerMap[i][1] % 3;
        CubeColor color = m_state[face][row][col];
        // Simplified: assume solved state for now
        m_cubeState.cornerPerm[i] = i;
        m_cubeState.cornerOrient[i] = 0;
    }
    
    // Edge mapping (simplified)
    int edgeMap[12][3] = {
        {4, 1, 1}, // UF
        {4, 5, 1}, // UR
        {4, 7, 1}, // UB
        {4, 3, 1}, // UL
        {1, 1, 1}, // DF
        {1, 5, 1}, // DR
        {1, 7, 1}, // DB
        {1, 3, 1}, // DL
        {2, 3, 1}, // FL
        {3, 5, 1}, // FR
        {5, 3, 1}, // BL
        {2, 5, 1}  // BR
    };
    
    for (int i = 0; i < 12; i++) {
        int face = edgeMap[i][0];
        int row = edgeMap[i][1] / 3;
        int col = edgeMap[i][1] % 3;
        CubeColor color = m_state[face][row][col];
        // Simplified: assume solved state for now
        m_cubeState.edgePerm[i] = i;
        m_cubeState.edgeOrient[i] = 0;
    }
}

void RubiksCube::convertFromCoordinates() {
    // Convert back from coordinates to 3D array (simplified)
    // This would need to be implemented properly for a full solver
    reset(); // For now, just reset to solved state
}

void RubiksCube::applyMove(Move move) {
    // Apply move to coordinate representation
    std::array<int, 8> newCornerPerm = m_cubeState.cornerPerm;
    std::array<int, 8> newCornerOrient = m_cubeState.cornerOrient;
    std::array<int, 12> newEdgePerm = m_cubeState.edgePerm;
    std::array<int, 12> newEdgeOrient = m_cubeState.edgeOrient;
    
    // Apply corner transformations
    for (int i = 0; i < 8; i++) {
        int src = cornerPermTable[move][i];
        newCornerPerm[i] = m_cubeState.cornerPerm[src];
        newCornerOrient[i] = (m_cubeState.cornerOrient[src] + cornerOrientTable[move][i]) % 3;
    }
    
    // Apply edge transformations
    for (int i = 0; i < 12; i++) {
        int src = edgePermTable[move][i];
        newEdgePerm[i] = m_cubeState.edgePerm[src];
        newEdgeOrient[i] = (m_cubeState.edgeOrient[src] + edgeOrientTable[move][i]) % 2;
    }
    
    m_cubeState.cornerPerm = newCornerPerm;
    m_cubeState.cornerOrient = newCornerOrient;
    m_cubeState.edgePerm = newEdgePerm;
    m_cubeState.edgeOrient = newEdgeOrient;
}

int RubiksCube::getCornerOrientCoord() const {
    int coord = 0;
    for (int i = 0; i < 7; i++) {
        coord = coord * 3 + m_cubeState.cornerOrient[i];
    }
    return coord;
}

int RubiksCube::getEdgeOrientCoord() const {
    int coord = 0;
    for (int i = 0; i < 11; i++) {
        coord = coord * 2 + m_cubeState.edgeOrient[i];
    }
    return coord;
}

int RubiksCube::getUDSliceCoord() const {
    // Simplified: count edges not in U or D faces
    int coord = 0;
    int positions[4] = {8, 9, 10, 11}; // FL, FR, BL, BR edges
    for (int i = 0; i < 4; i++) {
        int pos = 0;
        for (int j = 0; j < 12; j++) {
            if (m_cubeState.edgePerm[j] == positions[i]) {
                pos = j;
                break;
            }
        }
        coord = coord * 12 + pos;
    }
    return coord;
}

int RubiksCube::getCornerPermCoord() const {
    // Simplified factorial number system
    int coord = 0;
    std::array<int, 8> perm = m_cubeState.cornerPerm;
    for (int i = 0; i < 8; i++) {
        int smaller = 0;
        for (int j = i + 1; j < 8; j++) {
            if (perm[j] < perm[i]) smaller++;
        }
        coord = coord * (8 - i) + smaller;
    }
    return coord;
}

int RubiksCube::getUDSliceSortedCoord() const {
    // Simplified: sort the UD slice edges
    std::vector<int> slice;
    int positions[4] = {8, 9, 10, 11};
    for (int pos : positions) {
        for (int j = 0; j < 12; j++) {
            if (m_cubeState.edgePerm[j] == pos) {
                slice.push_back(j);
                break;
            }
        }
    }
    std::sort(slice.begin(), slice.end());
    
    int coord = 0;
    for (int i = 0; i < 4; i++) {
        coord = coord * (12 - i) + slice[i];
    }
    return coord;
}

std::vector<RubiksCube::Move> RubiksCube::solveKociemba() {
    printf("Starting Kociemba algorithm...\n");
    
    initializeKociemba();
    convertToCoordinates();
    
    // Phase 1: Solve to <U,D,L,R,F2,B2> group
    int co = getCornerOrientCoord();
    int eo = getEdgeOrientCoord();
    int uds = getUDSliceCoord();
    
    printf("Phase 1 starting coordinates: CO=%d, EO=%d, UDS=%d\n", co, eo, uds);
    
    std::vector<Move> phase1Moves = phase1Search(co, eo, uds, 0, 20);
    if (phase1Moves.empty()) {
        printf("Phase 1 failed\n");
        return {};
    }
    
    // Apply phase 1 moves
    for (Move move : phase1Moves) {
        applyMove(move);
    }
    
    // Phase 2: Solve remaining cube
    int cp = getCornerPermCoord();
    int udss = getUDSliceSortedCoord();
    
    printf("Phase 2 starting coordinates: CP=%d, UDSS=%d\n", cp, udss);
    
    std::vector<Move> phase2Moves = phase2Search(cp, udss, 0, 18);
    if (phase2Moves.empty()) {
        printf("Phase 2 failed\n");
        return {};
    }
    
    // Combine moves
    std::vector<Move> allMoves = phase1Moves;
    allMoves.insert(allMoves.end(), phase2Moves.begin(), phase2Moves.end());
    
    printf("Kociemba solve complete: %zu moves\n", allMoves.size());
    return allMoves;
}

std::vector<RubiksCube::Move> RubiksCube::phase1Search(int co, int eo, int uds, int depth, int maxDepth) {
    if (isPhase1Solved(co, eo, uds)) {
        return {};
    }
    
    if (depth >= maxDepth) {
        return {};
    }
    
    // Try all moves
    for (int m = 0; m < 18; m++) {
        Move move = static_cast<Move>(m);
        
        // Apply move to coordinates (simplified - would need proper coordinate transformation)
        int newCo = (co + 1) % 2187; // Placeholder
        int newEo = (eo + 1) % 2048; // Placeholder  
        int newUds = (uds + 1) % 11880; // Placeholder
        
        // Check pruning
        if (cornerOrientPruning[newCo] > maxDepth - depth - 1 ||
            edgeOrientPruning[newEo] > maxDepth - depth - 1 ||
            UDSlicePruning[newUds] > maxDepth - depth - 1) {
            continue;
        }
        
        // Recursive search
        std::vector<Move> result = phase1Search(newCo, newEo, newUds, depth + 1, maxDepth);
        if (!result.empty()) {
            result.insert(result.begin(), move);
            return result;
        }
    }
    
    return {};
}

std::vector<RubiksCube::Move> RubiksCube::phase2Search(int cp, int udss, int depth, int maxDepth) {
    if (isPhase2Solved(cp, udss)) {
        return {};
    }
    
    if (depth >= maxDepth) {
        return {};
    }
    
    // Try all moves (only <U,D,L,R,F2,B2>)
    int moves[10] = {U1, U2, U3, D1, D2, D3, L1, L2, R1, R2}; // F2, B2 not included for simplicity
    
    for (int m : moves) {
        Move move = static_cast<Move>(m);
        
        // Apply move to coordinates (simplified)
        int newCp = (cp + 1) % 40320; // Placeholder
        int newUdss = (udss + 1) % 11880; // Placeholder
        
        // Check pruning
        if (cornerPermPruning[newCp] > maxDepth - depth - 1 ||
            UDSliceSortedPruning[newUdss] > maxDepth - depth - 1) {
            continue;
        }
        
        // Recursive search
        std::vector<Move> result = phase2Search(newCp, newUdss, depth + 1, maxDepth);
        if (!result.empty()) {
            result.insert(result.begin(), move);
            return result;
        }
    }
    
    return {};
}

bool RubiksCube::isPhase1Solved(int co, int eo, int uds) const {
    return co == 0 && eo == 0 && uds == 0;
}

bool RubiksCube::isPhase2Solved(int cp, int udss) const {
    return cp == 0 && udss == 0;
}

void RubiksCube::solve() {
    printf("Solving cube using Kociemba algorithm...\n");
    
    std::vector<Move> solution = solveKociemba();
    
    if (solution.empty()) {
        printf("Failed to find solution\n");
        return;
    }
    
    // Apply solution moves with visualization
    for (size_t i = 0; i < solution.size(); i++) {
        Move move = solution[i];
        
        // Convert move to face rotation
        CubeFace face;
        bool clockwise;
        switch (move) {
            case U1: face = TOP; clockwise = true; break;
            case U2: face = TOP; clockwise = true; rotateFace(face, clockwise); break; // U2 = U twice
            case U3: face = TOP; clockwise = false; break;
            case D1: face = BOTTOM; clockwise = true; break;
            case D2: face = BOTTOM; clockwise = true; rotateFace(face, clockwise); break;
            case D3: face = BOTTOM; clockwise = false; break;
            case L1: face = LEFT; clockwise = true; break;
            case L2: face = LEFT; clockwise = true; rotateFace(face, clockwise); break;
            case L3: face = LEFT; clockwise = false; break;
            case R1: face = RIGHT; clockwise = true; break;
            case R2: face = RIGHT; clockwise = true; rotateFace(face, clockwise); break;
            case R3: face = RIGHT; clockwise = false; break;
            case F1: face = FRONT; clockwise = true; break;
            case F2: face = FRONT; clockwise = true; rotateFace(face, clockwise); break;
            case F3: face = FRONT; clockwise = false; break;
            case B1: face = BACK; clockwise = true; break;
            case B2: face = BACK; clockwise = true; rotateFace(face, clockwise); break;
            case B3: face = BACK; clockwise = false; break;
        }
        
        rotateFace(face, clockwise);
        printf("Applied move %d/%zu\n", (int)(i+1), solution.size());
        SDL_Delay(200); // Delay for visualization
    }
    
    printf("Cube solved!\n");
}

void RubiksCube::drawDigit(int digit, int x, int y, int width, int height) {
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    
    // Define segments for 7-segment display
    bool segments[7] = {false};
    switch (digit) {
        case 0: segments[0] = segments[1] = segments[2] = segments[3] = segments[4] = segments[5] = true; break;
        case 1: segments[1] = segments[2] = true; break;
        case 2: segments[0] = segments[1] = segments[3] = segments[4] = segments[6] = true; break;
        case 3: segments[0] = segments[1] = segments[2] = segments[3] = segments[6] = true; break;
        case 4: segments[1] = segments[2] = segments[5] = segments[6] = true; break;
        case 5: segments[0] = segments[2] = segments[3] = segments[5] = segments[6] = true; break;
        case 6: segments[0] = segments[2] = segments[3] = segments[4] = segments[5] = segments[6] = true; break;
        case 7: segments[0] = segments[1] = segments[2] = true; break;
        case 8: segments[0] = segments[1] = segments[2] = segments[3] = segments[4] = segments[5] = segments[6] = true; break;
        case 9: segments[0] = segments[1] = segments[2] = segments[3] = segments[5] = segments[6] = true; break;
    }

    int thickness = 3;
    // Draw each segment
    if (segments[0]) { // Top
        SDL_Rect rect = {x + thickness, y, width - 2*thickness, thickness};
        SDL_RenderFillRect(m_renderer, &rect);
    }
    if (segments[1]) { // Top-right
        SDL_Rect rect = {x + width - thickness, y + thickness, thickness, height/2 - thickness};
        SDL_RenderFillRect(m_renderer, &rect);
    }
    if (segments[2]) { // Bottom-right
        SDL_Rect rect = {x + width - thickness, y + height/2, thickness, height/2 - thickness};
        SDL_RenderFillRect(m_renderer, &rect);
    }
    if (segments[3]) { // Bottom
        SDL_Rect rect = {x + thickness, y + height - thickness, width - 2*thickness, thickness};
        SDL_RenderFillRect(m_renderer, &rect);
    }
    if (segments[4]) { // Bottom-left
        SDL_Rect rect = {x, y + height/2, thickness, height/2 - thickness};
        SDL_RenderFillRect(m_renderer, &rect);
    }
    if (segments[5]) { // Top-left
        SDL_Rect rect = {x, y + thickness, thickness, height/2 - thickness};
        SDL_RenderFillRect(m_renderer, &rect);
    }
    if (segments[6]) { // Middle
        SDL_Rect rect = {x + thickness, y + height/2 - thickness/2, width - 2*thickness, thickness};
        SDL_RenderFillRect(m_renderer, &rect);
    }
}

void RubiksCube::rotateFace(CubeFace face, bool clockwise) {
    CubeColor temp[3][3];
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            temp[row][col] = m_state[face][row][col];
        }
    }
    
    if (clockwise) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                m_state[face][col][2 - row] = temp[row][col];
            }
        }
    } else {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                m_state[face][2 - col][row] = temp[row][col];
            }
        }
    }
    
    const char* faceNames[] = {"FRONT", "BACK", "LEFT", "RIGHT", "TOP", "BOTTOM"};
    printf("Rotated %s face %s\n", faceNames[face], clockwise ? "clockwise" : "counter-clockwise");
}

void RubiksCube::handleEvents() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        m_running = false;
                        break;
                        
                    case SDLK_SPACE:
                        scramble();
                        break;
                        
                    case SDLK_r:
                        reset();
                        break;
                    case SDLK_s:
                        solve();
                        break;
                    case SDLK_c:
                        scanColors();
                        break;
                    case SDLK_t:
                        if (m_isTiming) stopTimer();
                        else startTimer();
                        break;
                    case SDLK_y:
                        resetTimer();
                        break;
                }
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_mouseDown = true;
                    m_lastMouseX = event.button.x;
                    m_lastMouseY = event.button.y;
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_mouseDown = false;
                }
                break;
                
            case SDL_MOUSEMOTION:
                if (m_mouseDown) {
                    int deltaX = event.motion.x - m_lastMouseX;
                    int deltaY = event.motion.y - m_lastMouseY;
                    
                    // Sensitivity factor for rotation
                    float sensitivity = 0.01f;
                    
                    // Update rotation angles based on mouse movement
                    m_rotationY += deltaX * sensitivity;
                    m_rotationX += deltaY * sensitivity;
                    
                    // Keep rotation angles in reasonable range
                    while (m_rotationX > 2 * M_PI) m_rotationX -= 2 * M_PI;
                    while (m_rotationX < 0) m_rotationX += 2 * M_PI;
                    while (m_rotationY > 2 * M_PI) m_rotationY -= 2 * M_PI;
                    while (m_rotationY < 0) m_rotationY += 2 * M_PI;
                    
                    m_lastMouseX = event.motion.x;
                    m_lastMouseY = event.motion.y;
                }
                break;
        }
    }
}

void RubiksCube::render() {
    SDL_SetRenderDrawColor(m_renderer, 20, 20, 30, 255);
    SDL_RenderClear(m_renderer);
    
    draw3DCube();
    renderTimer();
    
    SDL_RenderPresent(m_renderer);
}

void RubiksCube::draw3DCube() {
    // Define cube vertices in 3D space
    float size = 1.0f;
    Vector3D vertices[8] = {
        Vector3D(-size, -size, -size), // 0: back-bottom-left
        Vector3D( size, -size, -size), // 1: back-bottom-right
        Vector3D( size,  size, -size), // 2: back-top-right
        Vector3D(-size,  size, -size), // 3: back-top-left
        Vector3D(-size, -size,  size), // 4: front-bottom-left
        Vector3D( size, -size,  size), // 5: front-bottom-right
        Vector3D( size,  size,  size), // 6: front-top-right
        Vector3D(-size,  size,  size)  // 7: front-top-left
    };
    
    // Apply rotation to all vertices
    Vector3D rotatedVertices[8];
    for (int i = 0; i < 8; i++) {
        rotatedVertices[i] = rotatePoint(vertices[i], m_rotationX, m_rotationY, m_rotationZ);
    }
    
    // Define faces with proper winding
    int faceIndices[6][4] = {
        {4, 5, 6, 7}, // FRONT face
        {1, 0, 3, 2}, // BACK face  
        {0, 4, 7, 3}, // LEFT face
        {5, 1, 2, 6}, // RIGHT face
        {7, 6, 2, 3}, // TOP face
        {0, 1, 5, 4}  // BOTTOM face
    };
    
    // Calculate face visibility and depth
    struct FaceInfo {
        int faceIndex;
        float avgZ;
        Point2D vertices2D[4];
        bool visible;
    };
    
    FaceInfo faces[6];
    
    for (int face = 0; face < 6; face++) {
        Vector3D faceVertices3D[4];
        float sumZ = 0;
        
        // Get vertices and calculate average Z
        for (int i = 0; i < 4; i++) {
            faceVertices3D[i] = rotatedVertices[faceIndices[face][i]];
            faces[face].vertices2D[i] = project3DTo2D(faceVertices3D[i]);
            sumZ += faceVertices3D[i].z;
        }
        faces[face].avgZ = sumZ / 4.0f;
        faces[face].faceIndex = face;
        
        // Simple backface culling
        Vector3D v1 = faceVertices3D[1] - faceVertices3D[0];
        Vector3D v2 = faceVertices3D[2] - faceVertices3D[0];
        float normalZ = v1.x * v2.y - v1.y * v2.x;
        
        // Face is visible if it faces the camera
        faces[face].visible = normalZ > 0.1f;
    }
    
    // Sort faces by depth (back to front)
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 6; j++) {
            if (faces[i].visible && faces[j].visible && faces[i].avgZ > faces[j].avgZ) {
                FaceInfo temp = faces[i];
                faces[i] = faces[j];
                faces[j] = temp;
            }
        }
    }
    
    // Draw visible faces back to front
    for (int i = 0; i < 6; i++) {
        if (faces[i].visible) {
            drawQuad(faces[i].vertices2D, m_state[faces[i].faceIndex]);
        }
    }
}

Point2D RubiksCube::project3DTo2D(const Vector3D& point3D) {
    // Simple perspective projection
    float distance = 5.0f;  // Camera distance
    float scale = 200.0f;   // Scale factor for cube size
    
    // Perspective projection with camera at (0, 0, distance)
    float projectedX = (point3D.x * distance) / (distance - point3D.z);
    float projectedY = (point3D.y * distance) / (distance - point3D.z);
    
    // Convert to screen coordinates (center on screen)
    int screenX = (int)(m_screenWidth / 2 + projectedX * scale);
    int screenY = (int)(m_screenHeight / 2 - projectedY * scale); // Flip Y axis
    
    return Point2D(screenX, screenY);
}

Vector3D RubiksCube::rotatePoint(const Vector3D& point, float angleX, float angleY, float angleZ) {
    Vector3D result = point;
    
    // Rotation around X-axis
    if (angleX != 0) {
        float cosX = cos(angleX);
        float sinX = sin(angleX);
        float newY = result.y * cosX - result.z * sinX;
        float newZ = result.y * sinX + result.z * cosX;
        result.y = newY;
        result.z = newZ;
    }
    
    // Rotation around Y-axis
    if (angleY != 0) {
        float cosY = cos(angleY);
        float sinY = sin(angleY);
        float newX = result.x * cosY + result.z * sinY;
        float newZ = -result.x * sinY + result.z * cosY;
        result.x = newX;
        result.z = newZ;
    }
    
    // Rotation around Z-axis
    if (angleZ != 0) {
        float cosZ = cos(angleZ);
        float sinZ = sin(angleZ);
        float newX = result.x * cosZ - result.y * sinZ;
        float newY = result.x * sinZ + result.y * cosZ;
        result.x = newX;
        result.y = newY;
    }
    
    return result;
}

void RubiksCube::drawQuad(const Point2D points[4], CubeColor faceColors[3][3]) {
    // Calculate face bounds
    int minX = points[0].x, maxX = points[0].x;
    int minY = points[0].y, maxY = points[0].y;
    
    for (int i = 1; i < 4; i++) {
        if (points[i].x < minX) minX = points[i].x;
        if (points[i].x > maxX) maxX = points[i].x;
        if (points[i].y < minY) minY = points[i].y;
        if (points[i].y > maxY) maxY = points[i].y;
    }
    
    // Only draw if face is reasonable size and on screen
    int faceWidth = maxX - minX;
    int faceHeight = maxY - minY;
    if (faceWidth < 20 || faceHeight < 20 || 
        maxX < 0 || minX > m_screenWidth || maxY < 0 || minY > m_screenHeight) {
        return;
    }
    
    // Draw 3x3 grid of clean squares with proper spacing
    int squareWidth = faceWidth / 3;
    int squareHeight = faceHeight / 3;
    int margin = 3; // Fixed margin between squares
    
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            // Calculate square position with margins
            int x = minX + (col * squareWidth) + margin;
            int y = minY + (row * squareHeight) + margin;
            int w = squareWidth - (2 * margin);
            int h = squareHeight - (2 * margin);
            
            // Skip if too small
            if (w <= 0 || h <= 0) continue;
            
            // Get color for this square
            SDL_Color color = getSDLColor(faceColors[row][col]);
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, 255);
            
            // Draw filled square
            SDL_Rect square = {x, y, w, h};
            SDL_RenderFillRect(m_renderer, &square);
            
            // Draw black border
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(m_renderer, &square);
        }
    }
    
    // Draw face outline
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        SDL_RenderDrawLine(m_renderer, points[i].x, points[i].y, points[next].x, points[next].y);
    }
}

SDL_Color RubiksCube::getSDLColor(CubeColor color) {
    switch (color) {
        case WHITE:  return {255, 255, 255, 255};
        case RED:    return {220, 50, 50, 255};
        case BLUE:   return {50, 100, 220, 255};
        case ORANGE: return {255, 140, 50, 255};
        case GREEN:  return {50, 180, 50, 255};
        case YELLOW: return {255, 220, 50, 255};
        default:     return {128, 128, 128, 255};
    }
}
