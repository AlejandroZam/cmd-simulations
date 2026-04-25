#pragma once
#include <random>
#include <string>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

enum class NoiseDist { NONE, GAUSSIAN, UNIFORM };

// Per-model noise generator. Configure from a YAML `noise:` subkey.
// YAML example (gaussian):
//   noise:
//     distribution: gaussian
//     mean: 0.0
//     stddev: 0.05
// YAML example (uniform):
//   noise:
//     distribution: uniform
//     min: -0.1
//     max:  0.1
class NoiseGen {
public:
    NoiseGen() : dist_(NoiseDist::NONE), mean_(0.0), stddev_(1.0),
                 lo_(0.0), hi_(1.0), id_(nextId()) {}

    void loadConfig(const YAML::Node& node) {
        if (!node || !node["distribution"]) return;
        std::string d = node["distribution"].as<std::string>("none");
        if (d == "gaussian") {
            dist_   = NoiseDist::GAUSSIAN;
            mean_   = node["mean"].as<double>(0.0);
            stddev_ = node["stddev"].as<double>(1.0);
        } else if (d == "uniform") {
            dist_ = NoiseDist::UNIFORM;
            lo_   = node["min"].as<double>(0.0);
            hi_   = node["max"].as<double>(1.0);
        } else {
            dist_ = NoiseDist::NONE;
        }
    }

    // Seed deterministically: call with baseSeed + currentRun.
    // Each NoiseGen instance gets an additional unique offset so
    // two models seeded with the same run seed stay independent.
    void seed(uint64_t s) {
        rng_.seed(s ^ (id_ * 2654435761ULL));
    }

    double sample() {
        switch (dist_) {
            case NoiseDist::GAUSSIAN: {
                std::normal_distribution<double> nd(mean_, stddev_);
                return nd(rng_);
            }
            case NoiseDist::UNIFORM: {
                std::uniform_real_distribution<double> ud(lo_, hi_);
                return ud(rng_);
            }
            default: return 0.0;
        }
    }

    NoiseDist distribution() const { return dist_; }

private:
    static uint64_t nextId() { static uint64_t c = 0; return c++; }

    NoiseDist          dist_;
    double             mean_, stddev_, lo_, hi_;
    uint64_t           id_;
    std::mt19937_64    rng_;
};
