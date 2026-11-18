#include "PrimitiveRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace {

constexpr float kPi = 3.14159265358979f;

int roundToInt(float value)
{
    return static_cast<int>(std::floor(value + 0.5f));
}

} // namespace

// ---------------------------------------------------------------------------
// Points
// ---------------------------------------------------------------------------

// PRIMITIVE: a single pixel. pixelCenter() adds 0.5 so exactly pixel (x, y) is lit.
void PrimitiveRenderer::drawPixel(int x, int y, sf::Color color)
{
    const sf::Vertex vertex{pixelCenter(x, y), color};
    // m_target = the window, a triple-buffer frame or an off-screen bitmap
    m_target.draw(&vertex, 1, sf::PrimitiveType::Points, m_states);
}

// PRIMITIVE: a point - one pixel (size <= 1) or a size x size square centred on it
void PrimitiveRenderer::drawPoint(const Point2D& point, sf::Color color, float size)
{
    if (size <= 1.f) {
        drawPixel(roundToInt(point.getX()), roundToInt(point.getY()), color);
        return;
    }
    sf::RectangleShape square({size, size});
    square.setOrigin({size / 2.f, size / 2.f});
    square.setPosition(point.getPosition() + sf::Vector2f(0.5f, 0.5f));
    square.setFillColor(color);
    m_target.draw(square, m_states);
}

// ---------------------------------------------------------------------------
// Line segments
// ---------------------------------------------------------------------------

// PRIMITIVE (library algorithm): line segment drawn by SFML/OpenGL (GL_LINES)
void PrimitiveRenderer::drawLine(sf::Vector2f start, sf::Vector2f end, sf::Color color)
{
    // Coordinates are in pixels, so shift to pixel centers like drawPixel does
    const sf::Vertex line[] = {
        {start + sf::Vector2f(0.5f, 0.5f), color},
        {end + sf::Vector2f(0.5f, 0.5f), color},
    };
    m_target.draw(line, 2, sf::PrimitiveType::Lines, m_states);
}

// OWN LINE ALGORITHM (incremental / DDA): step 1 pixel along the longer axis (x or y),
// add the slope m (or 1/m) to the other coordinate and round it -> one pixel per step, no gaps
void PrimitiveRenderer::incrementalPixels(sf::Vector2f start, sf::Vector2f end, sf::Color color,
                                          sf::VertexArray& out) const
{
    int x0 = roundToInt(start.x), y0 = roundToInt(start.y);
    int x1 = roundToInt(end.x),   y1 = roundToInt(end.y);
    const int dx = x1 - x0;
    const int dy = y1 - y0;

    // Degenerate segment: a single pixel
    if (dx == 0 && dy == 0) {
        out.append({pixelCenter(x0, y0), color});
        return;
    }

    if (std::abs(dy) <= std::abs(dx)) {
        // |m| <= 1: x is the driving axis, y grows by m each step
        if (x0 > x1) { std::swap(x0, x1); std::swap(y0, y1); }
        const float m = static_cast<float>(y1 - y0) / static_cast<float>(x1 - x0);
        float y = static_cast<float>(y0);
        for (int x = x0; x <= x1; ++x) {
            out.append({pixelCenter(x, roundToInt(y)), color});
            y += m;
        }
    } else {
        // |m| > 1: y is the driving axis, x grows by 1/m each step
        if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
        const float mInv = static_cast<float>(x1 - x0) / static_cast<float>(y1 - y0);
        float x = static_cast<float>(x0);
        for (int y = y0; y <= y1; ++y) {
            out.append({pixelCenter(roundToInt(x), y), color});
            x += mInv;
        }
    }
}

// PRIMITIVE (own algorithm): line drawn with the incremental algorithm above, all pixels in one draw call
void PrimitiveRenderer::drawLineIncremental(sf::Vector2f start, sf::Vector2f end, sf::Color color)
{
    // Pixels are collected and sent to the GPU in one draw call
    sf::VertexArray pixels(sf::PrimitiveType::Points);
    incrementalPixels(start, end, color, pixels);
    m_target.draw(pixels, m_states);
}

