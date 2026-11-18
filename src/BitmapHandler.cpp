#include "BitmapHandler.h"

#include "Logger.h"

#include <filesystem>

void BitmapHandler::error(const std::string& message) const
{
    if (m_log)
        m_log->error("BitmapHandler: " + message);
}

BitmapHandler::Entry* BitmapHandler::find(const std::string& name, const char* operation)
{
    const auto it = m_bitmaps.find(name);
    if (it == m_bitmaps.end()) {
        error(std::string(operation) + ": no bitmap named '" + name + "'");
        return nullptr;
    }
    return &it->second;
}

const BitmapHandler::Entry* BitmapHandler::find(const std::string& name, const char* operation) const
{
    return const_cast<BitmapHandler*>(this)->find(name, operation);
}

// ---------------------------------------------------------------------------
// Creating / deleting
// ---------------------------------------------------------------------------

bool BitmapHandler::create(const std::string& name, sf::Vector2u size, sf::Color color)
{
    if (size.x == 0 || size.y == 0) {
        error("create '" + name + "': size must be non-zero");
        return false;
    }
    return create(name, sf::Image(size, color));
}

bool BitmapHandler::create(const std::string& name, const sf::Image& image)
{
    Entry& entry = m_bitmaps[name]; // replaces an existing bitmap's pixels
    entry.image = image;
    entry.textureDirty = true;
    return true;
}

bool BitmapHandler::remove(const std::string& name)
{
    if (m_bitmaps.erase(name) == 0) {
        error("remove: no bitmap named '" + name + "'");
        return false;
    }
    return true;
}

void BitmapHandler::clear()
{
    m_bitmaps.clear();
}

bool BitmapHandler::exists(const std::string& name) const
{
    return m_bitmaps.count(name) != 0;
}

std::vector<std::string> BitmapHandler::names() const
{
    std::vector<std::string> result;
    for (const auto& [name, entry] : m_bitmaps)
        result.push_back(name);
    return result;
}

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------

// BITMAP LOADING: read an image file (png, bmp, jpg...) from disk into a named bitmap
bool BitmapHandler::load(const std::string& name, const std::string& path)
{
    if (!std::filesystem::exists(path)) {
        error("load: file '" + path + "' does not exist");
        return false;
    }
    sf::Image image;
    if (!image.loadFromFile(path)) {
        error("load: could not read '" + path + "' (unsupported or corrupt image)");
        return false;
    }
    return create(name, image);
}

