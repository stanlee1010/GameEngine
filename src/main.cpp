// Engine demo - six scenes, switch with 1 - 6:
//
//  [1] Movement   Arrows/WASD move the square, Space recolours it,
//                 left click changes the background, right click teleports,
//                 mouse wheel resizes (keyboard + mouse, polling + observers)
//  [2] Lines      Library vs incremental line algorithm: line fans for every
//                 inclination, a magnified overlay and a pixel comparison table
//                 (T re-runs the comparison and writes it to the log)
//  [3] Shapes     Point2D, LineSegment, polylines and other primitives.
//                 Left click adds a polyline point, right click removes one,
//                 C toggles open/closed, Tab toggles the line algorithm,
//                 Backspace clears
//  [4] Circles    Own circle (8-fold symmetry) and ellipse (4-fold symmetry)
//                 algorithms vs the library's shapes, with automatic checks
//  [5] Fill       Paint canvas: polygons (rejected if segments intersect),
//                 flood fill, boundary fill and parity scan-line polygon fill;
//                 stamp a circle / ellipse / star / square at the mouse.
//                 Also the GameObject hierarchy: every primitive as a
//                 ShapeObject (spin / orbit / scale them) and a two-bitmap
//                 BitmapObject
//  [6] Player     Only the sprite-animated Player (SpriteObject): walk,
//                 shoot towards the mouse, resize with the mouse wheel
//
//  Global: B toggles double / triple buffering, F12 screenshot,
//          F11 fullscreen, Esc quits.
//
//  "GameEngine --screenshots" renders every scene to scene_N.png and exits.

#include "DemoAssets.h"
#include "Engine.h"
#include "LineComparison.h"
#include "LineSegment.h"
#include "Point2D.h"
#include "Player.h"
#include "PrimitiveRenderer.h"
#include "RasterTests.h"
#include "Shapes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979f;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::optional<sf::Font> g_font;

void loadFont()
{
    for (const char* path : {"C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/arial.ttf",
                             "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"}) {
        sf::Font font;
        if (font.openFromFile(path)) {
            g_font = std::move(font);
            return;
        }
    }
    Engine::getInstance().getLogger().warning("No system font found - on-screen text disabled");
}

void drawText(sf::RenderTarget& target, const std::string& string, sf::Vector2f position,
              unsigned size = 14, sf::Color color = sf::Color(220, 220, 220))
{
    if (!g_font)
        return;
    sf::Text text(*g_font, string, size);
    text.setFillColor(color);
    text.setPosition(position);
    target.draw(text);
}

sf::Color randomColor()
{
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 255);
    return sf::Color(static_cast<std::uint8_t>(dist(rng)),
                     static_cast<std::uint8_t>(dist(rng)),
                     static_cast<std::uint8_t>(dist(rng)));
}

const char* algorithmName(LineAlgorithm algorithm)
{
    return algorithm == LineAlgorithm::Incremental ? "incremental" : "library (default)";
}

// ---------------------------------------------------------------------------
// Scenes
// ---------------------------------------------------------------------------

class Scene : public InputListener {
public:
    virtual void onEnter() {}
    virtual void update(float /*dt*/) {}
    virtual void render(sf::RenderTarget& target) = 0;
};

// [1] Movement: keyboard/mouse input, clearing the screen and a bitmap
class MovementScene : public Scene {
public:
    MovementScene() : m_shape({50.f, 50.f})
    {
        m_shape.setFillColor(sf::Color::Green);
        m_shape.setPosition({375.f, 275.f});
        if (!m_panel.resize({200, 120}))
            Engine::getInstance().getLogger().error("Could not create the off-screen panel bitmap");
    }

    // SCREEN COLOURING: restore this scene's background colour when it is opened
    void onEnter() override { Engine::getInstance().setClearColor(m_background); }

    // KEYBOARD, polling: move the square while arrows/WASD are held
    void update(float dt) override
    {
        const Engine& engine = Engine::getInstance();
        sf::Vector2f direction{0.f, 0.f};
        if (engine.isKeyDown(sf::Keyboard::Key::Left)  || engine.isKeyDown(sf::Keyboard::Key::A)) direction.x -= 1.f;
        if (engine.isKeyDown(sf::Keyboard::Key::Right) || engine.isKeyDown(sf::Keyboard::Key::D)) direction.x += 1.f;
        if (engine.isKeyDown(sf::Keyboard::Key::Up)    || engine.isKeyDown(sf::Keyboard::Key::W)) direction.y -= 1.f;
        if (engine.isKeyDown(sf::Keyboard::Key::Down)  || engine.isKeyDown(sf::Keyboard::Key::S)) direction.y += 1.f;
        m_shape.move(direction * 300.f * dt); // 300 pixels per second
    }

    // MOUSE POSITION + bitmap clearing: the small panel top-left shows a dot where the mouse is
    void render(sf::RenderTarget& target) override
    {
        Engine& engine = Engine::getInstance();

        // Off-screen bitmap cleared with Engine::clearBitmap, showing the mouse
        Engine::clearBitmap(m_panel, sf::Color(0, 0, 0, 160));         // clear the bitmap
        const sf::Vector2f windowSize(engine.getWindow().getSize());
        const sf::Vector2f mouse(engine.getMousePosition());           // where is the mouse?
        PrimitiveRenderer panelRenderer(m_panel);
        panelRenderer.drawCircle({mouse.x / windowSize.x * 200.f, mouse.y / windowSize.y * 120.f}, 6.f,
                                 sf::Color::Yellow);
        m_panel.display();

        target.draw(m_shape);
        sf::Sprite panelSprite(m_panel.getTexture());
        panelSprite.setPosition({10.f, 10.f});
        target.draw(panelSprite);
        drawText(target, "Arrows/WASD move, Space recolour, LMB background, RMB teleport, wheel resize",
                 {10.f, 140.f});
    }

    // KEYBOARD, event: Space gives the square a new colour (fires once per press)
    void onKeyPressed(sf::Keyboard::Key key) override
    {
        if (key == sf::Keyboard::Key::Space)
            m_shape.setFillColor(randomColor());
    }

