#pragma once

#include "GameObject.h"

class PrimitiveRenderer;

// Objects that can be drawn on the screen (called from the render callback).
class DrawableObject : public virtual GameObject {
public:
    virtual void draw(PrimitiveRenderer& renderer) const = 0;

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

private:
    bool m_visible = true;
};
