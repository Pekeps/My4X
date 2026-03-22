#pragma once

namespace engine::hex_metrics {

/// Hex radius (canonical constant for all hex geometry calculations).
static constexpr float HEX_RADIUS = 1.0F;

/// Fraction of the hex radius that remains solid-colored (no blending).
static constexpr float SOLID_FACTOR = 0.75F;

/// Fraction of the hex radius used for blend regions.
static constexpr float BLEND_FACTOR = 1.0F - SOLID_FACTOR;

/// World-space height per elevation level.
static constexpr float ELEVATION_STEP = 0.15F;

/// Number of terrace steps between two elevation levels.
static constexpr int TERRACE_STEPS = 2;

/// Total interpolation segments for terrace geometry (2 * steps + 1).
static constexpr int TERRACE_INTERPOLATION_STEPS = (TERRACE_STEPS * 2) + 1;

/// Horizontal interpolation increment per terrace step.
static constexpr float HORIZONTAL_TERRACE_STEP = 1.0F / static_cast<float>(TERRACE_INTERPOLATION_STEPS);

/// Vertical interpolation increment per terrace step (steps in pairs).
static constexpr float VERTICAL_TERRACE_STEP = 1.0F / static_cast<float>(TERRACE_STEPS + 1);

/// Perlin noise perturbation strength (XZ displacement in world units).
/// Tutorial uses ~40% of outer radius for visible irregularity.
static constexpr float PERTURBATION_STRENGTH = 0.35F;

/// Perlin noise sampling scale (controls spatial frequency of noise).
/// Higher = features change faster across the map.
static constexpr float NOISE_SCALE = 0.8F;

/// Elevation perturbation strength (Y displacement per cell).
static constexpr float ELEVATION_PERTURBATION_STRENGTH = 0.05F;

/// Number of edge subdivisions (creates this many + 1 vertices per edge).
static constexpr int EDGE_SUBDIVISIONS = 3;

/// Offset to sample a second independent noise axis (avoids correlation).
static constexpr float NOISE_AXIS_OFFSET = 100.0F;

/// Scale factor for elevation noise sampling (coarser than vertex noise).
static constexpr float ELEVATION_NOISE_SCALE = 0.5F;

} // namespace engine::hex_metrics