// BITMAP SAVING: write a named bitmap to an image file (folders are created if needed)
bool BitmapHandler::save(const std::string& name, const std::string& path) const
{
    const Entry* entry = find(name, "save");
    if (!entry)
        return false;
    const std::filesystem::path parent = std::filesystem::path(path).parent_path();
    std::error_code ec;
    if (!parent.empty())
        std::filesystem::create_directories(parent, ec);
    if (!entry->image.saveToFile(path)) {
        error("save: could not write '" + path + "'");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Copying
// ---------------------------------------------------------------------------

// BITMAP MANIPULATION: copy a whole bitmap into another one
bool BitmapHandler::copy(const std::string& source, const std::string& destination)
{
    const Entry* src = find(source, "copy");
    if (!src)
        return false;
    const sf::Image copyOfImage = src->image; // copy first: source may equal destination
    return create(destination, copyOfImage);
}

// BITMAP MANIPULATION: copy a rectangle of one bitmap onto another (refuses if it does not fit)
bool BitmapHandler::copyRegion(const std::string& source, sf::IntRect sourceRect, const std::string& destination,
                               sf::Vector2u destPosition, bool applyAlpha)
{
    const Entry* src = find(source, "copyRegion");
    Entry* dst = find(destination, "copyRegion");
    if (!src || !dst)
        return false;
    // Whole source when the rectangle is empty
    const sf::Vector2i sourceSize(src->image.getSize());
    if (sourceRect.size.x <= 0 || sourceRect.size.y <= 0)
        sourceRect = sf::IntRect({0, 0}, sourceSize);

    // SFML would silently clip; a partial copy is almost always a bug, so refuse
    const sf::Vector2i destSize(dst->image.getSize());
    const bool sourceInside = sourceRect.position.x >= 0 && sourceRect.position.y >= 0 &&
                              sourceRect.position.x + sourceRect.size.x <= sourceSize.x &&
                              sourceRect.position.y + sourceRect.size.y <= sourceSize.y;
    const bool destInside = static_cast<int>(destPosition.x) + sourceRect.size.x <= destSize.x &&
                            static_cast<int>(destPosition.y) + sourceRect.size.y <= destSize.y;
    if (!sourceInside || !destInside) {
        error("copyRegion: '" + source + "' -> '" + destination + "' does not fit (out of bounds)");
        return false;
    }

    // Copy the source first so a bitmap can be copied onto itself safely
    const sf::Image sourceImage = src->image;
    if (!dst->image.copy(sourceImage, destPosition, sourceRect, applyAlpha)) {
        error("copyRegion: '" + source + "' -> '" + destination + "' is out of bounds");
        return false;
    }
    dst->textureDirty = true;
    return true;
}

// BITMAP MANIPULATION: cut a rectangle out of a bitmap as a new bitmap (e.g. one sprite frame)
bool BitmapHandler::extract(const std::string& source, sf::IntRect sourceRect, const std::string& destination)
{
    const Entry* src = find(source, "extract");
    if (!src)
        return false;
    if (sourceRect.size.x <= 0 || sourceRect.size.y <= 0) {
        error("extract: empty rectangle");
        return false;
    }
    sf::Image part(sf::Vector2u(sourceRect.size), sf::Color::Transparent);
    if (!part.copy(src->image, {0, 0}, sourceRect)) {
        error("extract: rectangle outside '" + source + "'");
        return false;
    }
    return create(destination, part);
}

// ---------------------------------------------------------------------------
// Editing
// ---------------------------------------------------------------------------

// BITMAP MANIPULATION: fill the whole bitmap with one colour
bool BitmapHandler::fill(const std::string& name, sf::Color color)
{
    Entry* entry = find(name, "fill");
    if (!entry)
        return false;
    entry->image = sf::Image(entry->image.getSize(), color);
    entry->textureDirty = true;
    return true;
}

// BITMAP MANIPULATION: mirror left <-> right
bool BitmapHandler::flipHorizontally(const std::string& name)
{
    Entry* entry = find(name, "flipHorizontally");
    if (!entry)
        return false;
    entry->image.flipHorizontally();
    entry->textureDirty = true;
    return true;
}

// BITMAP MANIPULATION: mirror top <-> bottom
bool BitmapHandler::flipVertically(const std::string& name)
{
    Entry* entry = find(name, "flipVertically");
    if (!entry)
        return false;
    entry->image.flipVertically();
    entry->textureDirty = true;
    return true;
}

// BITMAP MANIPULATION: change one pixel
bool BitmapHandler::setPixel(const std::string& name, sf::Vector2u position, sf::Color color)
{
    Entry* entry = find(name, "setPixel");
    if (!entry)
        return false;
    const sf::Vector2u size = entry->image.getSize();
    if (position.x >= size.x || position.y >= size.y) {
        error("setPixel: position outside '" + name + "'");
        return false;
    }
    entry->image.setPixel(position, color);
    entry->textureDirty = true;
    return true;
}

sf::Color BitmapHandler::getPixel(const std::string& name, sf::Vector2u position) const
{
    const Entry* entry = find(name, "getPixel");
    if (!entry)
        return sf::Color::Transparent;
    const sf::Vector2u size = entry->image.getSize();
    if (position.x >= size.x || position.y >= size.y)
        return sf::Color::Transparent;
    return entry->image.getPixel(position);
}

// ---------------------------------------------------------------------------
// Access
// ---------------------------------------------------------------------------

sf::Vector2u BitmapHandler::getSize(const std::string& name) const
{
    const Entry* entry = find(name, "getSize");
    return entry ? entry->image.getSize() : sf::Vector2u{};
}

const sf::Image* BitmapHandler::getImage(const std::string& name) const
{
    const Entry* entry = find(name, "getImage");
    return entry ? &entry->image : nullptr;
}

// BITMAP DISPLAYING (step 1): the image (CPU) must be uploaded to the graphics card as a texture before it
// can be drawn. It is re-uploaded only after an edit (textureDirty), into the same texture object.
const sf::Texture* BitmapHandler::getTexture(const std::string& name)
{
    Entry* entry = find(name, "getTexture");
    if (!entry)
        return nullptr;
    if (!entry->texture)
        entry->texture = std::make_unique<sf::Texture>();
    if (entry->textureDirty) {
        if (!entry->texture->loadFromImage(entry->image)) {
            error("getTexture: could not upload '" + name + "' to the GPU");
            return nullptr;
        }
        entry->texture->setSmooth(false); // keep pixel art sharp
        entry->textureDirty = false;
    }
    return entry->texture.get();
}
