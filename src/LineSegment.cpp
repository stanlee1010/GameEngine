#include "LineSegment.h"

#include "PrimitiveRenderer.h"

#include <algorithm>
#include <cmath>

namespace {

// > 0: c is left of a->b, < 0: right, 0: collinear
double orientation(const Point2D& a, const Point2D& b, const Point2D& c)
{
    return (static_cast<double>(b.getX()) - a.getX()) * (static_cast<double>(c.getY()) - a.getY()) -
           (static_cast<double>(b.getY()) - a.getY()) * (static_cast<double>(c.getX()) - a.getX());
}

int sign(double v)
{
    constexpr double eps = 1e-9;
    return (v > eps) - (v < -eps);
}

// c is collinear with a-b: is it inside their bounding box?
bool withinBox(const Point2D& a, const Point2D& b, const Point2D& c)
{
    return c.getX() >= std::min(a.getX(), b.getX()) && c.getX() <= std::max(a.getX(), b.getX()) &&
           c.getY() >= std::min(a.getY(), b.getY()) && c.getY() <= std::max(a.getY(), b.getY());
}

} // namespace

float LineSegment::length() const
{
    return std::hypot(m_end.getX() - m_start.getX(), m_end.getY() - m_start.getY());
}

bool LineSegment::contains(const Point2D& p) const
{
    return sign(orientation(m_start, m_end, p)) == 0 && withinBox(m_start, m_end, p);
}

bool LineSegment::intersects(const LineSegment& other) const
{
    const Point2D& a = m_start;
    const Point2D& b = m_end;
    const Point2D& c = other.m_start;
    const Point2D& d = other.m_end;

    const int o1 = sign(orientation(a, b, c));
    const int o2 = sign(orientation(a, b, d));
    const int o3 = sign(orientation(c, d, a));
    const int o4 = sign(orientation(c, d, b));

    // Proper crossing: each segment's ends lie on opposite sides of the other
    if (o1 != o2 && o3 != o4)
        return true;

    // Touching / collinear overlap
    return (o1 == 0 && withinBox(a, b, c)) || (o2 == 0 && withinBox(a, b, d)) ||
           (o3 == 0 && withinBox(c, d, a)) || (o4 == 0 && withinBox(c, d, b));
}

// TRANSLATION / ROTATION / SCALING of a segment = transform both end points
void LineSegment::translate(float dx, float dy)
{
    m_start.translate(dx, dy);
    m_end.translate(dx, dy);
}

void LineSegment::rotate(float degrees, sf::Vector2f pivot)
{
    m_start.rotate(degrees, pivot);
    m_end.rotate(degrees, pivot);
}

void LineSegment::scale(float sx, float sy, sf::Vector2f pivot)
{
    m_start.scale(sx, sy, pivot);
    m_end.scale(sx, sy, pivot);
}

sf::Vector2f LineSegment::getCenter() const
{
    return (m_start.getPosition() + m_end.getPosition()) / 2.f;
}

void LineSegment::draw(PrimitiveRenderer& renderer) const
{
    renderer.drawLine(m_start, m_end, getColor(), getAlgorithm());
}

void LineSegment::draw(PrimitiveRenderer& renderer, sf::Color color, LineAlgorithm algorithm) const
{
    renderer.drawLine(m_start, m_end, color, algorithm);
}
