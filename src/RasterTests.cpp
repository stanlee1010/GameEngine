#include "RasterTests.h"

#include "Logger.h"
#include "BitmapHandler.h"
#include "DemoAssets.h"
#include "Player.h"
#include "PrimitiveRenderer.h"
#include "Shapes.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <functional>

namespace {

constexpr unsigned kSize = 160;
constexpr int kCenter = 80;
const sf::Color kRed = sf::Color::Red;
const sf::Color kGreen = sf::Color::Green;
const sf::Color kBlue = sf::Color::Blue;

// Draws into a fresh black canvas and returns its pixels
sf::Image render(sf::RenderTexture& canvas, const std::function<void(PrimitiveRenderer&)>& draw)
{
    canvas.clear(sf::Color::Black);
    PrimitiveRenderer renderer(canvas);
    draw(renderer);
    canvas.display();
    return canvas.getTexture().copyToImage();
}

bool inside(int x, int y)
{
    return x >= 0 && y >= 0 && x < static_cast<int>(kSize) && y < static_cast<int>(kSize);
}

bool lit(const sf::Image& image, int x, int y)
{
    if (!inside(x, y))
        return false;
    const sf::Color c = image.getPixel({static_cast<unsigned>(x), static_cast<unsigned>(y)});
    return c.r > 127 || c.g > 127 || c.b > 127;
}

unsigned countColor(const sf::Image& image, sf::Color color)
{
    unsigned count = 0;
    for (unsigned y = 0; y < kSize; ++y)
        for (unsigned x = 0; x < kSize; ++x)
            count += image.getPixel({x, y}) == color;
    return count;
}

unsigned countLit(const sf::Image& image)
{
    unsigned count = 0;
    for (int y = 0; y < static_cast<int>(kSize); ++y)
        for (int x = 0; x < static_cast<int>(kSize); ++x)
            count += lit(image, x, y);
    return count;
}

std::string format(const char* fmt, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return buffer;
}

// Distance (px) of pixel offset (dx, dy) from the ellipse x^2/a^2 + y^2/b^2 = 1,
// first-order estimate |F| / |grad F|
float curveError(int dx, int dy, float a, float b)
{
    const float x = static_cast<float>(dx), y = static_cast<float>(dy);
    const float f = x * x / (a * a) + y * y / (b * b) - 1.f;
    const float gx = 2.f * x / (a * a), gy = 2.f * y / (b * b);
    const float grad = std::sqrt(gx * gx + gy * gy);
    return grad > 0.f ? std::fabs(f) / grad : 0.f;
}

RasterTestResult curveTest(sf::RenderTexture& canvas, bool circle, float rx, float ry)
{
    const sf::Vector2f c{static_cast<float>(kCenter), static_cast<float>(kCenter)};
    const sf::Image own = render(canvas, [&](PrimitiveRenderer& r) {
        if (circle) r.drawCircleIncremental(c, rx, sf::Color::White);
        else        r.drawEllipseIncremental(c, rx, ry, sf::Color::White);
    });
    const sf::Image lib = render(canvas, [&](PrimitiveRenderer& r) {
        if (circle) r.drawCircle(c, rx, sf::Color::White, false);
        else        r.drawEllipse(c, rx, ry, sf::Color::White, false);
    });

    unsigned ownCount = 0, libCount = 0, common = 0;
    bool closed = true, symmetric = true;
    float ownErr = 0.f, libErr = 0.f;
    for (int y = 0; y < static_cast<int>(kSize); ++y) {
        for (int x = 0; x < static_cast<int>(kSize); ++x) {
            const int dx = x - kCenter, dy = y - kCenter;
            const bool o = lit(own, x, y), l = lit(lib, x, y);
            libCount += l;
            common += o && l;
            if (l)
                libErr = std::max(libErr, curveError(dx, dy, rx, ry));
            if (!o)
                continue;
            ++ownCount;
            ownErr = std::max(ownErr, curveError(dx, dy, rx, ry));

            // A closed curve: every pixel touches at least two others (8-neighbourhood)
            int neighbours = 0;
            for (int ny = -1; ny <= 1; ++ny)
                for (int nx = -1; nx <= 1; ++nx)
                    neighbours += (nx || ny) && lit(own, x + nx, y + ny);
            closed = closed && neighbours >= 2;

            // 4-fold symmetry (mirror in both axes), circles also 8-fold (swap x/y)
            symmetric = symmetric && lit(own, kCenter - dx, y) && lit(own, x, kCenter - dy);
            if (circle)
                symmetric = symmetric && lit(own, kCenter + dy, kCenter + dx);
        }
    }

    RasterTestResult result;
    result.group = circle ? "Circle" : "Ellipse";
    result.name = circle ? format("R=%g", rx) : format("Rx=%g Ry=%g", rx, ry);
    const unsigned total = std::max(ownCount, libCount);
    result.details = format("own %u px, library %u px, common %u (%.0f%%), max error own %.2f / library %.2f px%s%s",
                            ownCount, libCount, common, total ? 100.f * common / total : 100.f, ownErr, libErr,
                            closed ? "" : ", GAPS", symmetric ? "" : ", NOT SYMMETRIC");
    result.passed = closed && symmetric && ownErr <= 1.f;
    return result;
}

RasterTestResult check(const char* group, std::string name, bool passed, std::string details)
{
    return {group, std::move(name), std::move(details), passed};
}

} // namespace

