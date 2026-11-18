#pragma once

#include "BitmapHandler.h"
#include "InputListener.h"
#include "Logger.h"

#include <SFML/Graphics.hpp>

#include <array>
#include <functional>
#include <string>
#include <vector>

// Multibuffering strategy used by the main loop.
//  Double - SFML/OpenGL's own double buffer: we draw into the hidden back
//           buffer and RenderWindow::display() swaps it with the front one.
//  Triple - SFML has no triple-buffer option, so the engine manages three
//           off-screen frame buffers itself (back -> ready -> front rotation).
enum class BufferingMode { Double = 2, Triple = 3 };

struct EngineConfig {
    unsigned width = 800;
    unsigned height = 600;
    std::string title = "Game Engine";
    bool fullscreen = false;
    bool vsync = false;
    bool resizable = true;          // windowed mode only
    BufferingMode buffering = BufferingMode::Double;
    unsigned fps = 60;              // logic updates (timer ticks) per second
    bool keyboard = true;
    bool mouse = true;
    sf::Color clearColor = sf::Color::Black;
    std::string logFile = "engine_log.txt";
};

// Core of the game engine. Singleton: only one instance exists.
class Engine {
public:
    static Engine& getInstance();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // --- Configuration (call before init) ---
    Engine& setResolution(unsigned width, unsigned height);
    Engine& setFullscreen(bool fullscreen);
    Engine& setResizable(bool resizable);
    // Picks one of getAvailableModes() by index (fullscreen resolution selection)
    bool selectVideoMode(std::size_t index);
    static std::vector<sf::VideoMode> getAvailableModes();
    Engine& setTitle(const std::string& title);
    Engine& setFPS(unsigned fps);
    Engine& setVSync(bool enabled);
    Engine& enableKeyboard(bool enabled);
    Engine& enableMouse(bool enabled);
    Engine& setClearColor(sf::Color color);
    Engine& setLogFile(const std::string& path);
    // May also be called while running to switch strategy on the fly
    Engine& setBuffering(BufferingMode mode);

    // --- Error handling: logs the message, optionally shows a dialog ---
    void reportError(const std::string& message, bool showDialog = false);

    // --- Lifecycle ---
    bool init();
    void run();
    void stop();
    void shutdown();
    bool toggleFullscreen();

    // --- Game hooks called from the main loop ---
    void setUpdateCallback(std::function<void(float dt)> callback);
    void setRenderCallback(std::function<void(sf::RenderTarget& target)> callback);

    // --- Input: observer pattern ---
    void addInputListener(InputListener* listener);
    void removeInputListener(InputListener* listener);

    // --- Input: polling ---
    bool isKeyDown(sf::Keyboard::Key key) const;
    bool isMouseButtonDown(sf::Mouse::Button button) const;
    sf::Vector2i getMousePosition() const;

    // --- Clearing the screen / any bitmap ---
    // clearScreen() clears the buffer currently being drawn (the back buffer)
    void clearScreen();
    void clearScreen(sf::Color color);
    static void clearBitmap(sf::RenderTexture& bitmap, sf::Color color);
    static void clearBitmap(sf::Image& bitmap, sf::Color color);

    // Saves the frame currently on screen to an image file (png/bmp/jpg/tga)
    bool saveScreenshot(const std::string& path);

    // --- Access ---
    sf::RenderWindow& getWindow() { return m_window; }
    Logger& getLogger() { return m_log; }
    BitmapHandler& getBitmaps() { return m_bitmaps; }
    const EngineConfig& getConfig() const { return m_config; }
    float getCurrentFPS() const { return m_currentFps; }
    BufferingMode getBuffering() const { return m_config.buffering; }

private:
    Engine() = default;
    ~Engine();

    bool createWindow();
    void logVideoModes();
    void runFrame(sf::Clock& clock, sf::Time& accumulator, sf::Time step, sf::Time maxFrameTime);
    void processEvents();
    void updateFpsCounter();
    void resetInputState();

    // Multibuffering
    bool createFrameBuffers();
    sf::RenderTarget& beginFrame();
    void presentFrame();

    EngineConfig m_config;
    Logger m_log;
    BitmapHandler m_bitmaps{&m_log}; // after m_log: it logs through it
    sf::RenderWindow m_window;

    bool m_initialized = false;
    bool m_running = false;

    std::function<void(float)> m_onUpdate;
    std::function<void(sf::RenderTarget&)> m_onRender;

    std::vector<InputListener*> m_listeners;
    std::array<bool, sf::Keyboard::KeyCount> m_keys{};
    std::array<bool, sf::Mouse::ButtonCount> m_mouseButtons{};
    sf::Vector2i m_mousePosition;

    // Triple buffering: three frame buffers whose roles rotate every frame
    std::array<sf::RenderTexture, 3> m_frameBuffers;
    std::size_t m_backBuffer = 0;   // being drawn this frame
    std::size_t m_readyBuffer = 1;  // finished, waiting to be shown
    std::size_t m_frontBuffer = 2;  // currently on screen
    sf::RenderTarget* m_currentTarget = nullptr;

    sf::Clock m_fpsClock;
    unsigned m_frameCount = 0;
    float m_currentFps = 0.f;
};
