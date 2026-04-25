#pragma once
#include <random>
#include <cmath>
#include <string>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

enum class NoiseDist { NONE, GAUSSIAN, UNIFORM, LAPLACE, WEIBULL };

// Per-model noise generator. One instance per noise channel.
// Configure from a named YAML subkey (see examples below).
//
// Gaussian:
//   distribution: gaussian
//   mean:   0.0
//   stddev: 0.05
//
// Uniform:
//   distribution: uniform
//   min: -0.1
//   max:  0.1
//
// Laplace (double-exponential):
//   distribution: laplace
//   location: 0.0   # mean / center
//   scale:    0.05  # b parameter; variance = 2*b^2
//
// Weibull:
//   distribution: weibull
//   shape: 2.0      # k  (k=1 → exponential, k~3.5 → near-normal)
//   scale: 1.0      # lambda
class NoiseGen {
public:
    NoiseGen() : dist_(NoiseDist::NONE), id_(nextId()) {}

    void loadConfig(const YAML::Node& node) {
        if (!node || !node["distribution"]) { dist_ = NoiseDist::NONE; return; }
        std::string d = node["distribution"].as<std::string>("none");
        if (d == "gaussian") {
            dist_   = NoiseDist::GAUSSIAN;
            mean_   = node["mean"].as<double>(0.0);
            stddev_ = node["stddev"].as<double>(1.0);
        } else if (d == "uniform") {
            dist_ = NoiseDist::UNIFORM;
            lo_   = node["min"].as<double>(0.0);
            hi_   = node["max"].as<double>(1.0);
        } else if (d == "laplace") {
            dist_     = NoiseDist::LAPLACE;
            location_ = node["location"].as<double>(0.0);
            scaleB_   = node["scale"].as<double>(1.0);
        } else if (d == "weibull") {
            dist_    = NoiseDist::WEIBULL;
            shape_   = node["shape"].as<double>(1.0);
            scaleW_  = node["scale"].as<double>(1.0);
        } else {
            dist_ = NoiseDist::NONE;
        }
    }

    // Seed deterministically. Each NoiseGen instance has a unique internal ID
    // so two channels with the same run seed stay statistically independent.
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
            case NoiseDist::LAPLACE: {
                // Inverse CDF: X = location - scale*sign(U)*ln(1 - 2|U|), U~Uniform(-0.5,0.5)
                std::uniform_real_distribution<double> ud(-0.5, 0.5);
                double u = ud(rng_);
                double sign = (u >= 0.0) ? 1.0 : -1.0;
                return location_ - scaleB_ * sign * std::log(1.0 - 2.0 * std::fabs(u));
            }
            case NoiseDist::WEIBULL: {
                std::weibull_distribution<double> wd(shape_, scaleW_);
                return wd(rng_);
            }
            default: return 0.0;
        }
    }

    NoiseDist distribution() const { return dist_; }

private:
    static uint64_t nextId() { static uint64_t c = 0; return c++; }

    NoiseDist       dist_     = NoiseDist::NONE;
    // Gaussian
    double          mean_     = 0.0;
    double          stddev_   = 1.0;
    // Uniform
    double          lo_       = 0.0;
    double          hi_       = 1.0;
    // Laplace
    double          location_ = 0.0;
    double          scaleB_   = 1.0;
    // Weibull
    double          shape_    = 1.0;
    double          scaleW_   = 1.0;

    uint64_t        id_;
    std::mt19937_64 rng_;
};
