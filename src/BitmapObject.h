#pragma once

#include "DrawableObject.h"
#include "TransformableObject.h"

#include <SFML/Graphics.hpp>

#include <vector>

// One bitmap of a BitmapObject. An object may consist of several layers
// (e.g. shadow + body + hat), drawn in order, each with its own offset.
struct BitmapLayer {
    const sf::Texture* texture = nullptr; // usually owned by the BitmapHandler
    sf::IntRect rect;                     // part of the texture to show (empty = whole)
    sf::Vector2f origin;                  // point of the rect placed at the offset
    sf::Vector2f offset;                  // position relative to the object (local, before transform)
    bool visible = true;
};

// Base of every object drawn with bitmaps. Holds a list of layers and a
// transformation (position, rotation, scale) that is applied to all of them.
class BitmapObject : public DrawableObject, public TransformableObject {
public:
    // Adds a layer; rect empty = whole texture, origin defaults to the rect's center
    std::size_t addLayer(const sf::Texture& texture, sf::IntRect rect = {}, sf::Vector2f offset = {});
    std::size_t addLayer(const BitmapLayer& layer);
    BitmapLayer& getLayer(std::size_t index) { return m_layers.at(index); }
    const BitmapLayer& getLayer(std::size_t index) const { return m_layers.at(index); }
    std::size_t getLayerCount() const { return m_layers.size(); }
    void clearLayers() { m_layers.clear(); }

    // DrawableObject: every visible layer, bottom first
    void draw(PrimitiveRenderer& renderer) const override;

    // TransformableObject
    void translate(float dx, float dy) override { m_position += {dx, dy}; } // TRANSLATION
    void rotate(float degrees, sf::Vector2f pivot) override;
    void scale(float sx, float sy, sf::Vector2f pivot) override;
    sf::Vector2f getCenter() const override { return m_position; }

    sf::Vector2f getPosition() const { return m_position; }
    void setPosition(sf::Vector2f position) { m_position = position; }
    float getRotation() const { return m_rotation; }
    void setRotation(float degrees) { m_rotation = degrees; }
    sf::Vector2f getScale() const { return m_scale; }
    void setScale(sf::Vector2f scale) { m_scale = scale; }

    // position * rotation * scale, applied to every layer
    sf::Transform getTransform() const;

private:
    std::vector<BitmapLayer> m_layers;
    sf::Vector2f m_position;
    float m_rotation = 0.f; // degrees
    sf::Vector2f m_scale{1.f, 1.f};
};
