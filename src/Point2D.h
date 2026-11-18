#pragma once

#include "ShapeObject.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

class PrimitiveRenderer;

// A point in 2D space - the simplest drawable, transformable shape.
class Point2D : public ShapeObject {
public:
    Point2D() = default;
    Point2D(float x, float y) : m_x(x), m_y(y) {}
    explicit Point2D(sf::Vector2f position) : m_x(position.x), m_y(position.y) {}

    const char* getTypeName() const override { return "Point2D"; }

    // --- Reading coordinates ---
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    sf::Vector2f getPosition() const { return {m_x, m_y}; }

    // --- Modifying coordinates ---
    void setX(float x) { m_x = x; }
    void setY(float y) { m_y = y; }
    void set(float x, float y) { m_x = x; m_y = y; }
    void setPosition(sf::Vector2f position) { set(position.x, position.y); }

    // --- Transformations (TransformableObject) ---
    void translate(float dx, float dy) override { m_x += dx; m_y += dy; } // TRANSLATION
    void rotate(float degrees, sf::Vector2f pivot) override;
    void scale(float sx, float sy, sf::Vector2f pivot) override;
    sf::Vector2f getCenter() const override { return getPosition(); }

    // --- Drawing ---
    // DrawableObject: uses the point's own colour and size
    void draw(PrimitiveRenderer& renderer) const override;
    // size == 1 draws a single pixel, larger sizes draw a size x size square
    void draw(PrimitiveRenderer& renderer, sf::Color color, float size = 1.f) const;

    float getSize() const { return m_size; }
    void setSize(float size) { m_size = size; }

    // Equal coordinates (colour / size are not compared)
    bool operator==(const Point2D& other) const { return m_x == other.m_x && m_y == other.m_y; }
    bool operator!=(const Point2D& other) const { return !(*this == other); }

private:
    float m_x = 0.f;
    float m_y = 0.f;
    float m_size = 1.f;
};
