#pragma once

#include "GameObject.h"

// Objects that take part in the game logic (called from the update callback).
class UpdatableObject : public virtual GameObject {
public:
    // dt = time step in seconds (fixed, 1 / FPS)
    virtual void update(float dt) = 0;
};
