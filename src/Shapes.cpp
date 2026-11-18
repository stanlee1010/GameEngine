#include "Shapes.h"

#include "PrimitiveRenderer.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979f;

} // namespace

// ---------------------------------------------------------------------------
// PolylineObject
// ---------------------------------------------------------------------------

void PolylineObject::draw(PrimitiveRenderer& renderer) const
{
    renderer.drawPolyline(m_points, getColor(), m_closed, getAlgorithm());
}

// TRANSLATION: move every vertex by (dx, dy)
void PolylineObject::translate(float dx, float dy)
{
    for (Point2D& p : m_points)
        p.translate(dx, dy);
}

// ROTATION: turn every vertex around the pivot
void PolylineObject::rotate(float degrees, sf::Vector2f pivot)
{
    for (Point2D& p : m_points)
        p.rotate(degrees, pivot);
}

// SCALING (HOMOTHETY): every vertex moves away from / towards the pivot
void PolylineObject::scale(float sx, float sy, sf::Vector2f pivot)
{
    for (Point2D& p : m_points)
        p.scale(sx, sy, pivot);
}

sf::Vector2f PolylineObject::getCenter() const
{
    if (m_points.empty())
        return {};
    sf::Vector2f sum;
    for (const Point2D& p : m_points)
        sum += p.getPosition();
    return sum / static_cast<float>(m_points.size());
}

// ---------------------------------------------------------------------------
// PolygonObject and friends
// ---------------------------------------------------------------------------

bool PolygonObject::isValid() const
{
    return PrimitiveRenderer::validatePolygon(m_points) == PolygonCheck::Ok;
}

void PolygonObject::draw(PrimitiveRenderer& renderer) const
{
    if (!isValid())
        return; // self-intersecting chain: draw nothing
    if (isFilled())
        renderer.fillPolygon(m_points, getFillColor());
    renderer.drawPolygon(m_points, getColor(), getAlgorithm());
}

RectangleObject::RectangleObject(float x, float y, float width, float height)
    : PolygonObject({{x, y}, {x + width, y}, {x + width, y + height}, {x, y + height}})
{
}

TriangleObject::TriangleObject(const Point2D& a, const Point2D& b, const Point2D& c) : PolygonObject({a, b, c})
{
}

// ---------------------------------------------------------------------------
// CircleObject
// ---------------------------------------------------------------------------

void CircleObject::draw(PrimitiveRenderer& renderer) const
{
    if (isFilled())
        renderer.drawCircle(m_center, m_radius, getFillColor(), true);
    if (getAlgorithm() == LineAlgorithm::Incremental)
        renderer.drawCircleIncremental(m_center, m_radius, getColor());
    else
        renderer.drawCircle(m_center, m_radius, getColor(), false);
}

// ROTATION: only the centre moves (a circle looks the same when turned)
void CircleObject::rotate(float degrees, sf::Vector2f pivot)
{
    m_center = rotatePoint(m_center, degrees, pivot);
}

// SCALING: centre moves, radius grows/shrinks
void CircleObject::scale(float sx, float sy, sf::Vector2f pivot)
{
    m_center = scalePoint(m_center, sx, sy, pivot);
    m_radius *= std::sqrt(std::fabs(sx * sy));
}

// ---------------------------------------------------------------------------
// EllipseObject
// ---------------------------------------------------------------------------

// ROTATION: centre moves around the pivot and the ellipse itself turns (m_rotation)
void EllipseObject::rotate(float degrees, sf::Vector2f pivot)
{
    m_center = rotatePoint(m_center, degrees, pivot);
    m_rotation = std::fmod(m_rotation + degrees, 360.f);
    if (m_rotation < 0.f)
        m_rotation += 360.f;
}

// SCALING: centre moves, both radii grow/shrink
void EllipseObject::scale(float sx, float sy, sf::Vector2f pivot)
{
    m_center = scalePoint(m_center, sx, sy, pivot);
    m_radii.x *= std::fabs(sx);
    m_radii.y *= std::fabs(sy);
}

std::vector<Point2D> EllipseObject::outlinePoints() const
{
    const std::size_t count =
        std::max<std::size_t>(24, static_cast<std::size_t>(2.f * kPi * std::max(m_radii.x, m_radii.y) / 3.f));
    std::vector<Point2D> points;
    points.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const float a = 2.f * kPi * static_cast<float>(i) / static_cast<float>(count);
        const sf::Vector2f local = m_center + sf::Vector2f(m_radii.x * std::cos(a), m_radii.y * std::sin(a));
        points.emplace_back(rotatePoint(local, m_rotation, m_center));
    }
    return points;
}

void EllipseObject::draw(PrimitiveRenderer& renderer) const
{
    // Within 0.5 deg of a multiple of 90 deg the ellipse is axis-aligned
    const float quarter = std::round(m_rotation / 90.f);
    const bool axisAligned = std::fabs(m_rotation - quarter * 90.f) < 0.5f;
    if (axisAligned) {
        const bool swapped = static_cast<int>(quarter) % 2 != 0;
        const float rx = swapped ? m_radii.y : m_radii.x;
        const float ry = swapped ? m_radii.x : m_radii.y;
        if (isFilled())
            renderer.drawEllipse(m_center, rx, ry, getFillColor(), true);
        if (getAlgorithm() == LineAlgorithm::Incremental)
            renderer.drawEllipseIncremental(m_center, rx, ry, getColor());
        else
            renderer.drawEllipse(m_center, rx, ry, getColor(), false);
        return;
    }

    const std::vector<Point2D> outline = outlinePoints();
    if (isFilled())
        renderer.fillPolygon(outline, getFillColor());
    renderer.drawPolyline(outline, getColor(), true, getAlgorithm());
}
