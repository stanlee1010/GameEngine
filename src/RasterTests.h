#pragma once

#include <string>
#include <vector>

class Logger;

// One automatic check of a drawing algorithm, run on an off-screen bitmap.
struct RasterTestResult {
    std::string group;   // "Circle", "Ellipse", "Polygon", "Fill", "Scanline"
    std::string name;    // what was drawn
    std::string details; // measured numbers, incl. comparison with the library
    bool passed = false;
};

// Circle / ellipse: own algorithm vs the library's shape outline.
//   Checks: closed curve (no gaps), symmetry, max distance from the true
//   curve <= 1 px, and how many pixels both algorithms share.
std::vector<RasterTestResult> testCirclesAndEllipses();

// Polygon validation (self-intersection / closed chain) and that a rejected
// polygon draws nothing; flood fill vs boundary fill; parity scan-line fill
// vs the library's convex fill.
std::vector<RasterTestResult> testPolygonsAndFills();

// Geometric transformations (translate / rotate / scale) of Point2D,
// LineSegment and shapes, and the virtual inheritance (one GameObject).
std::vector<RasterTestResult> testTransformations();

// BitmapHandler: create, save + load round trip, copy, extract, flip,
// remove, missing files, texture upload; mirrored sprite-sheet rows.
std::vector<RasterTestResult> testBitmaps();

void logRasterTests(Logger& log, const std::vector<RasterTestResult>& results);