void PrimitiveRenderer::drawLine(const Point2D& start, const Point2D& end, sf::Color color,
                                 LineAlgorithm algorithm)
{
    if (algorithm == LineAlgorithm::Incremental)
        drawLineIncremental(start.getPosition(), end.getPosition(), color);
    else
        drawLine(start.getPosition(), end.getPosition(), color);
}

void PrimitiveRenderer::drawLine(const LineSegment& segment, sf::Color color, LineAlgorithm algorithm)
{
    drawLine(segment.getStart(), segment.getEnd(), color, algorithm);
}

// ---------------------------------------------------------------------------
// Polylines
// ---------------------------------------------------------------------------

void PrimitiveRenderer::drawPolyline(const std::vector<Point2D>& points, sf::Color color, bool closed,
                                     LineAlgorithm algorithm)
{
    if (points.empty())
        return;
    if (points.size() == 1) {
        drawPoint(points.front(), color);
        return;
    }

    if (algorithm == LineAlgorithm::Default) {
        // One connected strip, rasterized by the library
        sf::VertexArray strip(sf::PrimitiveType::LineStrip);
        for (const Point2D& p : points)
            strip.append({p.getPosition() + sf::Vector2f(0.5f, 0.5f), color});
        if (closed)
            strip.append({points.front().getPosition() + sf::Vector2f(0.5f, 0.5f), color});
        m_target.draw(strip, m_states);
        return;
    }

    sf::VertexArray pixels(sf::PrimitiveType::Points);
    for (std::size_t i = 0; i + 1 < points.size(); ++i)
        incrementalPixels(points[i].getPosition(), points[i + 1].getPosition(), color, pixels);
    if (closed && points.size() > 2)
        incrementalPixels(points.back().getPosition(), points.front().getPosition(), color, pixels);
    m_target.draw(pixels, m_states);
}

void PrimitiveRenderer::drawPolyline(const std::vector<LineSegment>& segments, sf::Color color,
                                     LineAlgorithm algorithm)
{
    if (algorithm == LineAlgorithm::Default) {
        sf::VertexArray lines(sf::PrimitiveType::Lines);
        for (const LineSegment& s : segments) {
            lines.append({s.getStart().getPosition() + sf::Vector2f(0.5f, 0.5f), color});
            lines.append({s.getEnd().getPosition() + sf::Vector2f(0.5f, 0.5f), color});
        }
        m_target.draw(lines, m_states);
        return;
    }

    sf::VertexArray pixels(sf::PrimitiveType::Points);
    for (const LineSegment& s : segments)
        incrementalPixels(s.getStart().getPosition(), s.getEnd().getPosition(), color, pixels);
    m_target.draw(pixels, m_states);
}

// ---------------------------------------------------------------------------
// Circles and ellipses
// ---------------------------------------------------------------------------

namespace {

// The library has a circle shape but no ellipse: an sf::Shape whose outline
// points lie on an ellipse is rasterized by SFML exactly like CircleShape
class EllipseShape : public sf::Shape {
public:
    EllipseShape(sf::Vector2f radii, std::size_t pointCount) : m_radii(radii), m_pointCount(pointCount)
    {
        update();
    }
    std::size_t getPointCount() const override { return m_pointCount; }
    sf::Vector2f getPoint(std::size_t index) const override
    {
        const float angle = 2.f * kPi * static_cast<float>(index) / static_cast<float>(m_pointCount);
        return {m_radii.x + m_radii.x * std::cos(angle), m_radii.y + m_radii.y * std::sin(angle)};
    }

private:
    sf::Vector2f m_radii;
    std::size_t m_pointCount;
};

std::size_t smoothPointCount(float radius)
{
    // About one polygon corner every 4 pixels of circumference, at least 30
    return std::max<std::size_t>(30, static_cast<std::size_t>(2.f * kPi * radius / 4.f));
}

} // namespace