    // MOUSE, event: right click teleports the square, left click recolours the SCREEN
    void onMouseButtonPressed(sf::Mouse::Button button, sf::Vector2i position) override
    {
        if (button == sf::Mouse::Button::Right) {
            m_shape.setPosition(sf::Vector2f(position) - m_shape.getSize() / 2.f); // centre it on the mouse
        } else if (button == sf::Mouse::Button::Left) {
            m_background = randomColor();
            Engine::getInstance().setClearColor(m_background); // next frame clears in this colour
        }
    }

    // MOUSE WHEEL: resize the square (5 px per notch, kept between 10 and 300 px)
    void onMouseWheelScrolled(float delta, sf::Vector2i /*position*/) override
    {
        const float size = std::clamp(m_shape.getSize().x + delta * 5.f, 10.f, 300.f);
        m_shape.setSize({size, size});
    }

private:
    sf::RectangleShape m_shape;
    sf::RenderTexture m_panel;
    sf::Color m_background{30, 30, 40};
};

// [2] Lines: library algorithm vs incremental algorithm
class LineComparisonScene : public Scene {
public:
    LineComparisonScene()
    {
        runComparison();
        if (m_zoom.resize({60, 60})) {
            // Overlay both algorithms with additive blending:
            // red = library only, green = incremental only, yellow = both
            m_zoom.clear(sf::Color::Black);
            PrimitiveRenderer zoom(m_zoom);
            zoom.setBlendMode(sf::BlendAdd);
            drawFan(zoom, {30, 30}, 28.f, 16, sf::Color::Red, LineAlgorithm::Default);
            drawFan(zoom, {30, 30}, 28.f, 16, sf::Color::Green, LineAlgorithm::Incremental);
            m_zoom.display();
        }
    }

    void onEnter() override { Engine::getInstance().setClearColor(sf::Color(15, 15, 20)); }

    void render(sf::RenderTarget& target) override
    {
        PrimitiveRenderer renderer(target);

        // Fans of segments at 15-degree steps cover every inclination/octant
        drawText(target, "Library (OpenGL)", {75.f, 15.f});
        drawFan(renderer, {140, 160}, 110.f, 24, sf::Color::White, LineAlgorithm::Default);
        drawText(target, "Incremental (DDA)", {320.f, 15.f});
        drawFan(renderer, {390, 160}, 110.f, 24, sf::Color::White, LineAlgorithm::Incremental);

        // Magnified overlay (nearest-neighbour scaling keeps pixels sharp)
        sf::Sprite zoom(m_zoom.getTexture());
        zoom.setPosition({540.f, 40.f});
        zoom.setScale({4.f, 4.f});
        target.draw(zoom);
        renderer.drawRectangle({539.f, 39.f}, {242.f, 242.f}, sf::Color(90, 90, 90), false);
        drawText(target, "Overlay x4:", {540.f, 15.f});
        drawText(target, "both", {650.f, 15.f}, 14, sf::Color::Yellow);
        drawText(target, "lib", {695.f, 15.f}, 14, sf::Color::Red);
        drawText(target, "incr", {730.f, 15.f}, 14, sf::Color::Green);

        // Pixel comparison table
        float y = 295.f;
        drawText(target, "case                   expected  library  increm.  common  incremental", {10.f, y}, 13,
                 sf::Color(150, 200, 255));
        drawText(target, "T: re-run (also logged)", {580.f, y}, 13, sf::Color(150, 150, 150));
        for (const LineComparisonResult& r : m_results) {
            y += 17.f;
            const bool ok = r.incrementalPixels == r.expectedPixels && r.incrementalHasEnds && r.incrementalConnected;
            char line[128];
            std::snprintf(line, sizeof(line), "%-22s %8u %8u %8u %7u   %s", r.name.c_str(), r.expectedPixels,
                          r.defaultPixels, r.incrementalPixels, r.commonPixels, ok ? "OK" : "FAIL");
            drawText(target, line, {10.f, y}, 13, ok ? sf::Color(220, 220, 220) : sf::Color(255, 120, 120));
        }
    }

    void onKeyPressed(sf::Keyboard::Key key) override
    {
        if (key == sf::Keyboard::Key::T)
            runComparison();
    }

private:
    static void drawFan(PrimitiveRenderer& renderer, sf::Vector2i center, float radius, int count,
                        sf::Color color, LineAlgorithm algorithm)
    {
        const Point2D c{sf::Vector2f(center)};
        for (int i = 0; i < count; ++i) {
            const float angle = 2.f * kPi * static_cast<float>(i) / static_cast<float>(count);
            const Point2D end(std::round(c.getX() + radius * std::cos(angle)),
                              std::round(c.getY() + radius * std::sin(angle)));
            LineSegment(c, end).draw(renderer, color, algorithm);
        }
    }

    void runComparison()
    {
        m_results = compareLineAlgorithms();
        logLineComparison(Engine::getInstance().getLogger(), m_results);
    }

    std::vector<LineComparisonResult> m_results;
    sf::RenderTexture m_zoom;
};

// [3] Shapes: Point2D, LineSegment, polylines and other primitives
class ShapesScene : public Scene {
public:
    ShapesScene()
    {
        // Pentagram built from a set of LineSegment objects
        const sf::Vector2f center{650.f, 140.f};
        std::vector<Point2D> vertices;
        for (int i = 0; i < 5; ++i) {
            const float angle = -kPi / 2.f + 2.f * kPi * static_cast<float>(i) / 5.f;
            vertices.emplace_back(std::round(center.x + 90.f * std::cos(angle)),
                                  std::round(center.y + 90.f * std::sin(angle)));
        }
        for (int i = 0; i < 5; ++i)
            m_star.emplace_back(vertices[i], vertices[(i + 2) % 5]);

        m_polyline = {{60.f, 120.f}, {140.f, 60.f}, {220.f, 140.f}, {300.f, 70.f}, {380.f, 160.f}};
    }

