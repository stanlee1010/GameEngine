#include "DemoAssets.h"

#include "BitmapHandler.h"
#include "PrimitiveRenderer.h"

#include <cmath>
#include <functional>

namespace {

constexpr float kPi = 3.14159265358979f;

enum class Facing { Right, Up, Down };

const sf::Color kOutline(30, 34, 48);
const sf::Color kBody(226, 236, 246);
const sf::Color kBodyShade(176, 198, 222);
const sf::Color kVisor(70, 200, 255);
const sf::Color kVisorShine(200, 245, 255);
const sf::Color kAntennaTip(255, 90, 90);

// Renders with the PrimitiveRenderer into a transparent bitmap and stores it
bool renderBitmap(BitmapHandler& bitmaps, const std::string& name, sf::Vector2u size,
                  const std::function<void(PrimitiveRenderer&)>& draw)
{
    sf::RenderTexture canvas;
    if (!canvas.resize(size))
        return false;
    canvas.clear(sf::Color::Transparent);
    PrimitiveRenderer renderer(canvas);
    draw(renderer);
    canvas.display();
    return bitmaps.create(name, canvas.getTexture().copyToImage());
}

void blob(PrimitiveRenderer& r, sf::Vector2f c, float rx, float ry, sf::Color fill)
{
    r.drawEllipse(c, rx, ry, fill, true);
    r.drawEllipseIncremental(c, rx, ry, kOutline);
}

// One frame of the little robot. frame 0..3 = walk cycle:
// 0 stand, 1 left foot forward, 2 stand, 3 right foot forward
void drawRobot(PrimitiveRenderer& r, sf::Vector2f cell, float size, Facing facing, int frame)
{
    const float u = size / 48.f; // the design is made for 48 px cells
    const float step = frame == 1 ? 1.f : frame == 3 ? -1.f : 0.f;
    const float bob = (frame % 2 == 1) ? -1.f * u : 0.f;
    const float cx = cell.x + 24.f * u;
    const auto at = [&](float x, float y) { return sf::Vector2f(cx + x * u, cell.y + y * u + bob); };
    const auto ground = [&](float x, float y) { return sf::Vector2f(cx + x * u, cell.y + y * u); };

    // Feet
    if (facing == Facing::Right) {
        blob(r, ground(-4.f - 5.f * step, 42.f), 5.f * u, 3.f * u, kBodyShade);
        blob(r, ground(4.f + 5.f * step, 42.f), 5.f * u, 3.f * u, kBody);
    } else {
        blob(r, ground(-7.f, 42.f - (step > 0.f ? 3.f : 0.f)), 5.f * u, 3.f * u, kBody);
        blob(r, ground(7.f, 42.f - (step < 0.f ? 3.f : 0.f)), 5.f * u, 3.f * u, kBody);
    }

    // Arms swing opposite to the feet
    if (facing == Facing::Right) {
        blob(r, at(3.f - 5.f * step, 30.f), 3.5f * u, 3.5f * u, kBodyShade);
    } else {
        blob(r, at(-13.f, 29.f + 2.f * step), 3.5f * u, 3.5f * u, kBodyShade);
        blob(r, at(13.f, 29.f - 2.f * step), 3.5f * u, 3.5f * u, kBodyShade);
    }

    // Body and head
    blob(r, at(0.f, 31.f), 11.f * u, 10.f * u, kBody);
    r.drawEllipse(at(-3.f, 34.f), 5.f * u, 3.f * u, kBodyShade, true); // belly shading
    blob(r, at(0.f, 15.f), 12.f * u, 10.f * u, kBody);

    // Antenna
    r.drawLineIncremental(at(0.f, 5.f), at(facing == Facing::Right ? -3.f : 0.f, 1.f), kOutline);
    blob(r, at(facing == Facing::Right ? -3.f : 0.f, 1.f), 2.f * u, 2.f * u, kAntennaTip);

    // Face: where the visor is shows the direction
    switch (facing) {
    case Facing::Down:
        r.fillPolygon({Point2D(at(-9.f, 12.f)), Point2D(at(9.f, 12.f)), Point2D(at(8.f, 19.f)), Point2D(at(-8.f, 19.f))},
                      kVisor);
        r.drawPolygon(std::vector<Point2D>{Point2D(at(-9.f, 12.f)), Point2D(at(9.f, 12.f)), Point2D(at(8.f, 19.f)),
                                           Point2D(at(-8.f, 19.f))},
                      kOutline, LineAlgorithm::Incremental);
        r.drawLineIncremental(at(-6.f, 14.f), at(-2.f, 14.f), kVisorShine);
        break;
    case Facing::Right:
        r.fillPolygon({Point2D(at(2.f, 12.f)), Point2D(at(12.f, 12.f)), Point2D(at(11.f, 19.f)), Point2D(at(2.f, 19.f))},
                      kVisor);
        r.drawPolygon(std::vector<Point2D>{Point2D(at(2.f, 12.f)), Point2D(at(12.f, 12.f)), Point2D(at(11.f, 19.f)),
                                           Point2D(at(2.f, 19.f))},
                      kOutline, LineAlgorithm::Incremental);
        r.drawLineIncremental(at(4.f, 14.f), at(8.f, 14.f), kVisorShine);
        break;
    case Facing::Up:
        // Back of the head: a hatch instead of a visor
        r.drawEllipse(at(0.f, 15.f), 4.f * u, 4.f * u, kBodyShade, true);
        r.drawCircleIncremental(at(0.f, 15.f), 4.f * u, kOutline);
        break;
    }
}

} // namespace