std::vector<RasterTestResult> testCirclesAndEllipses()
{
    std::vector<RasterTestResult> results;
    sf::RenderTexture canvas;
    if (!canvas.resize({kSize, kSize}))
        return results;

    for (float r : {2.f, 5.f, 10.f, 25.f, 45.f, 70.f})
        results.push_back(curveTest(canvas, true, r, r));
    const float ellipses[][2] = {{70.f, 25.f}, {20.f, 60.f}, {40.f, 40.f}, {65.f, 6.f}, {3.f, 30.f}};
    for (const auto& e : ellipses)
        results.push_back(curveTest(canvas, false, e[0], e[1]));
    return results;
}

std::vector<RasterTestResult> testPolygonsAndFills()
{
    std::vector<RasterTestResult> results;
    sf::RenderTexture canvas;
    if (!canvas.resize({kSize, kSize}))
        return results;

    // --- Polygon validation ---------------------------------------------
    using P = std::vector<Point2D>;
    using S = std::vector<LineSegment>;
    struct PointCase { const char* name; P points; PolygonCheck expected; };
    const PointCase pointCases[] = {
        {"triangle",                 {{10, 10}, {60, 10}, {30, 50}},                         PolygonCheck::Ok},
        {"concave arrow",            {{10, 10}, {60, 30}, {10, 50}, {25, 30}},               PolygonCheck::Ok},
        {"bow-tie (crossing)",       {{10, 10}, {60, 60}, {60, 10}, {10, 60}},               PolygonCheck::SelfIntersecting},
        {"pentagram (crossing)",     {{50, 5}, {79, 95}, {2, 40}, {98, 40}, {21, 95}},       PolygonCheck::SelfIntersecting},
        {"folds back on itself",     {{10, 10}, {60, 10}, {40, 10}, {40, 50}},               PolygonCheck::SelfIntersecting},
        {"vertex touches an edge",   {{10, 10}, {60, 10}, {60, 60}, {35, 10}, {10, 60}},     PolygonCheck::SelfIntersecting},
        {"only two points",          {{10, 10}, {60, 60}},                                   PolygonCheck::TooFewPoints},
    };
    for (const PointCase& c : pointCases) {
        const PolygonCheck got = PrimitiveRenderer::validatePolygon(c.points);
        results.push_back(check("Polygon", std::string("points: ") + c.name, got == c.expected,
                                format("expected '%s', got '%s'", toString(c.expected), toString(got))));
    }

    struct SegmentCase { const char* name; S segments; PolygonCheck expected; };
    const SegmentCase segmentCases[] = {
        {"square", {{10, 10, 60, 10}, {60, 10, 60, 60}, {60, 60, 10, 60}, {10, 60, 10, 10}}, PolygonCheck::Ok},
        {"square, shuffled + reversed",
         {{60, 60, 60, 10}, {10, 10, 10, 60}, {60, 10, 10, 10}, {10, 60, 60, 60}}, PolygonCheck::Ok},
        {"bow-tie", {{10, 10, 60, 60}, {60, 60, 60, 10}, {60, 10, 10, 60}, {10, 60, 10, 10}},
         PolygonCheck::SelfIntersecting},
        {"open chain", {{10, 10, 60, 10}, {60, 10, 60, 60}, {60, 60, 10, 60}}, PolygonCheck::NotClosed},
        {"two separate triangles",
         {{0, 0, 10, 0}, {10, 0, 0, 10}, {0, 10, 0, 0}, {50, 50, 60, 50}, {60, 50, 50, 60}, {50, 60, 50, 50}},
         PolygonCheck::NotClosed},
    };
    for (const SegmentCase& c : segmentCases) {
        const PolygonCheck got = PrimitiveRenderer::validatePolygon(c.segments);
        results.push_back(check("Polygon", std::string("segments: ") + c.name, got == c.expected,
                                format("expected '%s', got '%s'", toString(c.expected), toString(got))));
    }

    // A rejected polygon must not draw a single pixel
    for (LineAlgorithm algorithm : {LineAlgorithm::Default, LineAlgorithm::Incremental}) {
        bool returned = true;
        const sf::Image image = render(canvas, [&](PrimitiveRenderer& r) {
            returned = r.drawPolygon(pointCases[2].points, sf::Color::White, algorithm);
        });
        const unsigned pixels = countLit(image);
        results.push_back(check("Polygon",
                                std::string("bow-tie draws nothing (") +
                                    (algorithm == LineAlgorithm::Default ? "library" : "incremental") + ")",
                                !returned && pixels == 0,
                                format("drawPolygon returned %s, %u pixels drawn", returned ? "true" : "false",
                                       pixels)));
    }

    // --- Seed fills ------------------------------------------------------
    const P rect = {{10, 10}, {50, 10}, {50, 40}, {10, 40}};
    {
        unsigned filled = 0;
        const sf::Image image = render(canvas, [&](PrimitiveRenderer& r) {
            r.drawPolyline(rect, kRed, true, LineAlgorithm::Incremental);
            filled = r.floodFill(30, 25, kGreen);
        });
        const unsigned green = countColor(image, kGreen);
        const unsigned expected = 39 * 29; // interior of the 41 x 31 outline
        results.push_back(check("Fill", "flood fill inside rectangle", green == expected && filled == expected,
                                format("%u pixels filled, expected %u (interior only, outline kept)", green,
                                       expected)));
    }
    {
        const sf::Image image = render(canvas, [&](PrimitiveRenderer& r) {
            r.drawCircleIncremental({80.f, 80.f}, 30.f, kRed);
            r.floodFill(80, 80, kGreen);
        });
        unsigned green = 0, leaked = 0, missed = 0;
        for (int y = 0; y < static_cast<int>(kSize); ++y)
            for (int x = 0; x < static_cast<int>(kSize); ++x) {
                const float d = std::hypot(static_cast<float>(x - 80), static_cast<float>(y - 80));
                const bool g = image.getPixel({static_cast<unsigned>(x), static_cast<unsigned>(y)}) == kGreen;
                green += g;
                leaked += g && d >= 30.f;
                missed += !g && d <= 28.5f;
            }
        results.push_back(check("Fill", "flood fill inside circle R=30", leaked == 0 && missed == 0,
                                format("%u pixels filled (pi*R^2 = 2827), %u leaked outside, %u missed inside",
                                       green, leaked, missed)));
    }
    const auto boxWithBlueSquare = [&](PrimitiveRenderer& r) {
        r.drawPolyline({{10, 10}, {70, 10}, {70, 50}, {10, 50}}, kRed, true, LineAlgorithm::Incremental);
        r.drawRectangle({30.f, 20.f}, {10.f, 10.f}, kBlue); // a different colour inside
    };
    {
        const sf::Image image = render(canvas, [&](PrimitiveRenderer& r) {
            boxWithBlueSquare(r);
            r.floodFill(15, 15, kGreen);
        });
        const unsigned blue = countColor(image, kBlue), green = countColor(image, kGreen);
        results.push_back(check("Fill", "flood fill stops at other colours", blue == 100 && green == 59 * 39 - 100,
                                format("blue square kept: %u/100 px, %u px filled", blue, green)));
    }
    {
        const sf::Image image = render(canvas, [&](PrimitiveRenderer& r) {
            boxWithBlueSquare(r);
            r.boundaryFill(15, 15, kGreen, kRed);
        });
        const unsigned blue = countColor(image, kBlue), green = countColor(image, kGreen);
        results.push_back(check("Fill", "boundary fill covers other colours", blue == 0 && green == 59 * 39,
                                format("blue left: %u px, %u px filled (whole interior up to the red border)", blue,
                                       green)));
    }
    {
        unsigned filled = 1;
        render(canvas, [&](PrimitiveRenderer& r) {
            r.drawPolyline(rect, kRed, true, LineAlgorithm::Incremental);
            filled = r.boundaryFill(10, 10, kGreen, kRed);
        });
        results.push_back(check("Fill", "seed on the boundary fills nothing", filled == 0,
                                format("%u pixels filled", filled)));
    }
    {
        // A diagonal (8-connected) outline must not let a 4-connected fill through
        const sf::Image image = render(canvas, [&](PrimitiveRenderer& r) {
            r.drawPolyline({{80, 10}, {130, 60}, {80, 110}, {30, 60}}, kRed, true, LineAlgorithm::Incremental);
            r.floodFill(80, 60, kGreen);
        });
        const bool outsideClean = image.getPixel({5, 5}) == sf::Color::Black &&
                                  image.getPixel({150, 150}) == sf::Color::Black;
        results.push_back(check("Fill", "flood fill inside diagonal diamond", outsideClean,
                                format("%u px filled, outside %s", countColor(image, kGreen),
                                       outsideClean ? "untouched" : "LEAKED")));
    }

    // --- Parity scan-line fill -----------------------------------------
    const auto compareWithLibrary = [&](const char* name, const P& polygon) {
        const sf::Image own = render(canvas, [&](PrimitiveRenderer& r) { r.fillPolygon(polygon, sf::Color::White); });
        const sf::Image lib = render(canvas, [&](PrimitiveRenderer& r) { r.fillConvexPolygon(polygon, sf::Color::White); });
        unsigned o = 0, l = 0, both = 0;
        for (int y = 0; y < static_cast<int>(kSize); ++y)
            for (int x = 0; x < static_cast<int>(kSize); ++x) {
                o += lit(own, x, y);
                l += lit(lib, x, y);
                both += lit(own, x, y) && lit(lib, x, y);
            }
        const float match = 100.f * static_cast<float>(both) / static_cast<float>(std::max(o, l));
        results.push_back(check("Scanline", name, match >= 98.f,
                                format("own %u px, library %u px, common %u (%.1f%%)", o, l, both, match)));
    };
    compareWithLibrary("rectangle 40x30 vs library", rect);
    compareWithLibrary("convex hexagon vs library",
                       {{80, 20}, {130, 50}, {130, 110}, {80, 140}, {30, 110}, {30, 50}});
    compareWithLibrary("triangle vs library", {{20, 130}, {80, 15}, {145, 100}});
    {
        const P lShape = {{10, 10}, {40, 10}, {40, 20}, {20, 20}, {20, 50}, {10, 50}};
        const sf::Image image = render(canvas, [&](PrimitiveRenderer& r) { r.fillPolygon(lShape, sf::Color::White); });
        const unsigned pixels = countLit(image);
        results.push_back(check("Scanline", "concave L-shape (area 600)", pixels == 600,
                                format("%u px filled, expected 600 (library cannot fill concave shapes)", pixels)));
    }
    {
        const P star = {{80, 5}, {124, 140}, {9, 56}, {151, 56}, {36, 140}};
        const sf::Image image = render(canvas, [&](PrimitiveRenderer& r) { r.fillPolygon(star, sf::Color::White); });
        const bool centerEmpty = !lit(image, 80, 80);
        const bool tipFilled = lit(image, 80, 30);
        results.push_back(check("Scanline", "self-intersecting pentagram (parity)", centerEmpty && tipFilled,
                                format("tip %s, center pentagon %s (even-odd rule leaves it empty)",
                                       tipFilled ? "filled" : "EMPTY", centerEmpty ? "empty" : "FILLED")));
    }
    return results;
}

