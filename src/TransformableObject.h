#pragma once

#include "GameObject.h"

#include <SFML/System/Vector2.hpp>

// Objects that can undergo 2D geometric transformations.
class TransformableObject : public virtual GameObject {
public:
    // TRANSLATION - moving an object to another position: shift by the vector (dx, dy)
    virtual void translate(float dx, float dy) = 0;
    // ROTATION - turning an object around a point: by 'degrees' (clockwise on screen, since y points down)
    // around the point 'pivot'
    virtual void rotate(float degrees, sf::Vector2f pivot) = 0;
    // SCALING (HOMOTHETY) - making an object bigger or smaller: by (sx, sy) relative to the fixed point 'pivot'
    virtual void scale(float sx, float sy, sf::Vector2f pivot) = 0;

    // Reference point for transformations "in place"
    virtual sf::Vector2f getCenter() const = 0;

    // Convenience methods with a body, built on the abstract ones
    void rotateAroundCenter(float degrees) { rotate(degrees, getCenter()); }
    void scaleAroundCenter(float factor) { scale(factor, factor, getCenter()); }
    void moveTo(sf::Vector2f position)
    {
        const sf::Vector2f c = getCenter();
        translate(position.x - c.x, position.y - c.y);
    }

    // The transformation formulas for a single point:
    //   rotation:  x' = px + (x - px) cos a - (y - py) sin a
    //              y' = py + (x - px) sin a + (y - py) cos a
    //   scaling:   x' = px + (x - px) sx,   y' = py + (y - py) sy
    static sf::Vector2f rotatePoint(sf::Vector2f point, float degrees, sf::Vector2f pivot);
    static sf::Vector2f scalePoint(sf::Vector2f point, float sx, float sy, sf::Vector2f pivot);
};
