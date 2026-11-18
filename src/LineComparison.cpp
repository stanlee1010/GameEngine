#include "LineComparison.h"

#include "Logger.h"
#include "PrimitiveRenderer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>

namespace {

constexpr unsigned kCanvasSize = 128;

struct TestCase {
    const char* name;
    sf::Vector2i start;
    sf::Vector2i end;
};

const TestCase kCases[] = {
    {"single point",            {64, 64},  {64, 64}},
    {"horizontal L->R",         {10, 60},  {110, 60}},
    {"horizontal R->L",         {110, 60}, {10, 60}},
    {"vertical T->B",           {60, 10},  {60, 110}},
    {"vertical B->T",           {60, 110}, {60, 10}},
    {"diagonal 45 deg",         {10, 10},  {110, 110}},
    {"diagonal -45 deg",        {10, 110}, {110, 10}},
    {"gentle +slope (0.3)",     {10, 40},  {110, 70}},
    {"gentle -slope (-0.3)",    {10, 70},  {110, 40}},
    {"gentle, R->L",            {110, 70}, {10, 40}},
    {"steep +slope (3.3)",      {40, 10},  {70, 110}},
    {"steep -slope (-3.3)",     {70, 10},  {40, 110}},
    {"steep, B->T",             {70, 110}, {40, 10}},
    {"almost horizontal",       {5, 50},   {120, 53}},
    {"almost vertical",         {50, 5},   {53, 120}},
};

// Renders a segment into the canvas and returns which pixels are lit
std::vector<bool> rasterize(sf::RenderTexture& canvas, const std::function<void(PrimitiveRenderer&)>& draw)
{
    canvas.clear(sf::Color::Black);
    PrimitiveRenderer renderer(canvas);
    draw(renderer);
    canvas.display();

    const sf::Image image = canvas.getTexture().copyToImage();
    std::vector<bool> lit(kCanvasSize * kCanvasSize, false);
    for (unsigned y = 0; y < kCanvasSize; ++y)
        for (unsigned x = 0; x < kCanvasSize; ++x)
            lit[y * kCanvasSize + x] = image.getPixel({x, y}).r > 127;
    return lit;
}

bool isLit(const std::vector<bool>& lit, int x, int y)
{
    return x >= 0 && y >= 0 && x < static_cast<int>(kCanvasSize) && y < static_cast<int>(kCanvasSize) &&
           lit[y * kCanvasSize + x];
}

// A correct rasterized line lights exactly one pixel per step on the major
// axis, so every row (or column) between the ends has exactly one pixel.
bool isConnected(const std::vector<bool>& lit, sf::Vector2i a, sf::Vector2i b)
{
    const bool xMajor = std::abs(b.x - a.x) >= std::abs(b.y - a.y);
    const int from = xMajor ? std::min(a.x, b.x) : std::min(a.y, b.y);
    const int to = xMajor ? std::max(a.x, b.x) : std::max(a.y, b.y);
    for (int major = from; major <= to; ++major) {
        int count = 0;
        for (int minor = 0; minor < static_cast<int>(kCanvasSize); ++minor)
            count += xMajor ? isLit(lit, major, minor) : isLit(lit, minor, major);
        if (count != 1)
            return false;
    }
    return true;
}

} // namespace

std::vector<LineComparisonResult> compareLineAlgorithms()
{
    std::vector<LineComparisonResult> results;
    sf::RenderTexture canvas;
    if (!canvas.resize({kCanvasSize, kCanvasSize}))
        return results;

    for (const TestCase& test : kCases) {
        const sf::Vector2f a(test.start), b(test.end);
        const auto byDefault = rasterize(canvas, [&](PrimitiveRenderer& r) { r.drawLine(a, b, sf::Color::White); });
        const auto byIncremental = rasterize(canvas, [&](PrimitiveRenderer& r) { r.drawLineIncremental(a, b, sf::Color::White); });

        LineComparisonResult result;
        result.name = test.name;
        result.start = test.start;
        result.end = test.end;
        result.expectedPixels = static_cast<unsigned>(
            std::max(std::abs(test.end.x - test.start.x), std::abs(test.end.y - test.start.y)) + 1);
        for (std::size_t i = 0; i < byDefault.size(); ++i) {
            result.defaultPixels += byDefault[i];
            result.incrementalPixels += byIncremental[i];
            result.commonPixels += byDefault[i] && byIncremental[i];
        }
        result.incrementalHasEnds = isLit(byIncremental, test.start.x, test.start.y) &&
                                    isLit(byIncremental, test.end.x, test.end.y);
        result.incrementalConnected = isConnected(byIncremental, test.start, test.end);
        results.push_back(result);
    }
    return results;
}

void logLineComparison(Logger& log, const std::vector<LineComparisonResult>& results)
{
    log.info("Line algorithm comparison (library vs incremental), pixels lit:");
    log.info("  case                   | expected | library | increm. | common | match | ends+connected");
    unsigned passed = 0;
    for (const LineComparisonResult& r : results) {
        const unsigned total = std::max(r.defaultPixels, r.incrementalPixels);
        const float match = total ? 100.f * static_cast<float>(r.commonPixels) / static_cast<float>(total) : 100.f;
        const bool ok = r.incrementalPixels == r.expectedPixels && r.incrementalHasEnds && r.incrementalConnected;
        passed += ok;
        char line[160];
        std::snprintf(line, sizeof(line), "  %-22s | %8u | %7u | %7u | %6u | %4.0f%% | %s",
                      r.name.c_str(), r.expectedPixels, r.defaultPixels, r.incrementalPixels,
                      r.commonPixels, match, ok ? "OK" : "FAIL");
        log.info(line);
    }
    log.info("Incremental algorithm: " + std::to_string(passed) + "/" + std::to_string(results.size()) +
             " cases correct");
}