    void onEnter() override { Engine::getInstance().setClearColor(sf::Color(20, 24, 32)); }

    void update(float /*dt*/) override
    {
        // LineSegment whose end follows the mouse (modifying an end point)
        const sf::Vector2i mouse = Engine::getInstance().getMousePosition();
        m_rubberBand.setEnd(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
    }

    void render(sf::RenderTarget& target) override
    {
        PrimitiveRenderer r(target);

        // Polyline from a vector<Point2D> (editable with the mouse)
        r.drawPolyline(m_polyline, sf::Color(80, 220, 255), m_closed, m_algorithm);
        for (const Point2D& p : m_polyline)
            p.draw(r, sf::Color::Yellow, 5.f);

        // Closed polyline from a vector<LineSegment>
        r.drawPolyline(m_star, sf::Color(255, 160, 60), m_algorithm);

        // Rubber-band segment from a fixed point to the mouse
        m_rubberBand.draw(r, sf::Color(255, 90, 160), m_algorithm);
        m_rubberBand.getStart().draw(r, sf::Color(255, 90, 160), 6.f);

        // Points of different sizes
        for (int i = 0; i < 5; ++i)
            Point2D(60.f + 30.f * static_cast<float>(i), 270.f).draw(r, sf::Color::White, 1.f + 2.f * static_cast<float>(i));

        // Library shapes
        r.drawRectangle({60.f, 320.f}, {100.f, 60.f}, sf::Color(70, 160, 90));
        r.drawRectangle({180.f, 320.f}, {100.f, 60.f}, sf::Color(70, 160, 90), false);
        r.drawCircle({350.f, 350.f}, 30.f, sf::Color(200, 80, 80));
        r.drawCircle({430.f, 350.f}, 30.f, sf::Color(200, 80, 80), false);
        r.drawTriangle({60.f, 480.f}, {120.f, 400.f}, {180.f, 480.f}, sf::Color(160, 120, 220));
        r.drawTriangle({200.f, 480.f}, {260.f, 400.f}, {320.f, 480.f}, sf::Color(160, 120, 220), false);
        std::vector<Point2D> hexagon;
        for (int i = 0; i < 6; ++i) {
            const float angle = 2.f * kPi * static_cast<float>(i) / 6.f;
            hexagon.emplace_back(400.f + 40.f * std::cos(angle), 440.f + 40.f * std::sin(angle));
        }
        r.fillConvexPolygon(hexagon, sf::Color(220, 200, 70));

        drawText(target, std::string("Algorithm: ") + algorithmName(m_algorithm) + "   Polyline: " +
                         (m_closed ? "closed" : "open") + " (" + std::to_string(m_polyline.size()) + " points)",
                 {10.f, 510.f});
        drawText(target, "LMB add point, RMB remove point, C open/closed, Tab algorithm, Backspace clear",
                 {10.f, 530.f}, 13, sf::Color(150, 150, 150));
    }

    void onKeyPressed(sf::Keyboard::Key key) override
    {
        if (key == sf::Keyboard::Key::C)
            m_closed = !m_closed;
        else if (key == sf::Keyboard::Key::Tab)
            m_algorithm = (m_algorithm == LineAlgorithm::Default) ? LineAlgorithm::Incremental : LineAlgorithm::Default;
        else if (key == sf::Keyboard::Key::Backspace)
            m_polyline.clear();
    }

    void onMouseButtonPressed(sf::Mouse::Button button, sf::Vector2i position) override
    {
        if (button == sf::Mouse::Button::Left)
            m_polyline.emplace_back(static_cast<float>(position.x), static_cast<float>(position.y));
        else if (button == sf::Mouse::Button::Right && !m_polyline.empty())
            m_polyline.pop_back();
    }

private:
    std::vector<Point2D> m_polyline;
    std::vector<LineSegment> m_star;
    LineSegment m_rubberBand{650.f, 420.f, 650.f, 420.f};
    bool m_closed = false;
    LineAlgorithm m_algorithm = LineAlgorithm::Default;
};

// [4] Circles: own circle / ellipse algorithms vs the library's shapes
class CirclesScene : public Scene {
public:
    CirclesScene()
    {
        runTests();
        if (m_zoom.resize({60, 60})) {
            // red = library only, green = own only, yellow = both
            m_zoom.clear(sf::Color::Black);
            PrimitiveRenderer zoom(m_zoom);
            zoom.setBlendMode(sf::BlendAdd);
            zoom.drawCircle({30.f, 30.f}, 26.f, sf::Color::Red, false);
            zoom.drawCircleIncremental({30.f, 30.f}, 26.f, sf::Color::Green);
            zoom.drawEllipse({30.f, 30.f}, 18.f, 9.f, sf::Color::Red, false);
            zoom.drawEllipseIncremental({30.f, 30.f}, 18.f, 9.f, sf::Color::Green);
            m_zoom.display();
        }
    }

    void onEnter() override { Engine::getInstance().setClearColor(sf::Color(15, 15, 20)); }

