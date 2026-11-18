#pragma once

#include "GameObject.h"

// Objects that animate over time (sprite frames, colour cycling, ...).
// Separate from UpdatableObject: animation is presentation, not game logic,
// and the engine may animate objects that have no logic at all.
class AnimatedObject : public virtual GameObject {
public:
    // dt = elapsed time in seconds
    virtual void animate(float dt) = 0;
};
