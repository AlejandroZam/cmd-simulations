#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <yaml-cpp/yaml.h>

#include "sim.h"
#include "factory.h"
#include "montecarlo.h"
#include "spring_mass_damper.h"
#include "missile.h"
#include "target.h"

namespace fs = std::filesystem;

// Resolve a model config path: if the scenario YAML specifies one, resolve it
// relative to the scenario directory; otherwise return empty (model uses its default).
static std::string resolveConfig(const YAML::Node& entry,
                                 const fs::path& scenarioDir)
{
    if (!entry["config"]) return {};
    return (scenarioDir / entry["config"].as<std::string>()).string();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: sim <scenario_name>\n"
                  << "  Reads  INPUT_DATA/<scenario_name>/scenario.yaml\n"
                  << "  Writes OUTPUT_DATA/<scenario_name>/\n"
                  << "  Run from the CMD repo root.\n";
        return 1;
    }

    const std::string scenarioName = argv[1];
    const fs::path    scenarioDir  = fs::path("INPUT_DATA") / scenarioName;
    const fs::path    scenarioYaml = scenarioDir / "scenario.yaml";
    const fs::path    outputBase   = fs::path("OUTPUT_DATA") / scenarioName;

    YAML::Node cfg;
    try {
        cfg = YAML::LoadFile(scenarioYaml.string());
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load " << scenarioYaml << ": " << e.what() << "\n";
        return 1;
    }

    // ── Sim-level settings ────────────────────────────────────────────────────
    double      tmax      = cfg["simulation"]["tmax"].as<double>();
    double      dt        = cfg["simulation"]["dt"].as<double>();
    std::string methodStr = cfg["simulation"]["integration_method"].as<std::string>("RK4");

    IntegMethod method = IntegMethod::RK4;
    if      (methodStr == "EULER") method = IntegMethod::EULER;
    else if (methodStr == "RK2")   method = IntegMethod::RK2;

    // ── Monte Carlo settings ──────────────────────────────────────────────────
    MonteCarlo::loadConfig(cfg);

    // ── Output settings ───────────────────────────────────────────────────────
    std::string fmtStr = cfg["output"]["format"].as<std::string>("csv");
    LogFormat   logFmt = (fmtStr == "bin") ? LogFormat::BIN : LogFormat::CSV;

    // ── Register all known model types ────────────────────────────────────────
    ModelFactory::reg<SpringMassDamper>("SpringMassDamper");
    ModelFactory::reg<Missile>("Missile");
    ModelFactory::reg<Target>("Target");

    // ── Collect enabled model entries ─────────────────────────────────────────
    struct ModelEntry { std::string name, type, config; };
    std::vector<ModelEntry> entries;
    for (auto e : cfg["models"]) {
        if (!e["enabled"].as<bool>(true)) continue;
        entries.push_back({ e["name"].as<std::string>(),
                            e["type"].as<std::string>(),
                            resolveConfig(e, scenarioDir) });
    }
    if (entries.empty()) {
        std::cerr << "No models enabled in scenario " << scenarioName << ".\n";
        return 1;
    }

    // ── Monte Carlo run loop ──────────────────────────────────────────────────
    for (int run = 0; run < MonteCarlo::runs; ++run) {
        MonteCarlo::currentRun = run;
        uint64_t runSeed = MonteCarlo::seedForRun(run);

        fs::path runOutDir = (MonteCarlo::runs > 1)
                           ? outputBase / ("run" + std::to_string(run))
                           : outputBase;
        fs::create_directories(runOutDir);

        if (MonteCarlo::runs > 1)
            std::printf("\n=== Monte Carlo run %d (seed %llu) ===\n",
                        run, static_cast<unsigned long long>(runSeed));

        // Instantiate models
        std::vector<Block*> allModels;
        std::vector<Block*> stage0;
        Target*  target  = nullptr;
        Missile* missile = nullptr;

        for (auto& e : entries) {
            Block* b = ModelFactory::create(e.type);
            if (!e.config.empty()) b->setConfigPath(e.config);
            b->name      = e.name;
            b->outputDir = runOutDir.string();
            b->logFmt    = logFmt;
            b->seed(runSeed);

            if (auto* t = dynamic_cast<Target*>(b))  target  = t;
            if (auto* m = dynamic_cast<Missile*>(b)) missile = m;

            allModels.push_back(b);
            stage0.push_back(b);
        }

        // Inter-model wiring (temporary — will be replaced by FastDDS pub/sub)
        if (missile && target) missile->getsFrom(target);

        std::vector<std::vector<Block*>> vStage = { stage0 };
        double dts[] = { dt };

        Sim* sim = new Sim(dts, tmax, vStage, method);
        sim->run();

        delete sim;
        for (auto* b : allModels) delete b;
    }

    return 0;
}
