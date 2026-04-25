#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <yaml-cpp/yaml.h>

#include "sim.h"
#include "factory.h"
#include "logger.h"
#include "montecarlo.h"
#include "spring_mass_damper.h"

namespace fs = std::filesystem;

// Helper: cast Block* to SpringMassDamper* and set meta fields.
// Extend this (or use a base interface) when more model types are added.
static void applyMeta(Block* b, const std::string& name,
                      const std::string& outDir, LogFormat fmt)
{
    if (auto* smd = dynamic_cast<SpringMassDamper*>(b)) {
        smd->name      = name;
        smd->outputDir = outDir;
        smd->logFmt    = fmt;
    }
}

int main(int argc, char* argv[]) {
    const char* simCfg = (argc > 1) ? argv[1] : "params.yaml";

    YAML::Node cfg;
    try {
        cfg = YAML::LoadFile(simCfg);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load " << simCfg << ": " << e.what() << "\n";
        return 1;
    }

    // --- Sim-level settings ---
    double      tmax      = cfg["simulation"]["tmax"].as<double>();
    double      dt        = cfg["simulation"]["dt"].as<double>();
    std::string methodStr = cfg["simulation"]["integration_method"].as<std::string>("RK4");

    IntegMethod method = IntegMethod::RK4;
    if      (methodStr == "EULER") method = IntegMethod::EULER;
    else if (methodStr == "RK2")   method = IntegMethod::RK2;

    // --- Monte Carlo settings ---
    MonteCarlo::loadConfig(cfg);

    // --- Output settings ---
    std::string outDir = cfg["output"]["dir"].as<std::string>("output");
    std::string fmtStr = cfg["output"]["format"].as<std::string>("csv");
    LogFormat   logFmt = (fmtStr == "bin") ? LogFormat::BIN : LogFormat::CSV;

    // --- Register model types ---
    ModelFactory::reg<SpringMassDamper>("SpringMassDamper");

    // --- Collect enabled model entries from YAML ---
    struct ModelEntry { std::string name, type, config; };
    std::vector<ModelEntry> entries;
    for (auto e : cfg["models"]) {
        if (!e["enabled"].as<bool>(true)) continue;
        entries.push_back({ e["name"].as<std::string>(),
                            e["type"].as<std::string>(),
                            e["config"].as<std::string>() });
    }
    if (entries.empty()) {
        std::cerr << "No models enabled — nothing to simulate.\n";
        return 1;
    }

    // --- Monte Carlo run loop ---
    for (int run = 0; run < MonteCarlo::runs; ++run) {
        MonteCarlo::currentRun = run;
        uint64_t runSeed = MonteCarlo::seedForRun(run);

        // Output sub-directory per run when runs > 1
        std::string runOutDir = outDir;
        if (MonteCarlo::runs > 1)
            runOutDir = outDir + "/run" + std::to_string(run);
        fs::create_directories(runOutDir);

        if (MonteCarlo::runs > 1)
            std::printf("\n=== Monte Carlo run %d (seed %llu) ===\n",
                        run, static_cast<unsigned long long>(runSeed));

        // Fresh model instances each run so initCount and state reset cleanly
        std::vector<Block*> allModels;
        std::vector<Block*> stage0;

        for (auto& e : entries) {
            Block* b = ModelFactory::create(e.type);
            b->setConfigPath(e.config);
            applyMeta(b, e.name, runOutDir, logFmt);

            // Seed noise deterministically for this run
            if (auto* smd = dynamic_cast<SpringMassDamper*>(b))
                smd->seed(runSeed);

            allModels.push_back(b);
            stage0.push_back(b);
        }

        std::vector<std::vector<Block*>> vStage = { stage0 };
        double dts[] = { dt };

        Sim* sim = new Sim(dts, tmax, vStage, method);
        sim->run();

        delete sim;
        for (auto* b : allModels) delete b;
    }

    return 0;
}