namespace {

bool isNear(sf::Vector2f a, sf::Vector2f b, float eps = 1e-3f)
{
    return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps;
}

RasterTestResult pointCheck(const char* name, sf::Vector2f got, sf::Vector2f expected)
{
    return check("Transform", name, isNear(got, expected),
                 format("got (%.3f, %.3f), expected (%.3f, %.3f)", got.x, got.y, expected.x, expected.y));
}

} // namespace

std::vector<RasterTestResult> testTransformations()
{
    std::vector<RasterTestResult> results;

    Point2D p(10.f, 20.f);
    p.translate(5.f, -3.f);
    results.push_back(pointCheck("Point2D translate (5, -3)", p.getPosition(), {15.f, 17.f}));

    p.set(10.f, 0.f);
    p.rotate(90.f, {0.f, 0.f});
    results.push_back(pointCheck("Point2D rotate 90 deg about origin", p.getPosition(), {0.f, 10.f}));

    p.set(5.f, 5.f);
    p.rotate(180.f, {3.f, 3.f});
    results.push_back(pointCheck("Point2D rotate 180 deg about (3, 3)", p.getPosition(), {1.f, 1.f}));

    p.set(4.f, 6.f);
    p.scale(2.f, 0.5f, {2.f, 2.f});
    results.push_back(pointCheck("Point2D scale (2, 0.5) about (2, 2)", p.getPosition(), {6.f, 4.f}));

    {
        LineSegment s(10.f, 20.f, 50.f, 80.f);
        const sf::Vector2f pivot{100.f, 100.f};
        for (int i = 0; i < 360; ++i)
            s.rotate(1.f, pivot);
        const bool back = isNear(s.getStart().getPosition(), {10.f, 20.f}, 0.01f) &&
                          isNear(s.getEnd().getPosition(), {50.f, 80.f}, 0.01f);
        results.push_back(check("Transform", "LineSegment 360 x 1 deg returns to start", back,
                                format("start (%.3f, %.3f), end (%.3f, %.3f)", s.getStart().getX(),
                                       s.getStart().getY(), s.getEnd().getX(), s.getEnd().getY())));
    }
    {
        LineSegment s(0.f, 0.f, 30.f, 40.f); // length 50
        s.rotate(37.f, {12.f, -5.f});
        s.translate(100.f, 7.f);
        results.push_back(check("Transform", "LineSegment length kept by rotate + translate",
                                std::fabs(s.length() - 50.f) < 1e-3f, format("length %.4f, expected 50", s.length())));
        const float before = s.length();
        s.scale(3.f, 3.f, s.getCenter());
        s.scale(1.f / 3.f, 1.f / 3.f, s.getCenter());
        results.push_back(check("Transform", "scale x3 then x1/3 restores length", std::fabs(s.length() - before) < 1e-3f,
                                format("length %.4f, expected %.4f", s.length(), before)));
    }
    {
        RectangleObject rect(0.f, 0.f, 40.f, 30.f);
        rect.rotate(33.f, {5.f, 5.f});
        const auto& v = rect.getPoints();
        const float a = std::hypot(v[1].getX() - v[0].getX(), v[1].getY() - v[0].getY());
        const float b = std::hypot(v[2].getX() - v[1].getX(), v[2].getY() - v[1].getY());
        const float d = std::hypot(v[2].getX() - v[0].getX(), v[2].getY() - v[0].getY());
        results.push_back(check("Transform", "rotated rectangle keeps its sides",
                                std::fabs(a - 40.f) < 1e-3f && std::fabs(b - 30.f) < 1e-3f && std::fabs(d - 50.f) < 1e-3f,
                                format("sides %.3f x %.3f, diagonal %.3f", a, b, d)));
    }
    {
        // Through the abstract interface: same result for every shape type
        CircleObject circle({50.f, 0.f}, 10.f);
        TransformableObject& t = circle;
        t.rotate(90.f, {0.f, 0.f});
        t.scale(2.f, 2.f, {0.f, 0.f});
        results.push_back(check("Transform", "CircleObject via TransformableObject&",
                                isNear(circle.getCenter(), {0.f, 100.f}) && std::fabs(circle.getRadius() - 20.f) < 1e-3f,
                                format("center (%.2f, %.2f), radius %.2f; expected (0, 100), 20", circle.getCenter().x,
                                       circle.getCenter().y, circle.getRadius())));
    }

    // Virtual inheritance: Player reaches GameObject through UpdatableObject
    // and through ShapeObject (twice) - there must be ONE GameObject
    {
        Player player({100.f, 100.f});
        GameObject* viaUpdatable = static_cast<UpdatableObject*>(&player);
        GameObject* viaDrawable = static_cast<DrawableObject*>(&player);
        GameObject* viaTransformable = static_cast<TransformableObject*>(&player);
        GameObject* viaAnimated = static_cast<AnimatedObject*>(&player);
        static_cast<UpdatableObject&>(player).setActive(false);
        const bool one = viaUpdatable == viaDrawable && viaDrawable == viaTransformable &&
                         viaTransformable == viaAnimated && !viaAnimated->isActive();
        results.push_back(check("Hierarchy", "Player has a single GameObject (virtual inheritance)", one,
                                one ? "Updatable, Drawable, Transformable and Animated paths reach the same GameObject"
                                    : "DIFFERENT GameObject copies"));

        Bullet bullet({0.f, 0.f}, {1.f, 0.f});
        GameObject* a = static_cast<UpdatableObject*>(&bullet);
        GameObject* b = static_cast<ShapeObject*>(&bullet);
        GameObject* c = static_cast<DrawableObject*>(&bullet);
        const bool oneBullet = a == b && b == c;
        results.push_back(check("Hierarchy", "Bullet has a single GameObject", oneBullet,
                                format("bullet %p: via Updatable %p, via Shape %p, via Drawable %p",
                                       static_cast<void*>(&bullet), static_cast<void*>(a), static_cast<void*>(b),
                                       static_cast<void*>(c))));
    }
    return results;
}