// PRIMITIVE (library algorithm): circle = an ellipse with two equal radii
void PrimitiveRenderer::drawCircle(sf::Vector2f center, float radius, sf::Color color, bool filled)
{
    drawEllipse(center, radius, radius, color, filled);
}

// PRIMITIVE (library algorithm): ellipse, filled or outline, drawn by SFML via EllipseShape
void PrimitiveRenderer::drawEllipse(sf::Vector2f center, float radiusX, float radiusY, sf::Color color, bool filled)
{
    // An outline is a 1 px band centered on the curve: radius r-0.5 grown by 1
    const float shrink = filled ? 0.f : 0.5f;
    const sf::Vector2f radii{std::max(radiusX - shrink, 0.f), std::max(radiusY - shrink, 0.f)};
    EllipseShape ellipse(radii, smoothPointCount(std::max(radiusX, radiusY)));
    ellipse.setOrigin(radii);
    ellipse.setPosition(center + sf::Vector2f(0.5f, 0.5f));
    if (filled) {
        ellipse.setFillColor(color);
    } else {
        ellipse.setFillColor(sf::Color::Transparent);
        ellipse.setOutlineColor(color);
        ellipse.setOutlineThickness(1.f);
    }
    m_target.draw(ellipse, m_states);
}

// OWN CIRCLE ALGORITHM (8-fold symmetry): computes points only for 0..45 degrees
// and mirrors each one into all 8 octants (swap x/y and change signs).
// Step 1/R radians = about one pixel of arc, so the outline has no gaps.
void PrimitiveRenderer::drawCircleIncremental(sf::Vector2f center, float radius, sf::Color color)
{
    const int cx = roundToInt(center.x);
    const int cy = roundToInt(center.y);
    sf::VertexArray pixels(sf::PrimitiveType::Points);
    const auto plot8 = [&](int x, int y) {
        // 8-fold symmetry: one point of the 0..45 deg octant gives 8 points
        const int pts[8][2] = {{x, y}, {y, x}, {-x, y}, {-y, x}, {x, -y}, {y, -x}, {-x, -y}, {-y, -x}};
        for (const auto& p : pts)
            pixels.append({pixelCenter(cx + p[0], cy + p[1]), color});
    };

    if (radius < 0.5f) {
        plot8(0, 0);
    } else {
        // Step 1/R rad = 1 pixel of arc, so neighbouring points touch
        const float step = 1.f / radius;
        for (float a = 0.f; a < kPi / 4.f; a += step)
            plot8(roundToInt(radius * std::cos(a)), roundToInt(radius * std::sin(a)));
        plot8(roundToInt(radius * std::cos(kPi / 4.f)), roundToInt(radius * std::sin(kPi / 4.f)));
    }
    m_target.draw(pixels, m_states);
}

// OWN ELLIPSE ALGORITHM (4-fold symmetry): computes points only for 0..90 degrees
// (first quadrant) and mirrors each one into the other 3 quadrants. An ellipse
// has no 8-fold symmetry: swapping x/y would turn a wide ellipse into a tall one.
// Step 1/max(Rx, Ry): neither coordinate moves more than 1 pixel per step.
void PrimitiveRenderer::drawEllipseIncremental(sf::Vector2f center, float radiusX, float radiusY, sf::Color color)
{
    const int cx = roundToInt(center.x);
    const int cy = roundToInt(center.y);
    sf::VertexArray pixels(sf::PrimitiveType::Points);
    const auto plot4 = [&](int x, int y) {
        // 4-fold symmetry: one point of the first quadrant gives 4 points
        pixels.append({pixelCenter(cx + x, cy + y), color});
        pixels.append({pixelCenter(cx - x, cy + y), color});
        pixels.append({pixelCenter(cx + x, cy - y), color});
        pixels.append({pixelCenter(cx - x, cy - y), color});
    };

    const float larger = std::max(radiusX, radiusY);
    if (larger < 0.5f) {
        plot4(0, 0);
    } else {
        // Step 1/max(Rx, Ry): no coordinate changes by more than 1 per step
        const float step = 1.f / larger;
        for (float a = 0.f; a < kPi / 2.f; a += step)
            plot4(roundToInt(radiusX * std::cos(a)), roundToInt(radiusY * std::sin(a)));
        plot4(0, roundToInt(radiusY));
    }
    m_target.draw(pixels, m_states);
}

