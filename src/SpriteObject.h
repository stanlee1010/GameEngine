#pragma once

#include "AnimatedObject.h"
#include "BitmapObject.h"

#include <map>
#include <string>
#include <vector>

// A named sequence of frames taken from a sprite sheet.
struct SpriteAnimation {
    std::vector<sf::IntRect> frames;
    float framesPerSecond = 8.f;
    bool loop = true;
};

// Bitmap object animated with sprites: one of its layers (the sprite layer)
// shows the current frame of the current animation; other layers stay as
// they are. BitmapObject + AnimatedObject -> one GameObject (virtual).
class SpriteObject : public BitmapObject, public AnimatedObject {
public:
    // Uses (or creates) the sprite layer showing frames of 'sheet'
    void setSpriteSheet(const sf::Texture& sheet, sf::Vector2f offset = {});

    void addAnimation(const std::string& name, SpriteAnimation animation);
    bool hasAnimation(const std::string& name) const { return m_animations.count(name) != 0; }

    // Frames laid out in a grid: 'count' cells of 'cellSize' in row 'row'
    static std::vector<sf::IntRect> gridFrames(sf::Vector2i cellSize, int row, int count, int firstColumn = 0);

    // Starts an animation; playing the current one again keeps its frame
    // unless restart is true
    void play(const std::string& name, bool restart = false);
    void stop(); // freeze on the current frame
    void setFrame(std::size_t index);

    bool isPlaying() const { return m_playing; }
    const std::string& getAnimation() const { return m_current; }
    std::size_t getFrame() const { return m_frame; }

    // AnimatedObject: advances the frame by elapsed time
    void animate(float dt) override;

private:
    void showFrame();

    std::map<std::string, SpriteAnimation> m_animations;
    std::string m_current;
    std::size_t m_frame = 0;
    float m_time = 0.f;
    bool m_playing = false;
    std::size_t m_spriteLayer = static_cast<std::size_t>(-1);
};
