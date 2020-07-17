#include "RandomSampler.hpp"

bool RandomSampler::bSeeded = false;
std::mt19937 RandomSampler::randomEngine;

float RandomSampler::randomFloat() {
    return RandomSampler::randomFloat(0.0f, 1.0f);
}

float RandomSampler::randomFloat(float a, float b) {
    if (!RandomSampler::bSeeded) {
        RandomSampler::seed();
    }
    std::uniform_real_distribution dist(a, b);
    return dist(RandomSampler::randomEngine);
}

void RandomSampler::seed() {
    std::random_device rd;
    RandomSampler::randomEngine.seed(rd());
    bSeeded = true;
}