std::vector<RasterTestResult> testBitmaps()
{
    std::vector<RasterTestResult> results;
    BitmapHandler bitmaps; // no logger: the failures below are expected
    const sf::Color red = sf::Color::Red, blue = sf::Color::Blue, none = sf::Color::Transparent;

    const bool created = bitmaps.create("a", {8, 8}, red) && bitmaps.exists("a") &&
                         bitmaps.getSize("a") == sf::Vector2u(8, 8) && bitmaps.getPixel("a", {7, 7}) == red;
    results.push_back(check("Bitmap", "create 8x8 filled with red", created, created ? "size and pixels ok" : "WRONG"));

    {
        const std::string path = "bitmap_test_roundtrip.png";
        bitmaps.setPixel("a", {3, 5}, blue);
        bool same = bitmaps.save("a", path) && bitmaps.load("b", path);
        for (unsigned y = 0; same && y < 8; ++y)
            for (unsigned x = 0; same && x < 8; ++x)
                same = bitmaps.getPixel("a", {x, y}) == bitmaps.getPixel("b", {x, y});
        std::error_code ec;
        std::filesystem::remove(path, ec);
        results.push_back(check("Bitmap", "save to PNG and load back", same,
                                same ? "all 64 pixels identical" : "pixels differ or file error"));
    }
    {
        bitmaps.create("c", {16, 16}, none);
        const bool copied = bitmaps.copyRegion("a", sf::IntRect({2, 2}, {4, 4}), "c", {10, 10});
        const bool ok = copied && bitmaps.getPixel("c", {10, 10}) == red && bitmaps.getPixel("c", {13, 13}) == red &&
                        bitmaps.getPixel("c", {9, 9}) == none && bitmaps.getPixel("c", {14, 14}) == none;
        results.push_back(check("Bitmap", "copy 4x4 region between bitmaps", ok,
                                ok ? "lands exactly at (10..13, 10..13)" : "WRONG placement"));
        const bool outOfBounds = !bitmaps.copyRegion("a", {}, "c", {12, 12});
        results.push_back(check("Bitmap", "copy outside the destination is refused", outOfBounds,
                                outOfBounds ? "returned false" : "returned true"));
    }
    {
        const bool ok = bitmaps.extract("c", sf::IntRect({10, 10}, {4, 4}), "d") &&
                        bitmaps.getSize("d") == sf::Vector2u(4, 4) && bitmaps.getPixel("d", {0, 0}) == red &&
                        bitmaps.getPixel("d", {3, 3}) == red;
        results.push_back(check("Bitmap", "extract region as a new bitmap", ok, ok ? "4x4, all red" : "WRONG"));
    }
    {
        bitmaps.create("e", {2, 1}, red);
        bitmaps.setPixel("e", {1, 0}, blue);
        bitmaps.copy("e", "f");
        bitmaps.flipHorizontally("f");
        const bool ok = bitmaps.getPixel("f", {0, 0}) == blue && bitmaps.getPixel("f", {1, 0}) == red &&
                        bitmaps.getPixel("e", {0, 0}) == red;
        results.push_back(check("Bitmap", "copy whole bitmap, flip the copy", ok,
                                ok ? "copy mirrored, original unchanged" : "WRONG"));
    }
    {
        const sf::Texture* before = bitmaps.getTexture("c");
        bitmaps.setPixel("c", {0, 0}, blue);
        const sf::Texture* after = bitmaps.getTexture("c");
        const bool ok = before && before == after && after->getSize() == sf::Vector2u(16, 16) &&
                        after->copyToImage().getPixel({0, 0}) == blue;
        results.push_back(check("Bitmap", "texture re-uploaded after edit, same object", ok,
                                ok ? "pointer stable, GPU copy updated" : "WRONG"));
    }
    {
        const bool removed = bitmaps.remove("a") && !bitmaps.exists("a") && !bitmaps.remove("a");
        const bool missing = !bitmaps.load("g", "no_such_file.png") && !bitmaps.exists("g");
        results.push_back(check("Bitmap", "remove; second remove fails", removed, removed ? "ok" : "WRONG"));
        results.push_back(check("Bitmap", "loading a missing file fails cleanly", missing, missing ? "ok" : "WRONG"));
    }
    {
        // Sprite sheet: the left row must be the right row mirrored, frame by frame
        bool ok = DemoAssets::createPlayerSheet(bitmaps, "sheet", 48);
        const sf::Image* sheet = bitmaps.getImage("sheet");
        unsigned differing = 0;
        for (unsigned f = 0; ok && sheet && f < 4; ++f)
            for (unsigned y = 0; y < 48; ++y)
                for (unsigned x = 0; x < 48; ++x)
                    differing += sheet->getPixel({f * 48 + x, y}) != sheet->getPixel({f * 48 + 47 - x, 48 + y});
        ok = ok && sheet && sheet->getSize() == sf::Vector2u(192, 192) && differing == 0;
        results.push_back(check("Bitmap", "sprite sheet 4x4 frames, left row = mirrored right row", ok,
                                format("sheet %ux%u, %u mismatching pixels", sheet ? sheet->getSize().x : 0,
                                       sheet ? sheet->getSize().y : 0, differing)));
    }
    return results;
}

void logRasterTests(Logger& log, const std::vector<RasterTestResult>& results)
{
    unsigned passed = 0;
    for (const RasterTestResult& r : results) {
        passed += r.passed;
        log.info("  [" + r.group + "] " + r.name + ": " + r.details + (r.passed ? "  -> OK" : "  -> FAIL"));
    }
    log.info("Raster tests: " + std::to_string(passed) + "/" + std::to_string(results.size()) + " passed");
}
