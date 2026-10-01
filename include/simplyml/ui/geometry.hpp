#pragma once
#include <cstddef>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

namespace sml::geo
{

// Mesh builders. Each appends triangles to `va`, which must use PrimitiveType::Triangles, so any
// mix of shapes batches into a single draw call.

void rect(sf::VertexArray& va, sf::FloatRect r, sf::Color color);

/// Filled rounded rectangle. `radius` is clamped to half the smaller side.
void roundedRect(sf::VertexArray& va, sf::FloatRect r, float radius, sf::Color color);

/// Rounded outline lying inside `r`: outer edge on `r` with `radius`, `thickness` wide.
void roundedRing(sf::VertexArray& va, sf::FloatRect r, float radius, float thickness, sf::Color color);

/// Soft shadow around `r`, fading from `color` at the edge to transparent `size` px outside.
void roundedShadow(sf::VertexArray& va, sf::FloatRect r, float radius, float size, sf::Color color);

/// Straight segment `width` px wide.
void line(sf::VertexArray& va, sf::Vector2f a, sf::Vector2f b, float width, sf::Color color);

void circle(sf::VertexArray& va, sf::Vector2f center, float radius, sf::Color color, unsigned segments = 32);

/// Circle outline `thickness` wide, inside `radius`.
void ring(sf::VertexArray& va, sf::Vector2f center, float radius, float thickness, sf::Color color,
          unsigned segments = 32);

/// Line with a triangular head at `to`.
void arrow(sf::VertexArray& va, sf::Vector2f from, sf::Vector2f to, float width, float headSize, sf::Color color);

/// Quadratic Bézier from `a` to `c` with control point `b`.
void bezier(sf::VertexArray& va, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, float width, sf::Color color,
            unsigned segments = 24);

/// Thick polyline with mitred joins (miters limited to 2x the width at sharp turns).
/// `widths`/`colors` are per point when non-null, else `width`/`color` apply to every point.
/// Fewer than two points draw nothing; repeated points are skipped.
void polyline(sf::VertexArray& va, sf::Vector2f const* points, std::size_t count, float width, sf::Color color,
              float const* widths = nullptr, sf::Color const* colors = nullptr);

/// Area between a polyline and the horizontal line `baseY`, colored per point (top) and `baseColor`.
void area(sf::VertexArray& va, sf::Vector2f const* points, std::size_t count, float baseY,
          sf::Color const* topColors, sf::Color baseColor);

/// Corner arc vertex count used by rounded shapes for `radius` px (more for bigger radii).
[[nodiscard]] unsigned arcSegments(float radius);

} // namespace sml::geo
