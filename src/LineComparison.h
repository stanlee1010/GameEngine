#pragma once

#include <SFML/System/Vector2.hpp>

#include <string>
#include <vector>

class Logger;

// Result of rasterizing one segment with both line algorithms.
struct LineComparisonResult {
    std::string name;
    sf::Vector2i start;
    sf::Vector2i end;
    unsigned expectedPixels = 0;     // max(|dx|, |dy|) + 1
    unsigned defaultPixels = 0;      // lit by the library (OpenGL) algorithm
    unsigned incrementalPixels = 0;  // lit by our incremental algorithm
    unsigned commonPixels = 0;       // lit by both
    bool incrementalHasEnds = false; // both end pixels drawn
    bool incrementalConnected = true;// no gaps (8-connected, 1 pixel per step)
};

// Draws a set of test segments covering every inclination (horizontal,
// vertical, diagonal, gentle/steep, all octants, both directions, a single
// point) into off-screen bitmaps with both algorithms and compares the pixels.
// Requires a graphics context (call after Engine::init()).
std::vector<LineComparisonResult> compareLineAlgorithms();

void logLineComparison(Logger& log, const std::vector<LineComparisonResult>& results);
