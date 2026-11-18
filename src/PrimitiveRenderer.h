#pragma once

#include "LineAlgorithm.h"
#include "LineSegment.h"
#include "Point2D.h"

#include <SFML/Graphics.hpp>

#include <optional>
#include <vector>

// Result of validating a polygon before drawing it.
enum class PolygonCheck {
    Ok,
    TooFewPoints,     // fewer than 3 distinct vertices
    SelfIntersecting, // two segments cross / overlap - not a simple polygon
    NotClosed,        // segments do not form one closed chain
};
const char* toString(PolygonCheck check);

// Draws primitives (pixels, lines, polylines, circles, ellipses, polygons,
// fills) onto any render target: the window's back buffer, a triple-buffer
// frame or an off-screen bitmap (sf::RenderTexture). Cheap to construct -
// create one per frame.
//
// Coordinates are in pixels: point (x, y) is the center of pixel (x, y).
class PrimitiveRenderer {
public:
    explicit PrimitiveRenderer(sf::RenderTarget& target) : m_target(target) {}

    // Blend mode used for everything drawn afterwards (default: alpha blending)
    void setBlendMode(const sf::BlendMode& mode) { m_states.blendMode = mode; }
    sf::RenderTarget& getTarget() { return m_target; }

    // --- Points ---
    void drawPixel(int x, int y, sf::Color color);
    void drawPoint(const Point2D& point, sf::Color color, float size = 1.f);

    // --- Line segments ---
    // Library algorithm (SFML / OpenGL line rasterization)
    void drawLine(sf::Vector2f start, sf::Vector2f end, sf::Color color);
    // Incremental algorithm (DDA): steps one pixel along the major axis and
    // adds the slope to the other coordinate each step
    void drawLineIncremental(sf::Vector2f start, sf::Vector2f end, sf::Color color);

    void drawLine(const Point2D& start, const Point2D& end, sf::Color color,
                  LineAlgorithm algorithm = LineAlgorithm::Default);
    void drawLine(const LineSegment& segment, sf::Color color,
                  LineAlgorithm algorithm = LineAlgorithm::Default);

    // --- Polylines (open chains; no intersection check) ---
    // Connects consecutive points; closed == true also joins last -> first
    void drawPolyline(const std::vector<Point2D>& points, sf::Color color, bool closed = false,
                      LineAlgorithm algorithm = LineAlgorithm::Default);
    // Draws every segment of the set
    void drawPolyline(const std::vector<LineSegment>& segments, sf::Color color,
                      LineAlgorithm algorithm = LineAlgorithm::Default);

    // --- Circles and ellipses ---
    // Library algorithm (sf::CircleShape / sf::Shape outline or fill)
    void drawCircle(sf::Vector2f center, float radius, sf::Color color, bool filled = true);
    void drawEllipse(sf::Vector2f center, float radiusX, float radiusY, sf::Color color, bool filled = false);
    // Own algorithm: points (R cos a, R sin a) for a in [0, 45 deg] with step
    // 1/R (about one pixel of arc), mirrored with 8-fold symmetry
    void drawCircleIncremental(sf::Vector2f center, float radius, sf::Color color);
    // Own algorithm: points (Rx cos a, Ry sin a) for a in [0, 90 deg] with step
    // 1/max(Rx, Ry), mirrored with 4-fold symmetry
    void drawEllipseIncremental(sf::Vector2f center, float radiusX, float radiusY, sf::Color color);

    // --- Polygons ---
    // Closed simple polygon outline. If any two segments intersect (or the
    // segments do not form a closed chain) nothing is drawn and false is returned.
    bool drawPolygon(const std::vector<Point2D>& points, sf::Color color,
                     LineAlgorithm algorithm = LineAlgorithm::Default);
    bool drawPolygon(const std::vector<LineSegment>& segments, sf::Color color,
                     LineAlgorithm algorithm = LineAlgorithm::Default);
    static PolygonCheck validatePolygon(const std::vector<Point2D>& points);
    static PolygonCheck validatePolygon(const std::vector<LineSegment>& segments);

    // --- Filled primitives ---
    void drawRectangle(sf::Vector2f position, sf::Vector2f size, sf::Color color, bool filled = true);
    void drawTriangle(const Point2D& a, const Point2D& b, const Point2D& c, sf::Color color,
                      bool filled = true);
    // Library fill of a convex polygon (sf::ConvexShape)
    void fillConvexPolygon(const std::vector<Point2D>& points, sf::Color color);
    // Own scan-line fill with the parity (even-odd) rule: any polygon,
    // convex, concave or even self-intersecting. Returns false for < 3 points.
    bool fillPolygon(const std::vector<Point2D>& points, sf::Color color);

    // --- Bitmaps ---
    // Draws part 'rect' of a texture; 'origin' (in rect pixels) is placed at
    // the transform's origin, then the transform is applied
    void drawBitmap(const sf::Texture& texture, sf::IntRect rect, const sf::Transform& transform,
                    sf::Vector2f origin = {});
    void drawBitmap(const sf::Texture& texture, sf::Vector2f position);

    // --- Area filling (seed fill, 4-connected) ---
    // Reads the target's current pixels, so draw the outline first.
    // Flood fill: recolours the connected area that has the seed pixel's colour.
    // Returns the number of pixels changed.
    unsigned floodFill(int x, int y, sf::Color fillColor);
    // Boundary fill: recolours everything connected to the seed up to pixels
    // of the boundary colour (whatever colour they had before).
    unsigned boundaryFill(int x, int y, sf::Color fillColor, sf::Color boundaryColor);

    // The same algorithms working directly on an image; changed pixels are
    // appended to 'changed' when it is given.
    static unsigned floodFill(sf::Image& image, sf::Vector2u seed, sf::Color fillColor,
                              std::vector<sf::Vector2u>* changed = nullptr);
    static unsigned boundaryFill(sf::Image& image, sf::Vector2u seed, sf::Color fillColor,
                                 sf::Color boundaryColor, std::vector<sf::Vector2u>* changed = nullptr);

private:
    // Pixel (x, y) covers [x, x+1) x [y, y+1); its center is at +0.5
    static sf::Vector2f pixelCenter(int x, int y) { return {x + 0.5f, y + 0.5f}; }
    void incrementalPixels(sf::Vector2f start, sf::Vector2f end, sf::Color color,
                           sf::VertexArray& out) const;
    std::optional<sf::Image> captureTarget();
    void drawChangedPixels(const std::vector<sf::Vector2u>& pixels, sf::Color color);

    sf::RenderTarget& m_target;
    sf::RenderStates m_states = sf::RenderStates::Default;
};
