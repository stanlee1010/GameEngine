#pragma once

#include "DrawableObject.h"
#include "LineAlgorithm.h"
#include "TransformableObject.h"

#include <SFML/Graphics/Color.hpp>

// Base of every object drawn with primitives: it can be drawn AND transformed.
// DrawableObject and TransformableObject both inherit GameObject virtually,
// so a ShapeObject holds a single GameObject.
class ShapeObject : public DrawableObject, public TransformableObject {
public:
    sf::Color getColor() const { return m_color; }
    void setColor(sf::Color color) { m_color = color; }

    sf::Color getFillColor() const { return m_fillColor; }
    void setFillColor(sf::Color color) { m_fillColor = color; }

    bool isFilled() const { return m_filled; }
    void setFilled(bool filled) { m_filled = filled; }

    // Which line / curve rasterization the outline uses
    LineAlgorithm getAlgorithm() const { return m_algorithm; }
    void setAlgorithm(LineAlgorithm algorithm) { m_algorithm = algorithm; }

private:
    sf::Color m_color = sf::Color::White;
    sf::Color m_fillColor = sf::Color(90, 90, 90);
    bool m_filled = false;
    LineAlgorithm m_algorithm = LineAlgorithm::Default;
};