namespace DemoAssets {

bool createPlayerSheet(BitmapHandler& bitmaps, const std::string& name, unsigned cellSize)
{
    const float cell = static_cast<float>(cellSize);
    const bool ok = renderBitmap(bitmaps, name, {cellSize * 4, cellSize * 4}, [&](PrimitiveRenderer& r) {
        for (int frame = 0; frame < 4; ++frame) {
            drawRobot(r, {frame * cell, 0.f * cell}, cell, Facing::Right, frame);
            drawRobot(r, {frame * cell, 2.f * cell}, cell, Facing::Up, frame);
            drawRobot(r, {frame * cell, 3.f * cell}, cell, Facing::Down, frame);
        }
    });
    if (!ok)
        return false;

    // BITMAP MANIPULATION in practice: extract + flipHorizontally + copyRegion
    // Left row = right row mirrored. Each frame is mirrored in place so the
    // walk cycle keeps its order: extract -> flip -> copy into row 1
    const int c = static_cast<int>(cellSize);
    for (int frame = 0; frame < 4; ++frame) {
        const std::string temp = name + "#mirror";
        if (!bitmaps.extract(name, sf::IntRect({frame * c, 0}, {c, c}), temp) || !bitmaps.flipHorizontally(temp) ||
            !bitmaps.copyRegion(temp, {}, name, {static_cast<unsigned>(frame * c), cellSize}))
            return false;
        bitmaps.remove(temp);
    }
    return true;
}

bool createShadow(BitmapHandler& bitmaps, const std::string& name, sf::Vector2u size)
{
    return renderBitmap(bitmaps, name, size, [&](PrimitiveRenderer& r) {
        const sf::Vector2f c(static_cast<float>(size.x) / 2.f - 0.5f, static_cast<float>(size.y) / 2.f - 0.5f);
        r.drawEllipse(c, c.x, c.y, sf::Color(0, 0, 0, 70), true);
        r.drawEllipse(c, c.x * 0.6f, c.y * 0.6f, sf::Color(0, 0, 0, 50), true);
    });
}

bool createCrate(BitmapHandler& bitmaps, const std::string& name, unsigned size)
{
    return renderBitmap(bitmaps, name, {size, size}, [&](PrimitiveRenderer& r) {
        const float s = static_cast<float>(size) - 1.f;
        const sf::Color wood(170, 112, 60), dark(100, 62, 30), light(205, 150, 90);
        r.drawRectangle({0.f, 0.f}, {s + 1.f, s + 1.f}, wood);
        r.drawRectangle({0.f, 0.f}, {s + 1.f, s + 1.f}, dark, false);
        r.drawRectangle({4.f, 4.f}, {s - 7.f, s - 7.f}, dark, false);
        for (float y = 10.f; y < s - 4.f; y += 8.f)
            r.drawLineIncremental({5.f, y}, {s - 5.f, y}, light);
        r.drawLineIncremental({4.f, 4.f}, {s - 4.f, s - 4.f}, dark);
        r.drawLineIncremental({5.f, 4.f}, {s - 4.f, s - 5.f}, dark);
    });
}

bool createStar(BitmapHandler& bitmaps, const std::string& name, unsigned size)
{
    return renderBitmap(bitmaps, name, {size, size}, [&](PrimitiveRenderer& r) {
        const float c = static_cast<float>(size) / 2.f - 0.5f;
        std::vector<Point2D> star;
        for (int i = 0; i < 10; ++i) {
            const float radius = (i % 2 == 0) ? c : c * 0.45f;
            const float a = -kPi / 2.f + static_cast<float>(i) * kPi / 5.f;
            star.emplace_back(c + radius * std::cos(a), c + radius * std::sin(a));
        }
        r.fillPolygon(star, sf::Color(255, 215, 60));
        r.drawPolygon(star, sf::Color(150, 100, 0), LineAlgorithm::Incremental);
    });
}

} // namespace DemoAssets
