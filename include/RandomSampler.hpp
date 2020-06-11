#pragma once
#include <random>

class RandomSampler {
public:
    RandomSampler() = delete;

    static float randomFloat();
    static float randomFloat(float a, float b);
    static void seed();

private:
    static bool bSeeded;
    static std::mt19937 randomEngine;
    static std::uniform_real_distribution<float> randomDist;
};