// ---------------------------------------------------------------------------
// Polygons
// ---------------------------------------------------------------------------

const char* toString(PolygonCheck check)
{
    switch (check) {
    case PolygonCheck::Ok:               return "ok";
    case PolygonCheck::TooFewPoints:     return "too few points";
    case PolygonCheck::SelfIntersecting: return "segments intersect";
    case PolygonCheck::NotClosed:        return "not a closed chain";
    }
    return "?";
}

namespace {

// Two segments that share the end point 'shared' may only meet there; if the
// far end of either one lies on the other segment, the chain folds back
bool adjacentOverlap(const LineSegment& s1, const LineSegment& s2, const Point2D& shared)
{
    const Point2D& far1 = (s1.getStart() == shared) ? s1.getEnd() : s1.getStart();
    const Point2D& far2 = (s2.getStart() == shared) ? s2.getEnd() : s2.getStart();
    return s2.contains(far1) || s1.contains(far2);
}

bool sharedEnd(const LineSegment& a, const LineSegment& b, Point2D& shared)
{
    for (const Point2D& p : {a.getStart(), a.getEnd()}) {
        if (p == b.getStart() || p == b.getEnd()) {
            shared = p;
            return true;
        }
    }
    return false;
}

// Drops consecutive duplicates (zero-length edges), including last == first
std::vector<Point2D> withoutRepeats(const std::vector<Point2D>& points)
{
    std::vector<Point2D> result;
    for (const Point2D& p : points)
        if (result.empty() || result.back() != p)
            result.push_back(p);
    while (result.size() > 1 && result.front() == result.back())
        result.pop_back();
    return result;
}

} // namespace

// POLYGON CHECK (points): is the closed chain a simple polygon?
// Compares every pair of edges: neighbours may only share their corner,
// non-neighbours must not touch at all (LineSegment::intersects).
PolygonCheck PrimitiveRenderer::validatePolygon(const std::vector<Point2D>& input)
{
    const std::vector<Point2D> points = withoutRepeats(input);
    const std::size_t n = points.size();
    if (n < 3)
        return PolygonCheck::TooFewPoints;

    std::vector<LineSegment> edges;
    for (std::size_t i = 0; i < n; ++i)
        edges.emplace_back(points[i], points[(i + 1) % n]);

    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            const bool adjacent = (j == i + 1) || (i == 0 && j == n - 1);
            if (adjacent) {
                const Point2D& shared = (j == i + 1) ? points[j] : points[0];
                if (adjacentOverlap(edges[i], edges[j], shared))
                    return PolygonCheck::SelfIntersecting;
            } else if (edges[i].intersects(edges[j])) {
                return PolygonCheck::SelfIntersecting;
            }
        }
    }
    return PolygonCheck::Ok;
}