    void render(sf::RenderTarget& target) override
    {
        PrimitiveRenderer r(target);
        drawText(target, "Library (sf::Shape)", {55.f, 8.f});
        drawText(target, "Own (8/4-fold symmetry)", {290.f, 8.f});
        for (float radius = 12.f; radius <= 100.f; radius += 22.f) {
            r.drawCircle({135.f, 140.f}, radius, sf::Color::White, false);
            r.drawCircleIncremental({385.f, 140.f}, radius, sf::Color::White);
        }
        const float ellipses[][2] = {{115.f, 40.f}, {70.f, 20.f}, {25.f, 45.f}};
        for (const auto& e : ellipses) {
            r.drawEllipse({135.f, 300.f}, e[0], e[1], sf::Color(120, 200, 255), false);
            r.drawEllipseIncremental({385.f, 300.f}, e[0], e[1], sf::Color(120, 200, 255));
        }

        sf::Sprite zoom(m_zoom.getTexture());
        zoom.setPosition({540.f, 35.f});
        zoom.setScale({4.f, 4.f});
        target.draw(zoom);
        r.drawRectangle({539.f, 34.f}, {242.f, 242.f}, sf::Color(90, 90, 90), false);
        drawText(target, "Overlay x4:", {540.f, 8.f});
        drawText(target, "both", {650.f, 8.f}, 14, sf::Color::Yellow);
        drawText(target, "lib", {695.f, 8.f}, 14, sf::Color::Red);
        drawText(target, "own", {730.f, 8.f}, 14, sf::Color::Green);

        float y = 352.f;
        drawText(target, "Verification (own vs library):   T re-runs, also in engine_log.txt", {10.f, y}, 12,
                 sf::Color(150, 200, 255));
        for (const RasterTestResult& t : m_results) {
            y += 15.f;
            drawText(target, (t.passed ? "OK   " : "FAIL ") + t.group + " " + t.name + ": " + t.details, {10.f, y}, 11,
                     t.passed ? sf::Color(210, 210, 210) : sf::Color(255, 120, 120));
        }
    }

    void onKeyPressed(sf::Keyboard::Key key) override
    {
        if (key == sf::Keyboard::Key::T)
            runTests();
    }

private:
    void runTests()
    {
        m_results = testCirclesAndEllipses();
        Engine::getInstance().getLogger().info("Circle / ellipse algorithm tests (own vs library):");
        logRasterTests(Engine::getInstance().getLogger(), m_results);
    }

    std::vector<RasterTestResult> m_results;
    sf::RenderTexture m_zoom;
};

// A BitmapObject needs only a type name to be concrete
class Crate : public BitmapObject {
public:
    const char* getTypeName() const override { return "BitmapObject"; }
};

// The moving shape objects shown in scene 5 (they used to be in scene 6).
// GameObject hierarchy: the owner only knows GameObject pointers and asks each
// object, via dynamic_cast, whether it can be updated, drawn or transformed.
//   Space spin, M orbit, X scale pulse, I fill, Tab algorithm, Delete reset
class ShapeObjects {
public:
    ShapeObjects() { reset(); }

    // Applies the active transformations to every object (polymorphism)
    void update(float dt)
    {
        // Pulse: scale between 0.7 and 1.3; each frame scales by the ratio new/old
        float pulseRatio = 1.f;
        if (m_pulse) {
            m_pulseTime += dt;
            const float s = 1.f + 0.3f * std::sin(2.f * m_pulseTime);
            pulseRatio = s / m_pulseScale;
            m_pulseScale = s;
        }

        for (const auto& object : m_objects) {
            if (!object->isActive())
                continue;
            if (auto* updatable = dynamic_cast<UpdatableObject*>(object.get()))
                updatable->update(dt);                          // has game logic?
            if (auto* animated = dynamic_cast<AnimatedObject*>(object.get()))
                animated->animate(dt);                          // has an animation?
            if (auto* t = dynamic_cast<TransformableObject*>(object.get())) { // can be transformed?
                if (m_spin)  t->rotateAroundCenter(60.f * dt);  // 60 degrees per second
                if (m_orbit) t->rotate(20.f * dt, kOrbitCenter); // around the middle of the shape area
                if (m_pulse) t->scaleAroundCenter(pulseRatio);
            }
        }
    }

    // Draws every drawable object and writes its class name under it
    void draw(sf::RenderTarget& target) const
    {
        PrimitiveRenderer r(target);
        for (const auto& object : m_objects) {
            if (!object->isActive())
                continue;
            if (const auto* drawable = dynamic_cast<const DrawableObject*>(object.get()); drawable && drawable->isVisible())
                drawable->draw(r);
            if (const auto* t = dynamic_cast<const TransformableObject*>(object.get()))
                drawText(target, object->getTypeName(), t->getCenter() + sf::Vector2f(-30.f, 38.f), 11,
                         sf::Color(160, 160, 170));
        }
    }

    // Returns true when the key was one of the shape keys
    bool onKeyPressed(sf::Keyboard::Key key)
    {
        using Key = sf::Keyboard::Key;
        switch (key) {
        case Key::Space: m_spin = !m_spin; return true;
        case Key::M: m_orbit = !m_orbit; return true;
        case Key::X: m_pulse = !m_pulse; return true;
        case Key::I:
            m_filled = !m_filled;
            forEachShape([&](ShapeObject& s) { s.setFilled(m_filled); });
            return true;
        case Key::Tab:
            m_algorithm = m_algorithm == LineAlgorithm::Default ? LineAlgorithm::Incremental : LineAlgorithm::Default;
            forEachShape([&](ShapeObject& s) { s.setAlgorithm(m_algorithm); });
            return true;
        case Key::Delete: reset(); return true;
        default: return false;
        }
    }

    // One help line showing which effects are on
    std::string status() const
    {
        const auto onOff = [](bool on) { return on ? "[on]" : "[off]"; };
        return std::string("Shapes: Space spin ") + onOff(m_spin) + "  M orbit " + onOff(m_orbit) + "  X pulse " +
               onOff(m_pulse) + "  I fill " + onOff(m_filled) + "  Tab " +
               (m_algorithm == LineAlgorithm::Incremental ? "incremental" : "library") + "  Delete reset";
    }

private:
    static constexpr sf::Vector2f kOrbitCenter{545.f, 285.f};

    // Calls fn for every object that is a ShapeObject (not the bitmap crate)
    template <typename Fn>
    void forEachShape(Fn fn)
    {
        for (const auto& object : m_objects)
            if (auto* shape = dynamic_cast<ShapeObject*>(object.get()))
                fn(*shape);
    }

