#include "Random.h"

namespace DDD {

std::mt19937 Random::s_engine;

bool Random::s_seeded = false;

void Random::Seed(unsigned int seed) {
    s_engine.seed(seed);
    s_seeded = true;
}

void Random::SeedWithTime() {
    s_engine.seed(static_cast<unsigned int>(std::time(nullptr)));
    s_seeded = true;
}

std::mt19937& Random::GetEngine() {
    if (!s_seeded) {
        SeedWithTime();
    }
    return s_engine;
}

int Random::Range(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(GetEngine());
}

float Random::Range(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(GetEngine());
}

int Random::Int(int min, int max) {
    return Range(min, max);
}

float Random::Float(float min, float max) {
    return Range(min, max);
}

bool Random::Bool() {
    return Range(0, 1) == 1;
}

float Random::Chance(float probability) {
    return Range(0.0f, 1.0f) < probability;
}

} // namespace DDD
