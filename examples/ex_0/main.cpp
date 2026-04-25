#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <yaml-cpp/yaml.h>

#include "sim.h"
#include "factory.h"
#include "logger.h"
#include "spring_mass_damper.h"

namespace fs = std::filesystem;

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

    // --- Output settings ---
    std::string outDir    = cfg["output"]["dir"].as<std::string>("output");
    std::string fmtStr    = cfg["output"]["format"].as<std::string>("csv");
    LogFormat   logFmt    = (fmtStr == "bin") ? LogFormat::BIN : LogFormat::CSV;
    fs::create_directories(outDir);

    // --- Register model types ---
    ModelFactory::reg<SpringMassDamper>("SpringMassDamper");

    // --- Instantiate models from YAML list ---
    std::vector<Block*> allModels;
    std::vector<Block*> stage0;

    for (auto entry : cfg["models"]) {
        bool enabled = entry["enabled"].as<bool>(true);
        if (!enabled) continue;

        std::string name   = entry["name"].as<std::string>();
        std::string type   = entry["type"].as<std::string>();
        std::string config = entry["config"].as<std::string>();

        Block* b = ModelFactory::create(type);
        b->setConfigPath(config);

        // Expose name/outputDir/logFmt if the model supports it
        if (auto* smd = dynamic_cast<SpringMassDamper*>(b)) {
            smd->name      = name;
            smd->outputDir = outDir;
            smd->logFmt    = logFmt;
        }

        allModels.push_back(b);
        stage0.push_back(b);
    }

    if (stage0.empty()) {
        std::cerr << "No models enabled — nothing to simulate.\n";
        return 1;
    }

    std::vector<std::vector<Block*>> vStage = { stage0 };
    double dts[] = { dt };

    Sim* sim = new Sim(dts, tmax, vStage, method);
    sim->run();

    delete sim;
    for (auto* b : allModels) delete b;
    return 0;
}
