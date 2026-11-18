#pragma once

#include "LineAlgorithm.h"
#include "Point2D.h"
#include "ShapeObject.h"

class PrimitiveRenderer;

// A line segment defined by its two end points.
class LineSegment : public ShapeObject {
public:
    LineSegment() = default;
    LineSegment(const Point2D& start, const Point2D& end) : m_start(start), m_end(end) {}
    LineSegment(float x0, float y0, float x1, float y1) : m_start(x0, y0), m_end(x1, y1) {}

    const char* getTypeName() const override { return "LineSegment"; }

    // --- Reading the ends ---
    const Point2D& getStart() const { return m_start; }
    const Point2D& getEnd() const { return m_end; }

    // --- Modifying the ends ---
    void setStart(const Point2D& start) { m_start = start; }
    void setEnd(const Point2D& end) { m_end = end; }
    void setStart(float x, float y) { m_start.set(x, y); }
    void setEnd(float x, float y) { m_end.set(x, y); }
    void set(const Point2D& start, const Point2D& end) { m_start = start; m_end = end; }

    float length() const;

    // True when the two segments share at least one point (crossing,
    // touching at an end, or overlapping when collinear)
    bool intersects(const LineSegment& other) const;
    // True when point p lies on this segment
    bool contains(const Point2D& p) const;

    // --- Transformations (TransformableObject): applied to both ends ---
    void translate(float dx, float dy) override;
    void rotate(float degrees, sf::Vector2f pivot) override;
    void scale(float sx, float sy, sf::Vector2f pivot) override;
    sf::Vector2f getCenter() const override; // midpoint

    // --- Drawing (choice of algorithm) ---
    // DrawableObject: own colour and algorithm (setColor / setAlgorithm)
    void draw(PrimitiveRenderer& renderer) const override;
    void draw(PrimitiveRenderer& renderer, sf::Color color,
              LineAlgorithm algorithm = LineAlgorithm::Default) const;

private:
    Point2D m_start;
    Point2D m_end;
};
