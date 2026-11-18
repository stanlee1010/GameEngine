#include "SpriteObject.h"

// ANIMATION: the sprite sheet = one big image containing all the frames
void SpriteObject::setSpriteSheet(const sf::Texture& sheet, sf::Vector2f offset)
{
    if (m_spriteLayer < getLayerCount()) {
        getLayer(m_spriteLayer).texture = &sheet;
        getLayer(m_spriteLayer).offset = offset;
    } else {
        m_spriteLayer = addLayer(sheet, {}, offset);
    }
    showFrame();
}

// ANIMATION: register a named animation (list of frames + speed + loop)
void SpriteObject::addAnimation(const std::string& name, SpriteAnimation animation)
{
    m_animations[name] = std::move(animation);
    if (m_current.empty())
        play(name, true);
}

// ANIMATION: cut a sheet row into frames: frame i = rectangle at (i * width, row * height)
std::vector<sf::IntRect> SpriteObject::gridFrames(sf::Vector2i cellSize, int row, int count, int firstColumn)
{
    std::vector<sf::IntRect> frames;
    for (int column = firstColumn; column < firstColumn + count; ++column)
        frames.emplace_back(sf::Vector2i{column * cellSize.x, row * cellSize.y}, cellSize);
    return frames;
}

// ANIMATION: start an animation (e.g. "walk_left")
void SpriteObject::play(const std::string& name, bool restart)
{
    if (!hasAnimation(name))
        return;
    if (name != m_current || restart) {
        m_current = name;
        m_frame = 0;
        m_time = 0.f;
    }
    m_playing = true;
    showFrame();
}

// ANIMATION: freeze on the current frame
void SpriteObject::stop()
{
    m_playing = false;
}

// ANIMATION: jump to a frame (frame 0 = standing pose)
void SpriteObject::setFrame(std::size_t index)
{
    const auto it = m_animations.find(m_current);
    if (it == m_animations.end() || it->second.frames.empty())
        return;
    m_frame = index % it->second.frames.size();
    m_time = 0.f;
    showFrame();
}

// ANIMATION: the illusion of movement. Called every frame with dt; when enough time has passed
// (1 / framesPerSecond) the next image is shown, after the last one it loops back to frame 0.
void SpriteObject::animate(float dt)
{
    const auto it = m_animations.find(m_current);
    if (!m_playing || it == m_animations.end() || it->second.frames.empty() || it->second.framesPerSecond <= 0.f)
        return;

    const SpriteAnimation& animation = it->second;
    const float frameTime = 1.f / animation.framesPerSecond;
    m_time += dt;
    while (m_time >= frameTime) {
        m_time -= frameTime;
        if (m_frame + 1 < animation.frames.size()) {
            ++m_frame;
        } else if (animation.loop) {
            m_frame = 0;
        } else {
            m_playing = false; // one-shot animation ends on its last frame
            break;
        }
    }
    showFrame();
}

// ANIMATION: show the current frame = point the sprite layer at that rectangle of the sheet
void SpriteObject::showFrame()
{
    const auto it = m_animations.find(m_current);
    if (m_spriteLayer >= getLayerCount() || it == m_animations.end() || it->second.frames.empty())
        return;
    BitmapLayer& layer = getLayer(m_spriteLayer);
    layer.rect = it->second.frames[m_frame % it->second.frames.size()];
    layer.origin = sf::Vector2f(layer.rect.size) / 2.f;
}
