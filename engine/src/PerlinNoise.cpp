#include "engine/PerlinNoise.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace engine {

// Named constants for Perlin noise implementation.
static constexpr float FADE_COEFF_A = 6.0F;
static constexpr float FADE_COEFF_B = 15.0F;
static constexpr float FADE_COEFF_C = 10.0F;
static constexpr int GRAD_MASK = 3;

PerlinNoise::PerlinNoise(uint64_t seed) {
    // Fill with 0..255, then shuffle with the given seed.
    std::array<int, TABLE_SIZE> base{};
    std::ranges::iota(base, 0);

    std::mt19937_64 rng(seed);
    std::ranges::shuffle(base, rng);

    // Double the table so we can index without modular arithmetic.
    for (int i = 0; i < TABLE_SIZE; ++i) {
        perm_.at(i) = base.at(i);
        perm_.at(i + TABLE_SIZE) = base.at(i);
    }
}

float PerlinNoise::fade(float t) {
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * FADE_COEFF_A - FADE_COEFF_B) + FADE_COEFF_C);
}

float PerlinNoise::lerp(float a, float b, float t) { return a + (t * (b - a)); }

float PerlinNoise::grad(int hash, float x, float y) {
    // Use bottom 2 bits to select one of 4 gradient vectors:
    // (1,1), (-1,1), (1,-1), (-1,-1)
    switch (hash & GRAD_MASK) {
    case 0:
        return x + y;
    case 1:
        return -x + y;
    case 2:
        return x - y;
    default:
        return -x - y;
    }
}

float PerlinNoise::sample(float x, float y) const {
    // Integer cell coordinates (floored).
    int xi = static_cast<int>(std::floor(x)) & TABLE_MASK;
    int yi = static_cast<int>(std::floor(y)) & TABLE_MASK;

    // Fractional position within cell.
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);

    // Fade curves for interpolation weights.
    float u = fade(xf);
    float v = fade(yf);

    // Hash the four corners.
    int aa = perm_.at(perm_.at(xi) + yi);
    int ab = perm_.at(perm_.at(xi) + yi + 1);
    int ba = perm_.at(perm_.at(xi + 1) + yi);
    int bb = perm_.at(perm_.at(xi + 1) + yi + 1);

    // Bilinear interpolation of gradient dot products.
    float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0F, yf), u);
    float x2 = lerp(grad(ab, xf, yf - 1.0F), grad(bb, xf - 1.0F, yf - 1.0F), u);

    return lerp(x1, x2, v);
}

} // namespace engine
