#pragma once

#include "InputListener.h"
#include "Shapes.h"
#include "SpriteObject.h"
#include "UpdatableObject.h"

#include <string>
#include <vector>

// A projectile: a circle that also has game logic.
// UpdatableObject + CircleObject (-> ShapeObject -> Drawable/Transformable):
// four paths lead to GameObject, all virtual, so there is one GameObject.
class Bullet : public UpdatableObject, public CircleObject {
public:
    Bullet(sf::Vector2f position, sf::Vector2f velocity);

    const char* getTypeName() const override { return "Bullet"; }
    void update(float dt) override;

private:
    sf::Vector2f m_velocity;
    float m_lifetime = 2.f; // seconds; deactivates itself afterwards
};

// The player, drawn and animated with bitmaps (sprites).
//  - game logic   (UpdatableObject): moves, picks the walk animation, shoots
//  - sprites      (SpriteObject -> BitmapObject + AnimatedObject):
//                 two layers - a shadow bitmap and the animated body sprite
//  - transforms   (TransformableObject, via BitmapObject): translate / scale
//  - input        (InputListener): mouse click shoots, wheel scales
//
// Controls: WASD / arrows walk in four directions (the matching walk
// animation plays; standing still shows the first frame), left click or
// Space shoots towards the mouse, mouse wheel scales.
//
// Sprite sheet: assets/player_sheet.png, 4 rows (right, left, up, down) x
// 4 frames. Generated on first run if missing; replace it with your own art.
class Player : public UpdatableObject, public SpriteObject, public InputListener {
public:
    enum class Direction { Right = 0, Left = 1, Up = 2, Down = 3 };

    static constexpr const char* kSheetPath = "assets/player_sheet.png";

    explicit Player(sf::Vector2f position);

    const char* getTypeName() const override { return "Player"; }

    // UpdatableObject
    void update(float dt) override;
    // DrawableObject: bullets, then the bitmap layers (BitmapObject::draw)
    void draw(PrimitiveRenderer& renderer) const override;

    // InputListener
    void onMouseButtonPressed(sf::Mouse::Button button, sf::Vector2i position) override;
    void onMouseWheelScrolled(float delta, sf::Vector2i position) override;

    Direction getDirection() const { return m_direction; }
    static const char* toString(Direction direction);
    // Current size compared with the normal size (1.0 = normal, 2.0 = twice as big)
    float getSizeFactor() const { return getScale().x / kBaseScale; }
    const std::vector<Bullet>& getBullets() const { return m_bullets; }

    // Keeps the player inside this area (e.g. the window)
    void setBounds(sf::Vector2f size) { m_bounds = size; }

private:
    static constexpr float kBaseScale = 1.5f; // normal size: the 48 px sprite drawn 1.5x bigger

    void loadSprites();
    void shoot();

    Direction m_direction = Direction::Down;
    float m_speed = 180.f; // pixels per second
    float m_cooldown = 0.f;
    sf::Vector2f m_bounds{800.f, 600.f};
    std::vector<Bullet> m_bullets;
};
