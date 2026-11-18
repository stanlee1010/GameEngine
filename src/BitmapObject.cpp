#include "BitmapObject.h"

#include "PrimitiveRenderer.h"

std::size_t BitmapObject::addLayer(const sf::Texture& texture, sf::IntRect rect, sf::Vector2f offset)
{
    BitmapLayer layer;
    layer.texture = &texture;
    layer.rect = (rect.size.x > 0 && rect.size.y > 0) ? rect : sf::IntRect({0, 0}, sf::Vector2i(texture.getSize()));
    layer.origin = sf::Vector2f(layer.rect.size) / 2.f;
    layer.offset = offset;
    return addLayer(layer);
}

std::size_t BitmapObject::addLayer(const BitmapLayer& layer)
{
    m_layers.push_back(layer);
    return m_layers.size() - 1;
}

// ROTATION of a bitmap: move its position around the pivot and turn the image
void BitmapObject::rotate(float degrees, sf::Vector2f pivot)
{
    m_position = rotatePoint(m_position, degrees, pivot);
    m_rotation += degrees;
}

// SCALING of a bitmap: move its position and enlarge/shrink the image
void BitmapObject::scale(float sx, float sy, sf::Vector2f pivot)
{
    m_position = scalePoint(m_position, sx, sy, pivot);
    m_scale.x *= sx;
    m_scale.y *= sy;
}

sf::Transform BitmapObject::getTransform() const
{
    sf::Transform transform;
    transform.translate(m_position);
    transform.rotate(sf::degrees(m_rotation));
    transform.scale(m_scale);
    return transform;
}

// BITMAP DISPLAYING (step 2): draw every visible layer with the object's position/rotation/scale
void BitmapObject::draw(PrimitiveRenderer& renderer) const
{
    const sf::Transform transform = getTransform();
    for (const BitmapLayer& layer : m_layers) {
        if (!layer.visible || !layer.texture)
            continue;
        sf::Transform layerTransform = transform;
        layerTransform.translate(layer.offset);
        renderer.drawBitmap(*layer.texture, layer.rect, layerTransform, layer.origin);
    }
}
