#pragma once

#include <SFML/Graphics.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

class Logger;

// Owns the game's bitmaps, addressed by name.
//
// Each bitmap is kept as an sf::Image (CPU side: pixels can be read and
// edited) and, on demand, as an sf::Texture (GPU side: needed to draw it).
// Editing a bitmap marks its texture out of date; getTexture() re-uploads it
// into the SAME texture object, so pointers held by sprites stay valid until
// the bitmap is removed.
class BitmapHandler {
public:
    explicit BitmapHandler(Logger* log = nullptr) : m_log(log) {}

    // --- Creating / deleting ---
    bool create(const std::string& name, sf::Vector2u size, sf::Color color = sf::Color::Transparent);
    bool create(const std::string& name, const sf::Image& image); // takes a copy
    bool remove(const std::string& name);
    void clear(); // deletes every bitmap (called by Engine::shutdown)
    bool exists(const std::string& name) const;
    std::vector<std::string> names() const;

    // --- Files (png, bmp, tga, jpg, ...) ---
    bool load(const std::string& name, const std::string& path);
    bool save(const std::string& name, const std::string& path) const;

    // --- Copying ---
    // Whole bitmap into a new / replaced one
    bool copy(const std::string& source, const std::string& destination);
    // Rectangle of 'source' onto 'destination' at 'destPosition'.
    // applyAlpha = true blends by the source alpha, false copies raw pixels.
    bool copyRegion(const std::string& source, sf::IntRect sourceRect, const std::string& destination,
                    sf::Vector2u destPosition, bool applyAlpha = false);
    // Rectangle of 'source' as a new bitmap
    bool extract(const std::string& source, sf::IntRect sourceRect, const std::string& destination);

    // --- Editing ---
    bool fill(const std::string& name, sf::Color color);
    bool flipHorizontally(const std::string& name);
    bool flipVertically(const std::string& name);
    bool setPixel(const std::string& name, sf::Vector2u position, sf::Color color);
    sf::Color getPixel(const std::string& name, sf::Vector2u position) const;

    // --- Access ---
    sf::Vector2u getSize(const std::string& name) const;
    const sf::Image* getImage(const std::string& name) const;
    // GPU copy for drawing (uploaded on first use / after edits); nullptr if missing
    const sf::Texture* getTexture(const std::string& name);

private:
    struct Entry {
        sf::Image image;
        std::unique_ptr<sf::Texture> texture;
        bool textureDirty = true;
    };

    Entry* find(const std::string& name, const char* operation);
    const Entry* find(const std::string& name, const char* operation) const;
    void error(const std::string& message) const;

    std::map<std::string, Entry> m_bitmaps;
    Logger* m_log = nullptr;
};
