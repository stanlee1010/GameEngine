#pragma once

#include <SFML/System/Vector2.hpp>

#include <string>

class BitmapHandler;

// Generates simple original bitmaps for the demo, drawn with the
// PrimitiveRenderer into off-screen bitmaps, so the engine runs without any
// asset files. Replace the saved PNGs with your own art in the same layout.
namespace DemoAssets {

// Player sprite sheet: 4 rows (0 right, 1 left, 2 up, 3 down) x 4 walking
// frames, each cell cellSize x cellSize. The left row is the right row
// copied and mirrored with the BitmapHandler.
bool createPlayerSheet(BitmapHandler& bitmaps, const std::string& name, unsigned cellSize = 48);

// Soft oval shadow drawn under characters (a second bitmap layer)
bool createShadow(BitmapHandler& bitmaps, const std::string& name, sf::Vector2u size = {34, 10});

// Wooden crate and a star emblem: a two-layer bitmap object
bool createCrate(BitmapHandler& bitmaps, const std::string& name, unsigned size = 44);
bool createStar(BitmapHandler& bitmaps, const std::string& name, unsigned size = 22);

} // namespace DemoAssets
