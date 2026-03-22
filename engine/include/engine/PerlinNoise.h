#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace engine {

/// 2D Perlin noise generator with deterministic output for a given seed.
/// Returns values approximately in the range [-1, 1].
class PerlinNoise {
  public:
    /// Construct with a seed for the permutation table.
    explicit PerlinNoise(uint64_t seed);

    /// Sample 2D noise at (x, y). Returns approximately [-1, 1].
    [[nodiscard]] float sample(float x, float y) const;

  private:
    static constexpr int TABLE_SIZE = 256;
    static constexpr int TABLE_MASK = TABLE_SIZE - 1;

    /// Doubled permutation table (512 entries) to avoid index wrapping.
    static constexpr std::size_t DOUBLED_TABLE_SIZE = static_cast<std::size_t>(TABLE_SIZE) * 2;
    std::array<int, DOUBLED_TABLE_SIZE> perm_{};

    /// Gradient dot product for a hash and offset vector.
    [[nodiscard]] static float grad(int hash, float x, float y);

    /// Fade/ease curve: 6t^5 - 15t^4 + 10t^3
    [[nodiscard]] static float fade(float t);

    /// Linear interpolation.
    [[nodiscard]] static float lerp(float a, float b, float t);
};

} // namespace engine