    // Creates the objects in the free area right of / below the canvas examples
    void reset()
    {
        m_objects.clear();
        m_pulseScale = 1.f;
        m_pulseTime = 0.f;

        auto point = std::make_unique<Point2D>(330.f, 165.f);
        point->setColor(sf::Color::Yellow);
        point->setSize(6.f);
        m_objects.push_back(std::move(point));

        auto segment = std::make_unique<LineSegment>(370.f, 145.f, 450.f, 200.f);
        segment->setColor(sf::Color(90, 220, 255));
        m_objects.push_back(std::move(segment));

        auto polyline = std::make_unique<PolylineObject>(
            std::vector<Point2D>{{480, 150}, {505, 195}, {530, 150}, {555, 195}, {580, 150}});
        polyline->setColor(sf::Color(255, 160, 60));
        m_objects.push_back(std::move(polyline));

        auto polygon = std::make_unique<PolygonObject>(
            std::vector<Point2D>{{615, 145}, {675, 145}, {690, 175}, {675, 205}, {645, 185}, {615, 205}});
        polygon->setColor(sf::Color(255, 120, 200));
        polygon->setFillColor(sf::Color(130, 40, 100));
        m_objects.push_back(std::move(polygon));

        auto rect = std::make_unique<RectangleObject>(700.f, 145.f, 60.f, 50.f);
        rect->setColor(sf::Color(120, 160, 255));
        rect->setFillColor(sf::Color(40, 60, 140));
        m_objects.push_back(std::move(rect));

        auto triangle = std::make_unique<TriangleObject>(Point2D(350, 255), Point2D(395, 335), Point2D(305, 335));
        triangle->setColor(sf::Color(200, 255, 120));
        triangle->setFillColor(sf::Color(70, 120, 30));
        m_objects.push_back(std::move(triangle));

        auto circle = std::make_unique<CircleObject>(sf::Vector2f(470.f, 300.f), 35.f);
        circle->setColor(sf::Color(255, 110, 110));
        circle->setFillColor(sf::Color(120, 40, 40));
        m_objects.push_back(std::move(circle));

        auto ellipse = std::make_unique<EllipseObject>(sf::Vector2f(600.f, 300.f), 60.f, 25.f);
        ellipse->setColor(sf::Color(180, 140, 255));
        ellipse->setFillColor(sf::Color(70, 50, 130));
        m_objects.push_back(std::move(ellipse));

        // A bitmap object made of two bitmaps: crate + star emblem on top
        BitmapHandler& bitmaps = Engine::getInstance().getBitmaps();
        if (!bitmaps.exists("crate"))
            DemoAssets::createCrate(bitmaps, "crate");
        if (!bitmaps.exists("star"))
            DemoAssets::createStar(bitmaps, "star");
        auto crate = std::make_unique<Crate>();
        if (const sf::Texture* t = bitmaps.getTexture("crate"))
            crate->addLayer(*t);
        if (const sf::Texture* t = bitmaps.getTexture("star"))
            crate->addLayer(*t);
        crate->setPosition({730.f, 300.f});
        crate->setScale({1.2f, 1.2f});
        m_objects.push_back(std::move(crate));

        forEachShape([&](ShapeObject& s) {
            s.setFilled(m_filled);
            s.setAlgorithm(m_algorithm);
        });
    }

    std::vector<std::unique_ptr<GameObject>> m_objects;
    bool m_spin = true;
    bool m_orbit = false;
    bool m_pulse = false;
    bool m_filled = true;
    LineAlgorithm m_algorithm = LineAlgorithm::Incremental;
    float m_pulseTime = 0.f;
    float m_pulseScale = 1.f;
};

// [5] Polygons and fills: simple-polygon check, flood / boundary fill and the
// parity scan-line fill, all on a persistent bitmap you can paint on.
// On top of the canvas: the moving shape objects (ShapeObjects above).
class PolygonFillScene : public Scene {
public:
    PolygonFillScene()
    {
        runTests();
        const sf::Vector2u size = Engine::getInstance().getWindow().getSize();
        if (!m_canvas.resize(size))
            Engine::getInstance().getLogger().error("Could not create the paint canvas");
        resetCanvas();
    }

    void onEnter() override { Engine::getInstance().setClearColor(kBackground); }

    void update(float dt) override
    {
        m_messageTime = std::max(0.f, m_messageTime - dt);
        m_shapes.update(dt); // spin / orbit / pulse the shape objects
    }