// POLYGON CHECK (segments): loose segments must also form ONE closed chain -
// every corner shared by exactly 2 segments, and a walk along the chain must
// visit all of them - and no two segments may cross.
PolygonCheck PrimitiveRenderer::validatePolygon(const std::vector<LineSegment>& segments)
{
    if (segments.size() < 3)
        return PolygonCheck::TooFewPoints;
    for (const LineSegment& s : segments)
        if (s.getStart() == s.getEnd())
            return PolygonCheck::NotClosed;

    // Closed chain: every vertex is the end of exactly two segments
    for (const LineSegment& s : segments) {
        for (const Point2D& p : {s.getStart(), s.getEnd()}) {
            int count = 0;
            for (const LineSegment& t : segments)
                count += (t.getStart() == p) + (t.getEnd() == p);
            if (count != 2)
                return PolygonCheck::NotClosed;
        }
    }

    for (std::size_t i = 0; i < segments.size(); ++i) {
        for (std::size_t j = i + 1; j < segments.size(); ++j) {
            Point2D shared;
            if (sharedEnd(segments[i], segments[j], shared)) {
                if (adjacentOverlap(segments[i], segments[j], shared))
                    return PolygonCheck::SelfIntersecting;
            } else if (segments[i].intersects(segments[j])) {
                return PolygonCheck::SelfIntersecting;
            }
        }
    }

    // Degree 2 everywhere can still be two separate loops - walk the chain
    std::vector<bool> used(segments.size(), false);
    used[0] = true;
    std::size_t visited = 1;
    Point2D current = segments[0].getEnd();
    while (current != segments[0].getStart()) {
        bool advanced = false;
        for (std::size_t i = 0; i < segments.size() && !advanced; ++i) {
            if (used[i])
                continue;
            if (segments[i].getStart() == current) {
                current = segments[i].getEnd();
                advanced = true;
            } else if (segments[i].getEnd() == current) {
                current = segments[i].getStart();
                advanced = true;
            }
            if (advanced) {
                used[i] = true;
                ++visited;
            }
        }
        if (!advanced)
            return PolygonCheck::NotClosed;
    }
    return visited == segments.size() ? PolygonCheck::Ok : PolygonCheck::NotClosed;
}

// POLYGON (from points): draws the outline only if validatePolygon says Ok;
// otherwise draws NOTHING and returns false
bool PrimitiveRenderer::drawPolygon(const std::vector<Point2D>& points, sf::Color color, LineAlgorithm algorithm)
{
    if (validatePolygon(points) != PolygonCheck::Ok)
        return false; // a broken polygonal chain: draw nothing
    drawPolyline(withoutRepeats(points), color, true, algorithm);
    return true;
}

// POLYGON (from LineSegment objects): same rule - invalid chain, nothing drawn
bool PrimitiveRenderer::drawPolygon(const std::vector<LineSegment>& segments, sf::Color color,
                                    LineAlgorithm algorithm)
{
    if (validatePolygon(segments) != PolygonCheck::Ok)
        return false; // a broken polygonal chain: draw nothing
    drawPolyline(segments, color, algorithm);
    return true;
}

// ---------------------------------------------------------------------------
// Filled primitives
// ---------------------------------------------------------------------------

// FILLED PRIMITIVE: rectangle, filled (filled = true) or only its outline
void PrimitiveRenderer::drawRectangle(sf::Vector2f position, sf::Vector2f size, sf::Color color, bool filled)
{
    sf::RectangleShape rect(size);
    rect.setPosition(position);
    if (filled) {
        rect.setFillColor(color);
    } else {
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(color);
        rect.setOutlineThickness(-1.f); // inside the rectangle's bounds
    }
    m_target.draw(rect, m_states);
}

// FILLED PRIMITIVE: triangle
void PrimitiveRenderer::drawTriangle(const Point2D& a, const Point2D& b, const Point2D& c, sf::Color color,
                                     bool filled)
{
    if (filled)
        fillConvexPolygon({a, b, c}, color);   // filled: a triangle is always convex
    else
        drawPolyline({a, b, c}, color, true);  // outline: closed polyline of the 3 corners
}

// FILLED PRIMITIVE (library algorithm): any convex polygon, filled by SFML (sf::ConvexShape)
void PrimitiveRenderer::fillConvexPolygon(const std::vector<Point2D>& points, sf::Color color)
{
    if (points.size() < 3)
        return;
    sf::ConvexShape polygon(points.size());
    for (std::size_t i = 0; i < points.size(); ++i)
        polygon.setPoint(i, points[i].getPosition() + sf::Vector2f(0.5f, 0.5f));
    polygon.setFillColor(color);
    m_target.draw(polygon, m_states);
}

