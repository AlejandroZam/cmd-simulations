#include <iostream>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "sim.h"
#include "spring_mass_damper.h"

int main(int argc, char* argv[]) {
    const char* cfgPath = (argc > 1) ? argv[1] : "params.yaml";

    YAML::Node cfg;
    try {
        cfg = YAML::LoadFile(cfgPath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load " << cfgPath << ": " << e.what() << "\n";
        return 1;
    }

    double tmax           = cfg["simulation"]["tmax"].as<double>();
    double dt             = cfg["simulation"]["dt"].as<double>();
    double reportInterval = cfg["simulation"]["report_interval"].as<double>();
    std::string methodStr = cfg["simulation"]["integration_method"].as<std::string>("RK4");

    IntegMethod method = IntegMethod::RK4;
    if      (methodStr == "EULER") method = IntegMethod::EULER;
    else if (methodStr == "RK2")   method = IntegMethod::RK2;

    double mass      = cfg["model"]["mass"].as<double>();
    double damping   = cfg["model"]["damping"].as<double>();
    double stiffness = cfg["model"]["stiffness"].as<double>();
    double x0        = cfg["model"]["initial_position"].as<double>();
    double v0        = cfg["model"]["initial_velocity"].as<double>();

    std::printf("Spring-Mass-Damper  m=%.2f  c=%.2f  k=%.2f\n", mass, damping, stiffness);
    std::printf("%-8s  %-12s  %-12s\n", "time(s)", "position(m)", "velocity(m/s)");
    std::printf("%s\n", std::string(38, '-').c_str());

    auto* model = new SpringMassDamper(mass, damping, stiffness, x0, v0, reportInterval);

    std::vector<Block*>              stage0  = { model };
    std::vector<std::vector<Block*>> vStage  = { stage0 };
    double dts[] = { dt };

    Sim* sim = new Sim(dts, tmax, vStage, method);
    sim->run();

    delete sim;
    delete model;
    return 0;
}
