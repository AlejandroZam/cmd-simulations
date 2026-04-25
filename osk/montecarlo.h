#pragma once
#include <cstdint>
#include <string>
#include <yaml-cpp/yaml.h>

// Monte Carlo run control. Read from params.yaml `monte_carlo:` section.
// Seed for run N = baseSeed + N, giving deterministic, reproducible draws.
//
// YAML:
//   monte_carlo:
//     runs: 50
//     base_seed: 42
//
// If the section is absent, defaults to 1 run with seed 0 (no noise variation).
struct MonteCarlo {
    static int      runs;        // total number of runs
    static uint64_t baseSeed;    // starting seed
    static int      currentRun;  // set by the run loop in main.cpp

    static uint64_t seedForRun(int run) {
        return baseSeed + static_cast<uint64_t>(run);
    }

    static void loadConfig(const YAML::Node& cfg) {
        if (!cfg["monte_carlo"]) return;
        auto mc  = cfg["monte_carlo"];
        runs     = mc["runs"].as<int>(1);
        baseSeed = mc["base_seed"].as<uint64_t>(0);
    }
};

inline int      MonteCarlo::runs       = 1;
inline uint64_t MonteCarlo::baseSeed   = 0;
inline int      MonteCarlo::currentRun = 0;
