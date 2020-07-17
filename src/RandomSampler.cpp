#include "RandomSampler.hpp"

bool RandomSampler::bSeeded = false;
std::mt19937 RandomSampler::randomEngine;
std::uniform_real_distribution<float> RandomSampler::randomDist;

float RandomSampler::randomFloat() {
    if (!RandomSampler::bSeeded) {
        RandomSampler::seed();
    }
    return RandomSampler::randomDist(RandomSampler::randomEngine);
}

float RandomSampler::randomFloat(float a, float b) {
    if (!RandomSampler::bSeeded) {
        RandomSampler::seed();
    }
    return RandomSampler::randomDist(RandomSampler::randomEngine) * (b - a) + a;
}

void RandomSampler::seed() {
    std::random_device rd;
    RandomSampler::randomEngine.seed(rd());
    bSeeded = true;
}
