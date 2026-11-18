#pragma once

// Which rasterization algorithm is used to draw a line segment.
//  Default     - the graphics library's own line drawing (SFML -> OpenGL GL_LINES)
//  Incremental - our own incremental (DDA) algorithm, pixel by pixel
enum class LineAlgorithm { Default, Incremental };
