#include "Engine.h"

#include <algorithm>

Engine& Engine::getInstance()
{
    static Engine instance;
    return instance;
}

Engine::~Engine()
{
    shutdown();
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

Engine& Engine::setResolution(unsigned width, unsigned height)
{
    m_config.width = width;
    m_config.height = height;
    return *this;
}

Engine& Engine::setFullscreen(bool fullscreen) { m_config.fullscreen = fullscreen; return *this; }
Engine& Engine::setTitle(const std::string& title) { m_config.title = title; return *this; }
Engine& Engine::setVSync(bool enabled) { m_config.vsync = enabled; return *this; }
Engine& Engine::enableKeyboard(bool enabled) { m_config.keyboard = enabled; return *this; }
Engine& Engine::enableMouse(bool enabled) { m_config.mouse = enabled; return *this; }
// SCREEN COLOURING: sets the background colour; clearScreen() uses it from the next frame
Engine& Engine::setClearColor(sf::Color color) { m_config.clearColor = color; return *this; }
Engine& Engine::setLogFile(const std::string& path) { m_config.logFile = path; return *this; }

// MULTIBUFFERING: choose double or triple buffering. Works before init() and
// while the game runs (key B in the demo).
Engine& Engine::setBuffering(BufferingMode mode)
{
    m_config.buffering = mode;
    // Triple needs its 3 frame buffers; if they cannot be created, fall back to double
    if (m_initialized && mode == BufferingMode::Triple && !createFrameBuffers()) {
        m_log.warning("Could not create triple-buffer frame buffers, using double buffering");
        m_config.buffering = BufferingMode::Double;
    }
    if (m_initialized)
        m_log.info(std::string("Buffering: ") + (m_config.buffering == BufferingMode::Triple ? "triple" : "double"));
    return *this;
}

Engine& Engine::setResizable(bool resizable) { m_config.resizable = resizable; return *this; }

std::vector<sf::VideoMode> Engine::getAvailableModes()
{
    return sf::VideoMode::getFullscreenModes(); // sorted best -> worst
}

bool Engine::selectVideoMode(std::size_t index)
{
    const auto& modes = sf::VideoMode::getFullscreenModes();
    if (index >= modes.size()) {
        m_log.warning("selectVideoMode: index " + std::to_string(index) + " out of range (" +
                      std::to_string(modes.size()) + " modes available)");
        return false;
    }
    m_config.width = modes[index].size.x;
    m_config.height = modes[index].size.y;
    return true;
}

void Engine::reportError(const std::string& message, bool showDialog)
{
    if (showDialog)
        m_log.fatal(message);
    else
        m_log.error(message);
}

Engine& Engine::setFPS(unsigned fps)
{
    m_config.fps = (fps == 0) ? 60 : fps;
    return *this;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool Engine::init()
{
    if (m_initialized) {
        m_log.warning("init() called twice - ignoring");
        return true;
    }

    if (!m_log.open(m_config.logFile))
        m_log.warning("Could not open log file '" + m_config.logFile + "', logging to console only");

    m_log.info("Initializing engine...");
    logVideoModes();

    if (!createWindow()) {
        // Retry with safer settings before giving up
        if (m_config.fullscreen) {
            m_log.warning("Fullscreen failed, retrying in windowed mode");
            m_config.fullscreen = false;
            if (!createWindow()) {
                m_log.fatal("Could not create a window. Engine initialization failed.");
                return false;
            }
        } else {
            m_log.fatal("Could not create a window. Engine initialization failed.");
            return false;
        }
    }

    resetInputState();
    m_initialized = true;
    m_log.info("Engine initialized (" + std::to_string(m_config.width) + "x" +
               std::to_string(m_config.height) + (m_config.fullscreen ? ", fullscreen" : ", windowed") +
               ", " + std::to_string(m_config.fps) + " FPS, " +
               (m_config.buffering == BufferingMode::Triple ? "triple" : "double") + " buffering)");
    return true;
}

bool Engine::createWindow()
{
    sf::VideoMode mode({m_config.width, m_config.height});
    sf::State state = sf::State::Windowed;

    if (m_config.fullscreen) {
        if (!mode.isValid()) {
            m_log.warning("Resolution " + std::to_string(m_config.width) + "x" +
                          std::to_string(m_config.height) +
                          " is not supported in fullscreen, using desktop resolution");
            mode = sf::VideoMode::getDesktopMode();
            m_config.width = mode.size.x;
            m_config.height = mode.size.y;
        }
        state = sf::State::Fullscreen;
    } else {
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        if (m_config.width > desktop.size.x || m_config.height > desktop.size.y)
            m_log.warning("Window is larger than the desktop");
    }

    const auto style = m_config.resizable ? sf::Style::Default : (sf::Style::Titlebar | sf::Style::Close);
    m_window.create(mode, m_config.title, style, state);
    if (!m_window.isOpen()) {
        m_log.error("Failed to create the window");
        return false;
    }

    m_window.setVerticalSyncEnabled(m_config.vsync);
    // When vsync is on, the monitor limits the frame rate; otherwise we do
    m_window.setFramerateLimit(m_config.vsync ? 0 : m_config.fps);
    m_window.setKeyRepeatEnabled(false);
    m_window.setMouseCursorVisible(m_config.mouse);

    if (m_config.buffering == BufferingMode::Triple && !createFrameBuffers()) {
        m_log.warning("Could not create triple-buffer frame buffers, falling back to double buffering");
        m_config.buffering = BufferingMode::Double;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Multibuffering
// ---------------------------------------------------------------------------

// TRIPLE BUFFERING: creates the 3 off-screen frame buffers, the size of the window.
// Called at init, when switching to triple buffering and when the window is resized.
bool Engine::createFrameBuffers()
{
    const sf::Vector2u size = m_window.getSize();
    for (sf::RenderTexture& buffer : m_frameBuffers) {
        if (!buffer.resize(size))       // allocate the buffer on the graphics card
            return false;
        buffer.clear(m_config.clearColor);
        buffer.display();
    }
    // Starting roles: buffer 0 is drawn into, 1 waits, 2 is on screen
    m_backBuffer = 0;
    m_readyBuffer = 1;
    m_frontBuffer = 2;
    return true;
}

// MULTIBUFFERING: picks the hidden buffer this frame is drawn into.
// Everything the game draws goes there, never straight to the visible screen.
sf::RenderTarget& Engine::beginFrame()
{
    if (m_config.buffering == BufferingMode::Triple)
        m_currentTarget = &m_frameBuffers[m_backBuffer]; // our own back buffer
    else
        m_currentTarget = &m_window; // SFML's back buffer
    return *m_currentTarget;
}

// MULTIBUFFERING: shows the finished frame on screen.
// Only indices are swapped (std::swap on numbers), never the images themselves.
void Engine::presentFrame()
{
    if (m_config.buffering == BufferingMode::Triple) {
        // 1. The back buffer is finished: it becomes the "ready" frame
        m_frameBuffers[m_backBuffer].display();
        std::swap(m_backBuffer, m_readyBuffer);

        // 2. Show the ready frame: copy it 1:1 to the window
        const sf::Vector2u size = m_window.getSize();
        m_window.setView(sf::View(sf::FloatRect({0.f, 0.f}, sf::Vector2f(size))));
        sf::Sprite frame(m_frameBuffers[m_readyBuffer].getTexture());
        m_window.draw(frame, sf::RenderStates(sf::BlendNone));
        m_window.display();

        // 3. The shown frame becomes the front buffer; the old front one is
        //    free and will be drawn into on the next-but-one frame
        std::swap(m_readyBuffer, m_frontBuffer);
    } else {
        m_window.display(); // swap OpenGL back/front buffers
    }
    m_currentTarget = nullptr;
}

void Engine::logVideoModes()
{
    const auto& modes = sf::VideoMode::getFullscreenModes();
    m_log.info("Supported fullscreen modes: " + std::to_string(modes.size()));
    for (std::size_t i = 0; i < modes.size() && i < 5; ++i)
        m_log.info("  " + std::to_string(modes[i].size.x) + "x" + std::to_string(modes[i].size.y) +
                   " @ " + std::to_string(modes[i].bitsPerPixel) + " bpp");
}

bool Engine::toggleFullscreen()
{
    if (!m_initialized)
        return false;

    const EngineConfig previous = m_config;
    m_config.fullscreen = !m_config.fullscreen;
    if (m_config.fullscreen) {
        // Use the desktop resolution for fullscreen
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        m_config.width = desktop.size.x;
        m_config.height = desktop.size.y;
    }

    if (!createWindow()) {
        m_log.warning("Could not switch display mode, restoring previous mode");
        m_config = previous;
        createWindow();
        return false;
    }

    resetInputState();
    m_log.info(std::string("Switched to ") + (m_config.fullscreen ? "fullscreen" : "windowed") + " mode");
    return true;
}

void Engine::run()
{
    if (!m_initialized) {
        m_log.error("run() called before init()");
        return;
    }

    m_running = true;
    m_log.info("Main loop started");

    // Fixed-step timer: game logic updates exactly 'fps' times per second,
    // independent of how fast frames are rendered.
    const sf::Time step = sf::seconds(1.f / static_cast<float>(m_config.fps));
    const sf::Time maxFrameTime = sf::milliseconds(250);
    sf::Time accumulator = sf::Time::Zero;
    sf::Clock clock;
    m_fpsClock.restart();

    while (m_running && m_window.isOpen()) {
        try {
            runFrame(clock, accumulator, step, maxFrameTime);
        }
        catch (const std::exception& ex) {
            m_log.fatal(std::string("Unhandled exception in main loop: ") + ex.what());
            m_running = false;
        }
        catch (...) {
            m_log.fatal("Unknown exception in main loop");
            m_running = false;
        }
    }

    m_running = false;
    m_log.info("Main loop finished");
}

void Engine::runFrame(sf::Clock& clock, sf::Time& accumulator, sf::Time step, sf::Time maxFrameTime)
{
    // 1. Input devices
    processEvents();

    // 2-3. Logic + physics layer, driven by the timer
    accumulator += clock.restart();
    accumulator = std::min(accumulator, maxFrameTime); // avoid "spiral of death" after a stall
    while (accumulator >= step) {
        if (m_onUpdate)
            m_onUpdate(step.asSeconds());
        accumulator -= step;
    }

    // 4. Graphics layer - draw into the back buffer, then present it
    sf::RenderTarget& target = beginFrame(); // MULTIBUFFERING: get the hidden back buffer
    clearScreen(); // SCREEN COLOURING: fill the whole buffer with the background colour (setClearColor)
    if (m_onRender)
        m_onRender(target);              // the game draws its primitives into the back buffer
    presentFrame();                      // MULTIBUFFERING: swap / rotate the buffers, show the frame

    // 5. Sound layer - SFML plays sounds on its own thread; nothing to update yet

    updateFpsCounter();
}

void Engine::stop()
{
    m_running = false;
}

void Engine::shutdown()
{
    if (!m_initialized)
        return;

    m_log.info("Shutting down engine...");
    m_running = false;
    m_listeners.clear();
    m_onUpdate = nullptr;
    m_onRender = nullptr;
    m_bitmaps.clear(); // free bitmaps/textures while the graphics context still exists
    for (sf::RenderTexture& buffer : m_frameBuffers)
        buffer = sf::RenderTexture();
    if (m_window.isOpen())
        m_window.close();

    m_initialized = false;
    m_log.info("Engine shut down cleanly");
    m_log.close();
}

void Engine::updateFpsCounter()
{
    ++m_frameCount;
    const float elapsed = m_fpsClock.getElapsedTime().asSeconds();
    if (elapsed >= 1.f) {
        m_currentFps = static_cast<float>(m_frameCount) / elapsed;
        m_frameCount = 0;
        m_fpsClock.restart();
        m_window.setTitle(m_config.title + " | FPS: " + std::to_string(static_cast<int>(m_currentFps)));
    }
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

void Engine::setUpdateCallback(std::function<void(float)> callback)
{
    m_onUpdate = std::move(callback);
}

void Engine::setRenderCallback(std::function<void(sf::RenderTarget&)> callback)
{
    m_onRender = std::move(callback);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void Engine::addInputListener(InputListener* listener)
{
    if (listener && std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
        m_listeners.push_back(listener);
}

void Engine::removeInputListener(InputListener* listener)
{
    m_listeners.erase(std::remove(m_listeners.begin(), m_listeners.end(), listener), m_listeners.end());
}

// KEYBOARD POLLING: is this key held down right now? (reads m_keys, filled in processEvents)
// Use for continuous actions such as walking.
bool Engine::isKeyDown(sf::Keyboard::Key key) const
{
    const int index = static_cast<int>(key);
    // Bounds check: an unknown key must not read outside the array
    return index >= 0 && index < static_cast<int>(m_keys.size()) && m_keys[index];
}

// MOUSE POLLING: is this mouse button held down right now? (reads m_mouseButtons)
bool Engine::isMouseButtonDown(sf::Mouse::Button button) const
{
    const int index = static_cast<int>(button);
    return index >= 0 && index < static_cast<int>(m_mouseButtons.size()) && m_mouseButtons[index];
}

// MOUSE POSITION: where the mouse was at the last MouseMoved event (window pixels)
sf::Vector2i Engine::getMousePosition() const
{
    return m_mousePosition;
}

// Marks every key and mouse button as released.
// Called at init and when the window loses focus (see processEvents).
void Engine::resetInputState()
{
    m_keys.fill(false);
    m_mouseButtons.fill(false);
}

// INPUT DETECTION: reads every keyboard / mouse / window event of this frame.
// For each event it (1) stores the state for polling (m_keys, m_mouseButtons,
// m_mousePosition) and (2) notifies every registered InputListener (observer pattern).
void Engine::processEvents()
{
    while (const std::optional event = m_window.pollEvent()) {
        // Copy so listeners may add/remove themselves while being notified
        const std::vector<InputListener*> listeners = m_listeners;

        if (event->is<sf::Event::Closed>()) {
            m_log.info("Window close requested");
            stop();
        }
        else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            // Keep 1 pixel = 1 world unit after resizing
            m_window.setView(sf::View(sf::FloatRect({0.f, 0.f}, sf::Vector2f(resized->size))));
            if (m_config.buffering == BufferingMode::Triple && !createFrameBuffers()) {
                m_log.warning("Could not resize frame buffers, switching to double buffering");
                m_config.buffering = BufferingMode::Double;
            }
        }
        else if (event->is<sf::Event::FocusLost>()) {
            // Keys released while unfocused would otherwise stay "stuck"
            resetInputState();
        }
        // KEY DOWN
        else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            if (!m_config.keyboard) continue; // keyboard disabled with enableKeyboard(false)
            const int index = static_cast<int>(key->code);
            if (index >= 0 && index < static_cast<int>(m_keys.size()))
                m_keys[index] = true;                                      // remember it is held
            for (InputListener* l : listeners) l->onKeyPressed(key->code); // tell listeners
        }
        // KEY UP
        else if (const auto* key = event->getIf<sf::Event::KeyReleased>()) {
            if (!m_config.keyboard) continue;
            const int index = static_cast<int>(key->code);
            if (index >= 0 && index < static_cast<int>(m_keys.size()))
                m_keys[index] = false;                                      // no longer held
            for (InputListener* l : listeners) l->onKeyReleased(key->code); // tell listeners
        }
        // MOUSE CLICK
        else if (const auto* button = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (!m_config.mouse) continue; // mouse disabled with enableMouse(false)
            m_mouseButtons[static_cast<int>(button->button)] = true; // remember it is held
            for (InputListener* l : listeners) l->onMouseButtonPressed(button->button, button->position);
        }
        // MOUSE BUTTON RELEASED
        else if (const auto* button = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (!m_config.mouse) continue;
            m_mouseButtons[static_cast<int>(button->button)] = false;
            for (InputListener* l : listeners) l->onMouseButtonReleased(button->button, button->position);
        }
        // MOUSE MOVE
        else if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) {
            if (!m_config.mouse) continue;
            m_mousePosition = moved->position; // remember where it is (getMousePosition)
            for (InputListener* l : listeners) l->onMouseMoved(moved->position);
        }
        // MOUSE WHEEL (delta > 0: scrolled up / away from the user)
        else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
            if (!m_config.mouse) continue;
            for (InputListener* l : listeners) l->onMouseWheelScrolled(wheel->delta, wheel->position);
        }
    }
}

// ---------------------------------------------------------------------------
// Clearing
// ---------------------------------------------------------------------------

// SCREEN COLOURING: clears the screen with the background colour chosen with
// setClearColor(). Called by the main loop at the start of every frame.
void Engine::clearScreen()
{
    clearScreen(m_config.clearColor); // the colour chosen with setClearColor()
}

// SCREEN COLOURING: fills the whole frame being drawn with 'color'
void Engine::clearScreen(sf::Color color)
{
    if (m_currentTarget)
        m_currentTarget->clear(color); // the buffer being drawn (double or triple buffering)
    else
        m_window.clear(color);         // outside a frame: the window itself
}

bool Engine::saveScreenshot(const std::string& path)
{
    if (!m_initialized)
        return false;

    sf::Image image;
    if (m_config.buffering == BufferingMode::Triple) {
        image = m_frameBuffers[m_frontBuffer].getTexture().copyToImage();
    } else {
        sf::Texture texture;
        if (!texture.resize(m_window.getSize())) {
            m_log.error("saveScreenshot: could not create texture");
            return false;
        }
        texture.update(m_window); // copies the front buffer contents
        image = texture.copyToImage();
    }
    if (!image.saveToFile(path)) {
        m_log.error("Could not save screenshot to '" + path + "'");
        return false;
    }
    m_log.info("Screenshot saved to '" + path + "'");
    return true;
}

// BITMAP COLOURING: clears an off-screen bitmap (GPU render texture) to 'color'
void Engine::clearBitmap(sf::RenderTexture& bitmap, sf::Color color)
{
    bitmap.clear(color);
}

// BITMAP COLOURING: clears a CPU image to 'color' (same size, every pixel replaced)
void Engine::clearBitmap(sf::Image& bitmap, sf::Color color)
{
    bitmap = sf::Image(bitmap.getSize(), color);
}
