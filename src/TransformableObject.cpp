#include "TransformableObject.h"

#include <cmath>

// ROTATION of one point around the pivot: move pivot to origin, apply cos/sin, move back
sf::Vector2f TransformableObject::rotatePoint(sf::Vector2f point, float degrees, sf::Vector2f pivot)
{
    const float radians = degrees * 3.14159265358979f / 180.f;
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    const sf::Vector2f d = point - pivot;
    return {pivot.x + d.x * c - d.y * s, pivot.y + d.x * s + d.y * c};
}

// SCALING (HOMOTHETY) of one point: its distance from the pivot is multiplied by (sx, sy)
sf::Vector2f TransformableObject::scalePoint(sf::Vector2f point, float sx, float sy, sf::Vector2f pivot)
{
    return {pivot.x + (point.x - pivot.x) * sx, pivot.y + (point.y - pivot.y) * sy};
}