// OWN POLYGON FILL (parity / even-odd scan-line): for every pixel row, find
// where the row crosses the edges, sort those x values and fill between
// crossings 1-2, 3-4, ... Works for convex, concave and self-intersecting
// polygons (a star's centre stays empty).
bool PrimitiveRenderer::fillPolygon(const std::vector<Point2D>& points, sf::Color color)
{
    if (points.size() < 3)
        return false;

    float minY = points[0].getY(), maxY = points[0].getY();
    for (const Point2D& p : points) {
        minY = std::min(minY, p.getY());
        maxY = std::max(maxY, p.getY());
    }

    sf::VertexArray spans(sf::PrimitiveType::Triangles);
    std::vector<float> crossings;
    const std::size_t n = points.size();

    // Every pixel row whose center (y) lies inside the polygon's height
    for (int y = static_cast<int>(std::ceil(minY)); static_cast<float>(y) < maxY; ++y) {
        const float scanY = static_cast<float>(y);
        crossings.clear();
        for (std::size_t i = 0; i < n; ++i) {
            const Point2D& a = points[i];
            const Point2D& b = points[(i + 1) % n];
            // Half-open rule [ymin, ymax): a vertex shared by two edges is
            // counted once, horizontal edges never
            const bool crosses = (a.getY() <= scanY && scanY < b.getY()) || (b.getY() <= scanY && scanY < a.getY());
            if (crosses)
                crossings.push_back(a.getX() + (scanY - a.getY()) * (b.getX() - a.getX()) / (b.getY() - a.getY()));
        }
        std::sort(crossings.begin(), crossings.end());

        // Parity: inside between the 1st and 2nd crossing, the 3rd and 4th, ...
        for (std::size_t i = 0; i + 1 < crossings.size(); i += 2) {
            const int xStart = static_cast<int>(std::ceil(crossings[i]));
            const int xEnd = static_cast<int>(std::ceil(crossings[i + 1])); // exclusive
            if (xEnd <= xStart)
                continue;
            // One quad covering pixels [xStart, xEnd) of this row
            const float left = static_cast<float>(xStart), right = static_cast<float>(xEnd);
            const float top = scanY, bottom = scanY + 1.f;
            spans.append({{left, top}, color});
            spans.append({{right, top}, color});
            spans.append({{right, bottom}, color});
            spans.append({{left, top}, color});
            spans.append({{right, bottom}, color});
            spans.append({{left, bottom}, color});
        }
    }
    m_target.draw(spans, m_states);
    return true;
}

// ---------------------------------------------------------------------------
// Bitmaps
// ---------------------------------------------------------------------------

// BITMAP DISPLAYING (step 3): an sf::Sprite shows the texture (or only the rectangle 'rect' of it, = one frame)
void PrimitiveRenderer::drawBitmap(const sf::Texture& texture, sf::IntRect rect, const sf::Transform& transform,
                                   sf::Vector2f origin)
{
    sf::Sprite sprite(texture);
    if (rect.size.x > 0 && rect.size.y > 0)
        sprite.setTextureRect(rect);
    sprite.setOrigin(origin);
    sf::RenderStates states = m_states;
    states.transform *= transform;
    m_target.draw(sprite, states);
}

void PrimitiveRenderer::drawBitmap(const sf::Texture& texture, sf::Vector2f position)
{
    sf::Transform transform;
    transform.translate(position);
    drawBitmap(texture, {}, transform);
}

// ---------------------------------------------------------------------------
// Area filling
// ---------------------------------------------------------------------------

namespace {

// Shared seed-fill core: 4-connected, explicit stack (no recursion depth limit)
template <typename ShouldFill>
unsigned seedFill(sf::Image& image, sf::Vector2u seed, sf::Color fillColor, ShouldFill shouldFill,
                  std::vector<sf::Vector2u>* changed)
{
    const sf::Vector2u size = image.getSize();
    if (seed.x >= size.x || seed.y >= size.y)
        return 0;

    unsigned count = 0;
    std::vector<sf::Vector2u> stack{seed};
    while (!stack.empty()) {
        const sf::Vector2u p = stack.back();
        stack.pop_back();
        if (!shouldFill(image.getPixel(p)))
            continue;
        image.setPixel(p, fillColor);
        ++count;
        if (changed)
            changed->push_back(p);
        if (p.x > 0)          stack.push_back({p.x - 1, p.y});
        if (p.x + 1 < size.x) stack.push_back({p.x + 1, p.y});
        if (p.y > 0)          stack.push_back({p.x, p.y - 1});
        if (p.y + 1 < size.y) stack.push_back({p.x, p.y + 1});
    }
    return count;
}

} // namespace