    void render(sf::RenderTarget& target) override
    {
        target.draw(sf::Sprite(m_canvas.getTexture()));

        // Moving shapes are drawn over the canvas, not into it, so the
        // flood / boundary fills (which read the canvas) never see them
        m_shapes.draw(target);

        // Labels are drawn on top, never into the canvas, so they cannot
        // change what the fills see
        drawText(target, "flood fill", {30.f, 112.f}, 13);
        drawText(target, "boundary fill", {175.f, 112.f}, 13);
        drawText(target, "parity fill (star)", {315.f, 112.f}, 13);
        drawText(target, "parity fill (concave)", {470.f, 112.f}, 13);
        drawText(target, "own circle + ellipse", {635.f, 112.f}, 13);

        // Polygon being built
        PrimitiveRenderer r(target);
        if (!m_points.empty()) {
            std::vector<Point2D> preview = m_points;
            const sf::Vector2i mouse = Engine::getInstance().getMousePosition();
            preview.emplace_back(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            r.drawPolyline(preview, sf::Color::Yellow, false, LineAlgorithm::Incremental);
            for (const Point2D& p : m_points)
                p.draw(r, sf::Color::Yellow, 4.f);
        }

        // Help bar
        r.drawRectangle({0.f, 452.f}, {800.f, 126.f}, sf::Color(0, 0, 0, 170));
        drawText(target, "LMB add vertex | Enter: polygon (rejected if segments intersect) | P: parity fill | Backspace: drop",
                 {10.f, 456.f}, 13);
        drawText(target, "F/RMB flood fill | G/MMB boundary fill (white border) | at mouse: O circle E ellipse S star Q square",
                 {10.f, 472.f}, 13);
        drawText(target, "C: next fill colour | R: reset canvas | T: re-run tests", {10.f, 488.f}, 13);
        drawText(target, m_shapes.status(), {10.f, 504.f}, 13);
        r.drawRectangle({10.f, 524.f}, {14.f, 14.f}, kFillColors[m_colorIndex]);
        drawText(target, "fill colour   " + m_testSummary, {30.f, 521.f}, 13, sf::Color(150, 200, 255));
        if (m_messageTime > 0.f)
            drawText(target, m_message, {10.f, 540.f}, 14, m_messageColor);
    }

    void onMouseButtonPressed(sf::Mouse::Button button, sf::Vector2i p) override
    {
        if (button == sf::Mouse::Button::Left)
            m_points.emplace_back(static_cast<float>(p.x), static_cast<float>(p.y));
        else if (button == sf::Mouse::Button::Right)
            fill(p, false);
        else if (button == sf::Mouse::Button::Middle)
            fill(p, true);
    }

    void onKeyPressed(sf::Keyboard::Key key) override
    {
        if (m_shapes.onKeyPressed(key)) // Space, M, X, I, Tab, Delete: shape objects
            return;

        const sf::Vector2i mouse = Engine::getInstance().getMousePosition();
        PrimitiveRenderer r(m_canvas); // everything below draws INTO the canvas bitmap
        switch (key) {
        case sf::Keyboard::Key::Enter: {
            const PolygonCheck check = PrimitiveRenderer::validatePolygon(m_points);
            if (r.drawPolygon(m_points, kOutline, LineAlgorithm::Incremental)) {
                show("Polygon drawn (" + std::to_string(m_points.size()) + " vertices)", sf::Color(150, 255, 150));
                m_points.clear();
            } else {
                show(std::string("Polygon rejected: ") + toString(check) + " - nothing was drawn",
                     sf::Color(255, 120, 120));
            }
            break;
        }
        case sf::Keyboard::Key::P:
            if (r.fillPolygon(m_points, kFillColors[m_colorIndex])) {
                show("Polygon filled with the parity rule", sf::Color(150, 255, 150));
                m_points.clear();
            } else {
                show("Need at least 3 vertices", sf::Color(255, 120, 120));
            }
            break;
        case sf::Keyboard::Key::F: fill(mouse, false); return;
        case sf::Keyboard::Key::G: fill(mouse, true); return;
        case sf::Keyboard::Key::O: r.drawCircleIncremental(sf::Vector2f(mouse), 45.f, kOutline); break;
        case sf::Keyboard::Key::E: r.drawEllipseIncremental(sf::Vector2f(mouse), 70.f, 30.f, kOutline); break;
        // Star and square outlines at the mouse, drawn like the circle and
        // ellipse (own incremental algorithm, white) so F / G can fill them
        case sf::Keyboard::Key::S:
            r.drawPolygon(starPoints(sf::Vector2f(mouse), 50.f, 22.f), kOutline, LineAlgorithm::Incremental);
            show("Star drawn - fill it with F or G", sf::Color(150, 255, 150));
            break;
        case sf::Keyboard::Key::Q:
            r.drawPolygon(squarePoints(sf::Vector2f(mouse), 40.f), kOutline, LineAlgorithm::Incremental);
            show("Square drawn - fill it with F or G", sf::Color(150, 255, 150));
            break;
        case sf::Keyboard::Key::C: m_colorIndex = (m_colorIndex + 1) % kFillColors.size(); return;
        case sf::Keyboard::Key::Backspace: m_points.clear(); return;
        case sf::Keyboard::Key::R: resetCanvas(); return;
        case sf::Keyboard::Key::T: runTests(); return;
        default: return;
        }
        m_canvas.display();
    }

private:
    static constexpr sf::Color kBackground{25, 28, 38};
    static constexpr sf::Color kOutline{255, 255, 255};
    static constexpr std::array<sf::Color, 5> kFillColors{
        sf::Color(60, 200, 90), sf::Color(240, 180, 40), sf::Color(80, 140, 255), sf::Color(230, 80, 90),
        sf::Color(180, 90, 230)};

    // 5-pointed star outline: 10 corners alternating between the outer and the
    // inner radius. Its edges never cross, so it is a valid (simple) polygon.
    // Corners are rounded to whole pixels so the outline closes exactly.
    static std::vector<Point2D> starPoints(sf::Vector2f center, float outer, float inner)
    {
        std::vector<Point2D> points;
        for (int i = 0; i < 10; ++i) {
            const float radius = (i % 2 == 0) ? outer : inner;
            const float angle = -kPi / 2.f + static_cast<float>(i) * kPi / 5.f; // first tip points up
            points.emplace_back(std::round(center.x + radius * std::cos(angle)),
                                std::round(center.y + radius * std::sin(angle)));
        }
        return points;
    }

    // Square outline: 4 corners, 'half' pixels from the centre on each side
    static std::vector<Point2D> squarePoints(sf::Vector2f center, float half)
    {
        return {{center.x - half, center.y - half}, {center.x + half, center.y - half},
                {center.x + half, center.y + half}, {center.x - half, center.y + half}};
    }

    void fill(sf::Vector2i at, bool boundary)
    {
        PrimitiveRenderer r(m_canvas);
        const sf::Color color = kFillColors[m_colorIndex];
        const unsigned pixels =
            boundary ? r.boundaryFill(at.x, at.y, color, kOutline) : r.floodFill(at.x, at.y, color);
        m_canvas.display();
        show(std::string(boundary ? "Boundary" : "Flood") + " fill: " + std::to_string(pixels) + " pixels",
             sf::Color(150, 255, 150));
    }

    // Example content showing each fill; everything is drawn with exact
    // colours (own algorithms) so the seed fills can read it reliably
    void resetCanvas()
    {
        Engine::clearBitmap(m_canvas, kBackground);
        PrimitiveRenderer r(m_canvas);
        const sf::Color blue(80, 140, 255);

        // The same picture twice: white frame with a blue square inside
        for (float x : {20.f, 165.f}) {
            r.drawPolygon(std::vector<Point2D>{{x, 10.f}, {x + 120.f, 10.f}, {x + 120.f, 100.f}, {x, 100.f}}, kOutline,
                          LineAlgorithm::Incremental);
            r.drawRectangle({x + 45.f, 35.f}, {30.f, 30.f}, blue);
        }
        r.floodFill(30, 20, sf::Color(60, 200, 90));                   // stops at the blue square
        r.boundaryFill(175, 20, sf::Color(60, 200, 90), kOutline);     // covers the blue square

        // Self-intersecting star: parity leaves the middle empty
        r.fillPolygon({{375.f, 8.f}, {410.f, 102.f}, {320.f, 42.f}, {430.f, 42.f}, {340.f, 102.f}},
                      sf::Color(240, 180, 40));
        // Concave polygon
        r.fillPolygon({{470.f, 10.f}, {610.f, 10.f}, {610.f, 100.f}, {580.f, 100.f}, {580.f, 40.f}, {540.f, 80.f},
                       {500.f, 40.f}, {500.f, 100.f}, {470.f, 100.f}},
                      sf::Color(230, 80, 90));
        // Outlines to fill yourself
        r.drawCircleIncremental({680.f, 55.f}, 45.f, kOutline);
        r.drawEllipseIncremental({720.f, 70.f}, 60.f, 25.f, kOutline);

        // A polygon from LineSegment objects (closed chain)
        r.drawPolygon(std::vector<LineSegment>{{60.f, 200.f, 200.f, 160.f}, {200.f, 160.f, 260.f, 300.f},
                                               {260.f, 300.f, 120.f, 340.f}, {120.f, 340.f, 60.f, 200.f}},
                      kOutline, LineAlgorithm::Incremental);
        m_canvas.display();
        m_points.clear();
    }

    void runTests()
    {
        const std::vector<RasterTestResult> results = testPolygonsAndFills();
        Engine::getInstance().getLogger().info("Polygon / fill algorithm tests:");
        logRasterTests(Engine::getInstance().getLogger(), results);
        unsigned passed = 0;
        for (const RasterTestResult& t : results)
            passed += t.passed;
        m_testSummary = "Tests: " + std::to_string(passed) + "/" + std::to_string(results.size()) +
                        " passed (details in engine_log.txt)";
    }

    void show(const std::string& message, sf::Color color)
    {
        m_message = message;
        m_messageColor = color;
        m_messageTime = 4.f;
    }

    sf::RenderTexture m_canvas;
    ShapeObjects m_shapes; // the moving shape objects drawn over the canvas
    std::vector<Point2D> m_points;
    std::size_t m_colorIndex = 0;
    std::string m_testSummary;
    std::string m_message;
    sf::Color m_messageColor;
    float m_messageTime = 0.f;
};

// [6] Player: only the sprite-animated Player (UpdatableObject + SpriteObject +
// InputListener). The moving shape objects that used to be here are in scene 5.
//   WASD / arrows walk, LMB / Space shoot towards the mouse, wheel resizes,
//   Backspace resets the player, T re-runs the transformation / bitmap tests
class PlayerScene : public Scene {
public:
    PlayerScene()
    {
        reset();
        runTests();
    }

