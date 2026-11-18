#pragma once

#include "Point2D.h"
#include "ShapeObject.h"

#include <vector>

// Shapes built from primitives. Every class implements draw() (with the
// PrimitiveRenderer) and translate() / rotate() / scale() from ShapeObject.
// Point2D and LineSegment are in their own headers.

// Open or closed chain of points.
class PolylineObject : public ShapeObject {
public:
    PolylineObject() = default;
    explicit PolylineObject(std::vector<Point2D> points, bool closed = false)
        : m_points(std::move(points)), m_closed(closed) {}

    const char* getTypeName() const override { return "PolylineObject"; }

    const std::vector<Point2D>& getPoints() const { return m_points; }
    void setPoints(std::vector<Point2D> points) { m_points = std::move(points); }
    void addPoint(const Point2D& point) { m_points.push_back(point); }
    bool isClosed() const { return m_closed; }
    void setClosed(bool closed) { m_closed = closed; }

    void draw(PrimitiveRenderer& renderer) const override;
    void translate(float dx, float dy) override;
    void rotate(float degrees, sf::Vector2f pivot) override;
    void scale(float sx, float sy, sf::Vector2f pivot) override;
    sf::Vector2f getCenter() const override; // average of the vertices

protected:
    std::vector<Point2D> m_points;
    bool m_closed = false;
};

// Closed polygon: the outline is drawn only if it does not intersect itself;
// when filled, the inside uses the parity scan-line fill.
class PolygonObject : public PolylineObject {
public:
    PolygonObject() { m_closed = true; }
    explicit PolygonObject(std::vector<Point2D> points) : PolylineObject(std::move(points), true) {}

    const char* getTypeName() const override { return "PolygonObject"; }

    // Is the outline a simple polygon (no intersecting segments)?
    bool isValid() const;

    void draw(PrimitiveRenderer& renderer) const override;
};

class RectangleObject : public PolygonObject {
public:
    RectangleObject(float x, float y, float width, float height);
    const char* getTypeName() const override { return "RectangleObject"; }
};

class TriangleObject : public PolygonObject {
public:
    TriangleObject(const Point2D& a, const Point2D& b, const Point2D& c);
    const char* getTypeName() const override { return "TriangleObject"; }
};

// Circle. Rotation moves the center around the pivot; non-uniform scaling
// keeps it a circle, scaling the radius by sqrt(|sx * sy|) (same area ratio).
class CircleObject : public ShapeObject {
public:
    CircleObject(sf::Vector2f center, float radius) : m_center(center), m_radius(radius) {}

    const char* getTypeName() const override { return "CircleObject"; }

    float getRadius() const { return m_radius; }
    void setRadius(float radius) { m_radius = radius; }

    void draw(PrimitiveRenderer& renderer) const override;
    void translate(float dx, float dy) override { m_center += {dx, dy}; } // TRANSLATION: move the centre
    void rotate(float degrees, sf::Vector2f pivot) override;
    void scale(float sx, float sy, sf::Vector2f pivot) override;
    sf::Vector2f getCenter() const override { return m_center; }

private:
    sf::Vector2f m_center;
    float m_radius;
};

// Ellipse with its own rotation angle. Axis-aligned (0/90/180/270 deg) it is
// drawn with the ellipse algorithms; at any other angle it is drawn as a
// rotated polygon approximation (the incremental algorithm is axis-aligned).
class EllipseObject : public ShapeObject {
public:
    EllipseObject(sf::Vector2f center, float radiusX, float radiusY)
        : m_center(center), m_radii(radiusX, radiusY) {}

    const char* getTypeName() const override { return "EllipseObject"; }

    sf::Vector2f getRadii() const { return m_radii; }
    float getRotation() const { return m_rotation; }

    void draw(PrimitiveRenderer& renderer) const override;
    void translate(float dx, float dy) override { m_center += {dx, dy}; } // TRANSLATION: move the centre
    void rotate(float degrees, sf::Vector2f pivot) override;
    // Scales the radii along the ellipse's own axes and moves the center
    void scale(float sx, float sy, sf::Vector2f pivot) override;
    sf::Vector2f getCenter() const override { return m_center; }

private:
    std::vector<Point2D> outlinePoints() const;

    sf::Vector2f m_center;
    sf::Vector2f m_radii;
    float m_rotation = 0.f; // degrees
};