// FLOOD FILL (on an image): recolours the connected area that has the seed
// pixel's ORIGINAL colour; stops at any other colour
unsigned PrimitiveRenderer::floodFill(sf::Image& image, sf::Vector2u seed, sf::Color fillColor,
                                      std::vector<sf::Vector2u>* changed)
{
    const sf::Vector2u size = image.getSize();
    if (seed.x >= size.x || seed.y >= size.y)
        return 0;
    const sf::Color oldColor = image.getPixel(seed);
    if (oldColor == fillColor)
        return 0;
    // Flood fill: spread over pixels that have the seed's original colour
    return seedFill(image, seed, fillColor, [&](sf::Color c) { return c == oldColor; }, changed);
}

// BOUNDARY FILL (on an image): recolours EVERYTHING connected to the seed,
// whatever its colour, until it reaches pixels of the boundary colour
unsigned PrimitiveRenderer::boundaryFill(sf::Image& image, sf::Vector2u seed, sf::Color fillColor,
                                         sf::Color boundaryColor, std::vector<sf::Vector2u>* changed)
{
    // Boundary fill: spread over everything until the boundary colour
    return seedFill(image, seed, fillColor,
                    [&](sf::Color c) { return c != boundaryColor && c != fillColor; }, changed);
}

// Fills must READ pixels, which the GPU cannot do directly: copy the render
// target (bitmap or window) into a CPU image first
std::optional<sf::Image> PrimitiveRenderer::captureTarget()
{
    if (auto* texture = dynamic_cast<sf::RenderTexture*>(&m_target)) {
        texture->display(); // make what was drawn so far readable
        return texture->getTexture().copyToImage();
    }
    if (auto* window = dynamic_cast<sf::RenderWindow*>(&m_target)) {
        sf::Texture copy;
        if (!copy.resize(window->getSize()))
            return std::nullopt;
        copy.update(*window);
        return copy.copyToImage();
    }
    return std::nullopt;
}

// After a fill: draws only the pixels that changed back onto the target
void PrimitiveRenderer::drawChangedPixels(const std::vector<sf::Vector2u>& pixels, sf::Color color)
{
    sf::VertexArray points(sf::PrimitiveType::Points);
    for (const sf::Vector2u& p : pixels)
        points.append({pixelCenter(static_cast<int>(p.x), static_cast<int>(p.y)), color});
    sf::RenderStates exact = m_states;
    exact.blendMode = sf::BlendNone; // write the colour exactly, alpha included
    m_target.draw(points, exact);
}

// FLOOD FILL (on the render target): capture -> fill the image -> draw changes back
unsigned PrimitiveRenderer::floodFill(int x, int y, sf::Color fillColor)
{
    if (x < 0 || y < 0)
        return 0;
    std::optional<sf::Image> image = captureTarget();
    if (!image)
        return 0;
    std::vector<sf::Vector2u> changed;
    const unsigned count =
        floodFill(*image, {static_cast<unsigned>(x), static_cast<unsigned>(y)}, fillColor, &changed);
    drawChangedPixels(changed, fillColor);
    return count;
}

// BOUNDARY FILL (on the render target): capture -> fill the image -> draw changes back
unsigned PrimitiveRenderer::boundaryFill(int x, int y, sf::Color fillColor, sf::Color boundaryColor)
{
    if (x < 0 || y < 0)
        return 0;
    std::optional<sf::Image> image = captureTarget();
    if (!image)
        return 0;
    std::vector<sf::Vector2u> changed;
    const unsigned count = boundaryFill(*image, {static_cast<unsigned>(x), static_cast<unsigned>(y)}, fillColor,
                                        boundaryColor, &changed);
    drawChangedPixels(changed, fillColor);
    return count;
}
