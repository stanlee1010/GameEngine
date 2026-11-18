#include "Point2D.h"

#include "PrimitiveRenderer.h"

// ROTATION: the point turns around the pivot (uses TransformableObject::rotatePoint)
void Point2D::rotate(float degrees, sf::Vector2f pivot)
{
    setPosition(rotatePoint(getPosition(), degrees, pivot));
}

// SCALING: the point moves away from / towards the pivot (uses scalePoint)
void Point2D::scale(float sx, float sy, sf::Vector2f pivot)
{
    setPosition(scalePoint(getPosition(), sx, sy, pivot));
}

void Point2D::draw(PrimitiveRenderer& renderer) const
{
    renderer.drawPoint(*this, getColor(), m_size);
}

void Point2D::draw(PrimitiveRenderer& renderer, sf::Color color, float size) const
{
    renderer.drawPoint(*this, color, size);
}
