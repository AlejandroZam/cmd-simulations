#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <yaml-cpp/yaml.h>

#include "sim.h"
#include "simdds.h"
#include "viz_bridge.h"
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

    // ── Collect enabled model entries (sorted by optional 'order' field) ────────
    struct ModelEntry { std::string name, type, config; int order; };
    std::vector<ModelEntry> entries;
    for (auto e : cfg["models"]) {
        if (!e["enabled"].as<bool>(true)) continue;
        if (!e["order"]) {
            std::cerr << "Model '" << e["name"].as<std::string>()
                      << "' is missing required field 'order' in " << scenarioYaml << "\n";
            return 1;
        }
        entries.push_back({ e["name"].as<std::string>(),
                            e["type"].as<std::string>(),
                            resolveConfig(e, scenarioDir),
                            e["order"].as<int>() });
    }
    std::stable_sort(entries.begin(), entries.end(),
                     [](const ModelEntry& a, const ModelEntry& b){ return a.order < b.order; });
    if (entries.empty()) {
        std::cerr << "No models enabled in scenario " << scenarioName << ".\n";
        return 1;
    }

    // ── DDS domain participant ────────────────────────────────────────────────
    SimDDS::get().init();

    // ── Visualization bridge (UDP, optional) ──────────────────────────────────
    if (cfg["visualization"] && cfg["visualization"]["enabled"].as<bool>(false)) {
        auto   v    = cfg["visualization"];
        auto   host = v["host"].as<std::string>("127.0.0.1");
        auto   port = static_cast<uint16_t>(v["port"].as<int>(9999));
        VizBridge::get().init(host.c_str(), port);
        std::printf("[viz] Streaming to UDP %s:%d\n", host.c_str(), port);
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

        for (auto& e : entries) {
            Block* b = ModelFactory::create(e.type);
            if (!e.config.empty()) b->setConfigPath(e.config);
            b->name      = e.name;
            b->outputDir = runOutDir.string();
            b->logFmt    = logFmt;
            b->seed(runSeed);

            allModels.push_back(b);
            stage0.push_back(b);
        }

        std::vector<std::vector<Block*>> vStage = { stage0 };
        double dts[] = { dt };

        VizBridge::get().sendEvent(VizBridge::MSG_RUN_START, static_cast<uint16_t>(run));
        Sim* sim = new Sim(dts, tmax, vStage, method);
        sim->run();
        delete sim;
        for (auto* b : allModels) delete b;
        VizBridge::get().sendEvent(VizBridge::MSG_RUN_END, static_cast<uint16_t>(run));
    }

    SimDDS::get().shutdown();
    VizBridge::get().shutdown();
    return 0;
}
