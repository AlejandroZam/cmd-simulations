#include <iostream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "sim.h"
#include "spring_mass_damper.h"

int main(int argc, char* argv[]) {
    const char* simCfg   = (argc > 1) ? argv[1] : "params.yaml";
    const char* modelCfg = (argc > 2) ? argv[2] : "spring_mass_damper_params.yaml";

    YAML::Node cfg;
    try {
        cfg = YAML::LoadFile(simCfg);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load " << simCfg << ": " << e.what() << "\n";
        return 1;
    }

    double      tmax      = cfg["simulation"]["tmax"].as<double>();
    double      dt        = cfg["simulation"]["dt"].as<double>();
    std::string methodStr = cfg["simulation"]["integration_method"].as<std::string>("RK4");

    IntegMethod method = IntegMethod::RK4;
    if      (methodStr == "EULER") method = IntegMethod::EULER;
    else if (methodStr == "RK2")   method = IntegMethod::RK2;

    auto* model = new SpringMassDamper();
    model->setConfigPath(modelCfg);

    // To add more models: push them into the stage vector.
    // Use model->getsFrom(otherModel) + ACCESS_FN to wire model-to-model signals.
    std::vector<Block*>              stage0 = { model };
    std::vector<std::vector<Block*>> vStage = { stage0 };
    double dts[] = { dt };

    Sim* sim = new Sim(dts, tmax, vStage, method);
    sim->run();

    delete sim;
    delete model;
    return 0;
}