    void onEnter() override { Engine::getInstance().setClearColor(sf::Color(18, 22, 30)); }

    void update(float dt) override
    {
        m_player->setBounds(sf::Vector2f(Engine::getInstance().getWindow().getSize())); // stay in the window
        m_player->update(dt);  // UpdatableObject: walking, shooting, moving the bullets
        m_player->animate(dt); // ANIMATION: AnimatedObject - next walk frame when it is time
    }

    void render(sf::RenderTarget& target) override
    {
        PrimitiveRenderer r(target);
        m_player->draw(r); // DrawableObject: bullets, shadow layer, sprite layer

        // Help bar
        r.drawRectangle({0.f, 500.f}, {800.f, 78.f}, sf::Color(0, 0, 0, 170));
        drawText(target, "Player (sprites): WASD/arrows walk | LMB/Space shoot towards the mouse | wheel resize",
                 {10.f, 506.f}, 13);
        drawText(target, "Backspace: reset player | T: re-run tests", {10.f, 524.f}, 13);
        drawText(target, std::string("Facing ") + Player::toString(m_player->getDirection()) + ", frame " +
                             std::to_string(m_player->getFrame()) + (m_player->isPlaying() ? " (walking)" : " (idle)") +
                             ", size x" + std::to_string(m_player->getSizeFactor()).substr(0, 4) + ", bullets " +
                             std::to_string(m_player->getBullets().size()) + "     " + m_testSummary,
                 {10.f, 542.f}, 13, sf::Color(150, 200, 255));
    }

    // Input goes to the player (it is an InputListener) unless the scene uses it
    void onKeyPressed(sf::Keyboard::Key key) override
    {
        switch (key) {
        case sf::Keyboard::Key::Backspace: reset(); return;
        case sf::Keyboard::Key::T: runTests(); return;
        default: m_player->onKeyPressed(key);
        }
    }
    void onMouseButtonPressed(sf::Mouse::Button b, sf::Vector2i p) override { m_player->onMouseButtonPressed(b, p); }
    void onMouseWheelScrolled(float d, sf::Vector2i p) override { m_player->onMouseWheelScrolled(d, p); }

private:
    // A fresh player in the middle of the window (normal size, no bullets)
    void reset() { m_player = std::make_unique<Player>(sf::Vector2f(400.f, 280.f)); }

    void runTests()
    {
        std::vector<RasterTestResult> results = testTransformations();
        const std::vector<RasterTestResult> bitmapResults = testBitmaps();
        results.insert(results.end(), bitmapResults.begin(), bitmapResults.end());
        Engine::getInstance().getLogger().info("Transformation / hierarchy / bitmap tests:");
        logRasterTests(Engine::getInstance().getLogger(), results);
        unsigned passed = 0;
        for (const RasterTestResult& t : results)
            passed += t.passed;
        m_testSummary = "Tests: " + std::to_string(passed) + "/" + std::to_string(results.size()) + " passed";
    }

