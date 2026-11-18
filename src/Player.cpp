#include "Player.h"

#include "DemoAssets.h"
#include "Engine.h"
#include "PrimitiveRenderer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace {

const char* const kSheet = "player_sheet";
const char* const kShadow = "player_shadow";
const char* const kAnimationNames[] = {"walk_right", "walk_left", "walk_up", "walk_down"};

} // namespace

// ---------------------------------------------------------------------------
// Bullet
// ---------------------------------------------------------------------------

Bullet::Bullet(sf::Vector2f position, sf::Vector2f velocity) : CircleObject(position, 3.f), m_velocity(velocity)
{
    setColor(sf::Color(255, 230, 120));
    setFillColor(sf::Color(255, 200, 60));
    setFilled(true);
}

void Bullet::update(float dt)
{
    translate(m_velocity.x * dt, m_velocity.y * dt);
    m_lifetime -= dt;
    if (m_lifetime <= 0.f)
        setActive(false);
}

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------

Player::Player(sf::Vector2f position)
{
    setPosition(position);
    setScale({kBaseScale, kBaseScale});
    loadSprites();
    play(kAnimationNames[static_cast<int>(m_direction)]);
    stop(); // stand still until moved
}

const char* Player::toString(Direction direction)
{
    switch (direction) {
    case Direction::Right: return "right";
    case Direction::Left:  return "left";
    case Direction::Up:    return "up";
    case Direction::Down:  return "down";
    }
    return "?";
}

void Player::loadSprites()
{
    Engine& engine = Engine::getInstance();
    BitmapHandler& bitmaps = engine.getBitmaps();

    // BITMAP LOADING / SAVING in practice: load the sheet from the file, or generate it and save it
    // Sprite sheet: from the file if it exists, otherwise generate and save it
    if (!bitmaps.exists(kSheet)) {
        if (!std::filesystem::exists(kSheetPath) || !bitmaps.load(kSheet, kSheetPath)) {
            engine.getLogger().info(std::string("Generating player sprite sheet '") + kSheetPath + "'");
            if (DemoAssets::createPlayerSheet(bitmaps, kSheet))
                bitmaps.save(kSheet, kSheetPath);
        }
    }
    if (!bitmaps.exists(kShadow))
        DemoAssets::createShadow(bitmaps, kShadow);

    const sf::Texture* sheet = bitmaps.getTexture(kSheet);
    const sf::Texture* shadow = bitmaps.getTexture(kShadow);
    if (!sheet) {
        engine.reportError("Player: no sprite sheet available");
        return;
    }

    // Layer 0: shadow at the feet; layer 1: the animated body
    const sf::Vector2i cell(sf::Vector2i(sheet->getSize()) / 4);
    if (shadow)
        addLayer(*shadow, {}, {0.f, static_cast<float>(cell.y) * 0.4f});
    setSpriteSheet(*sheet);

    // One walking animation per row of the sheet
    for (int row = 0; row < 4; ++row)
        addAnimation(kAnimationNames[row], {gridFrames(cell, row, 4), 8.f, true});
}

void Player::update(float dt)
{
    const Engine& engine = Engine::getInstance();
    using Key = sf::Keyboard::Key;

    // Movement: keyboard polling, four directions
    sf::Vector2f move;
    if (engine.isKeyDown(Key::A) || engine.isKeyDown(Key::Left))  move.x -= 1.f;
    if (engine.isKeyDown(Key::D) || engine.isKeyDown(Key::Right)) move.x += 1.f;
    if (engine.isKeyDown(Key::W) || engine.isKeyDown(Key::Up))    move.y -= 1.f;
    if (engine.isKeyDown(Key::S) || engine.isKeyDown(Key::Down))  move.y += 1.f;

    if (move.x != 0.f || move.y != 0.f) {
        // Facing: the dominant axis (horizontal wins ties, as most games do)
        if (move.x != 0.f)
            m_direction = move.x > 0.f ? Direction::Right : Direction::Left;
        else
            m_direction = move.y > 0.f ? Direction::Down : Direction::Up;
        play(kAnimationNames[static_cast<int>(m_direction)]); // keeps the frame if already walking that way

        move /= std::hypot(move.x, move.y); // same speed diagonally
        translate(move.x * m_speed * dt, move.y * m_speed * dt); // TRANSLATION: walking
    } else if (isPlaying()) {
        stop();
        setFrame(0); // standing pose
    }

    // Stay inside the bounds
    const sf::Vector2f c = getCenter();
    const sf::Vector2f clamped{std::clamp(c.x, 0.f, m_bounds.x), std::clamp(c.y, 0.f, m_bounds.y)};
    if (clamped != c)
        setPosition(clamped);

    // Shooting with the left mouse button / Space held
    m_cooldown = std::max(0.f, m_cooldown - dt);
    if (engine.isMouseButtonDown(sf::Mouse::Button::Left) || engine.isKeyDown(Key::Space))
        shoot();

    for (Bullet& b : m_bullets)
        b.update(dt);
    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(),
                                   [&](const Bullet& b) {
                                       const sf::Vector2f p = b.getCenter();
                                       return !b.isActive() || p.x < -10.f || p.y < -10.f ||
                                              p.x > m_bounds.x + 10.f || p.y > m_bounds.y + 10.f;
                                   }),
                    m_bullets.end());
}

void Player::shoot()
{
    if (m_cooldown > 0.f)
        return;
    m_cooldown = 0.15f;

    // Towards the mouse; if it is on the player, in the facing direction
    sf::Vector2f dir = sf::Vector2f(Engine::getInstance().getMousePosition()) - getCenter();
    float length = std::hypot(dir.x, dir.y);
    if (length < 4.f) {
        const sf::Vector2f facing[] = {{1.f, 0.f}, {-1.f, 0.f}, {0.f, -1.f}, {0.f, 1.f}};
        dir = facing[static_cast<int>(m_direction)];
        length = 1.f;
    }
    dir /= length;
    m_bullets.emplace_back(getCenter() + dir * (20.f * getSizeFactor()), dir * 500.f);
}

void Player::draw(PrimitiveRenderer& renderer) const
{
    for (const Bullet& b : m_bullets)
        b.draw(renderer);
    SpriteObject::draw(renderer); // shadow + current sprite frame
}

void Player::onMouseButtonPressed(sf::Mouse::Button button, sf::Vector2i /*position*/)
{
    if (button == sf::Mouse::Button::Left)
        shoot(); // immediate shot on click; holding keeps firing in update()
}

// RESIZING: the mouse wheel scales the player (scene 6).
// Each notch up = 10% bigger, each notch down = 10% smaller.
void Player::onMouseWheelScrolled(float delta, sf::Vector2i /*position*/)
{
    const float factor = delta > 0.f ? 1.1f : 1.f / 1.1f; // wheel up: x1.1, wheel down: /1.1
    const float next = getSizeFactor() * factor;          // what the size would become
    if (next < 0.5f || next > 3.f)                        // limits: half size .. 3x size
        return;
    scaleAroundCenter(factor); // TransformableObject: scale about the centre, so it does not jump
}
