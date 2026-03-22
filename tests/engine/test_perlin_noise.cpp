// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/PerlinNoise.h"
#include <gtest/gtest.h>

#include <cmath>

TEST(PerlinNoiseTest, DeterministicSameSeed) {
    engine::PerlinNoise n1(42);
    engine::PerlinNoise n2(42);

    EXPECT_FLOAT_EQ(n1.sample(1.5F, 2.3F), n2.sample(1.5F, 2.3F));
    EXPECT_FLOAT_EQ(n1.sample(-3.7F, 0.1F), n2.sample(-3.7F, 0.1F));
    EXPECT_FLOAT_EQ(n1.sample(100.0F, 200.0F), n2.sample(100.0F, 200.0F));
}

TEST(PerlinNoiseTest, DifferentSeedsGiveDifferentValues) {
    engine::PerlinNoise n1(42);
    engine::PerlinNoise n2(123);

    // Extremely unlikely that two different seeds produce the same output
    // at multiple sample points.
    bool allSame = true;
    float points[][2] = {{1.5F, 2.3F}, {-3.7F, 0.1F}, {10.0F, 20.0F}};
    for (auto &p : points) {
        if (n1.sample(p[0], p[1]) != n2.sample(p[0], p[1])) {
            allSame = false;
            break;
        }
    }
    EXPECT_FALSE(allSame);
}

TEST(PerlinNoiseTest, OutputRange) {
    engine::PerlinNoise noise(99);

    float minVal = 1.0F;
    float maxVal = -1.0F;
    for (float x = -10.0F; x <= 10.0F; x += 0.37F) {
        for (float y = -10.0F; y <= 10.0F; y += 0.37F) {
            float val = noise.sample(x, y);
            if (val < minVal) minVal = val;
            if (val > maxVal) maxVal = val;
        }
    }

    // Perlin noise output should be in approximately [-1, 1].
    // With 4 gradient vectors, the theoretical range is [-1, 1].
    EXPECT_GE(minVal, -1.5F);
    EXPECT_LE(maxVal, 1.5F);
    // Should actually use a good range of the space.
    EXPECT_LT(minVal, 0.0F);
    EXPECT_GT(maxVal, 0.0F);
}

TEST(PerlinNoiseTest, SmoothnessNearbySamplesAreClose) {
    engine::PerlinNoise noise(7);

    float step = 0.001F;
    float x = 3.5F;
    float y = 2.1F;
    float val = noise.sample(x, y);
    float valDx = noise.sample(x + step, y);
    float valDy = noise.sample(x, y + step);

    // Nearby samples should be very close (Perlin noise is C2 continuous).
    EXPECT_NEAR(val, valDx, 0.01F);
    EXPECT_NEAR(val, valDy, 0.01F);
}

TEST(PerlinNoiseTest, IntegerCoordinatesReturnZero) {
    // At integer coordinates, all gradient dot products have zero fractional
    // components, so Perlin noise returns 0.
    engine::PerlinNoise noise(42);
    EXPECT_FLOAT_EQ(noise.sample(0.0F, 0.0F), 0.0F);
    EXPECT_FLOAT_EQ(noise.sample(1.0F, 1.0F), 0.0F);
    EXPECT_FLOAT_EQ(noise.sample(5.0F, 3.0F), 0.0F);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