    std::unique_ptr<Player> m_player;
    std::string m_testSummary;
};

// Owns the scenes, forwards input to the active one, handles global keys
class SceneManager : public InputListener {
public:
    SceneManager()
    {
        m_scenes.push_back(std::make_unique<MovementScene>());
        m_scenes.push_back(std::make_unique<LineComparisonScene>());
        m_scenes.push_back(std::make_unique<ShapesScene>());
        m_scenes.push_back(std::make_unique<CirclesScene>());
        m_scenes.push_back(std::make_unique<PolygonFillScene>());
        m_scenes.push_back(std::make_unique<PlayerScene>());
        m_scenes[m_current]->onEnter();
    }

    void update(float dt) { m_scenes[m_current]->update(dt); }

    void render(sf::RenderTarget& target)
    {
        m_scenes[m_current]->render(target);
        Engine& engine = Engine::getInstance();
        const float bottom = static_cast<float>(target.getSize().y) - 22.f;
        drawText(target, std::string("[1] Move [2] Lines [3] Shapes [4] Circles [5] Fill [6] Player  B: ") +
                         (engine.getBuffering() == BufferingMode::Triple ? "triple" : "double") +
                         " buffer  F12 shot  F11 full  Esc quit",
                 {10.f, bottom}, 13, sf::Color(130, 170, 130));
    }

    void onKeyPressed(sf::Keyboard::Key key) override
    {
        Engine& engine = Engine::getInstance();
        switch (key) {
        case sf::Keyboard::Key::Escape: engine.stop(); return;
        case sf::Keyboard::Key::F11: engine.toggleFullscreen(); return;
        case sf::Keyboard::Key::F12:
            engine.saveScreenshot("screenshot_" + std::to_string(++m_screenshotCount) + ".png");
            return;
        case sf::Keyboard::Key::B:
            engine.setBuffering(engine.getBuffering() == BufferingMode::Triple ? BufferingMode::Double
                                                                               : BufferingMode::Triple);
            return;
        case sf::Keyboard::Key::Num1: switchTo(0); return;
        case sf::Keyboard::Key::Num2: switchTo(1); return;
        case sf::Keyboard::Key::Num3: switchTo(2); return;
        case sf::Keyboard::Key::Num4: switchTo(3); return;
        case sf::Keyboard::Key::Num5: switchTo(4); return;
        case sf::Keyboard::Key::Num6: switchTo(5); return;
        default: m_scenes[m_current]->onKeyPressed(key);
        }
    }

    void onKeyReleased(sf::Keyboard::Key key) override { m_scenes[m_current]->onKeyReleased(key); }
    void onMouseButtonPressed(sf::Mouse::Button b, sf::Vector2i p) override { m_scenes[m_current]->onMouseButtonPressed(b, p); }
    void onMouseButtonReleased(sf::Mouse::Button b, sf::Vector2i p) override { m_scenes[m_current]->onMouseButtonReleased(b, p); }
    void onMouseMoved(sf::Vector2i p) override { m_scenes[m_current]->onMouseMoved(p); }
    void onMouseWheelScrolled(float d, sf::Vector2i p) override { m_scenes[m_current]->onMouseWheelScrolled(d, p); }

    // Renders every scene off-screen and saves it as scene_N.png
    void saveAllScenes(sf::Vector2u size)
    {
        sf::RenderTexture canvas;
        if (!canvas.resize(size))
            return;
        for (std::size_t i = 0; i < m_scenes.size(); ++i) {
            switchTo(i);
            m_scenes[i]->update(0.f);
            Engine::clearBitmap(canvas, Engine::getInstance().getConfig().clearColor);
            render(canvas);
            canvas.display();
            const std::string path = "scene_" + std::to_string(i + 1) + ".png";
            if (canvas.getTexture().copyToImage().saveToFile(path))
                Engine::getInstance().getLogger().info("Saved " + path);
        }
        switchTo(0);
    }

private:
    void switchTo(std::size_t index)
    {
        if (index < m_scenes.size() && index != m_current) {
            m_current = index;
            m_scenes[m_current]->onEnter();
        }
    }

    std::vector<std::unique_ptr<Scene>> m_scenes;
    std::size_t m_current = 0;
    unsigned m_screenshotCount = 0;
};

} // namespace

int main(int argc, char* argv[])
{
    const bool screenshotMode = argc > 1 && std::string(argv[1]) == "--screenshots";
    Engine& engine = Engine::getInstance();

    engine.setTitle("SFML 2D Engine")
          .setResolution(800, 600)
          .setFullscreen(false)
          .setResizable(true)
          .setFPS(60)
          .setVSync(false)
          .setBuffering(BufferingMode::Triple)   // or BufferingMode::Double
          .enableKeyboard(true)
          .enableMouse(true)
          .setClearColor(sf::Color(30, 30, 40))
          .setLogFile("engine_log.txt");

    // Resolution selection example: engine.selectVideoMode(0) picks the best
    // fullscreen mode reported by Engine::getAvailableModes().

    if (!engine.init()) {
        // init() already logged the reason and showed an error dialog
        return EXIT_FAILURE;
    }

    try {
        loadFont();
        SceneManager scenes;
        if (screenshotMode) {
            scenes.saveAllScenes(engine.getWindow().getSize());
            engine.shutdown();
            return EXIT_SUCCESS;
        }
        engine.addInputListener(&scenes);
        engine.setUpdateCallback([&](float dt) { scenes.update(dt); });
        engine.setRenderCallback([&](sf::RenderTarget& target) { scenes.render(target); });

        engine.run();

        engine.removeInputListener(&scenes);
        // Callbacks capture 'scenes' by reference - drop them before it dies
        engine.setUpdateCallback(nullptr);
        engine.setRenderCallback(nullptr);
    }
    catch (const std::exception& ex) {
        engine.reportError(std::string("Unhandled exception: ") + ex.what(), true);
        engine.shutdown();
        return EXIT_FAILURE;
    }

    engine.shutdown();
    return EXIT_SUCCESS;
}
