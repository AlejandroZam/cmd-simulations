#pragma once
#include "state.h"
#include "logger.h"
#include <string>
#include <vector>

// Creates a read-only accessor for model-to-model communication.
// ACCESS_FN(double, pos) generates:  double pos_() const { return pos; }
#define ACCESS_FN(type, var) type var##_() const { return var; }

class Block {
public:
    virtual ~Block();

    // Called by the kernel before initialize() at the start of each stage.
    // Override to load model parameters from a YAML file.
    virtual void loadConfig(const std::string& /*path*/) {}

    // Called once per stage before the time loop. initCount tells the model
    // which stage it is in (0 = first stage, 1 = second, ...).
    virtual void initialize() {}

    // Override to reseed all NoiseGen members deterministically per MC run.
    virtual void seed(uint64_t /*s*/) {}

    // Called 4x per time step (RK4) to compute state derivatives.
    // Use State::sample() to gate discrete operations.
    virtual void update()     {}

    // Called once per time step after integration. Use State::sample(),
    // State::tickfirst, and State::ticklast to control output.
    virtual void report()     {}

    void addIntegrator(double* x, double* xd);

    // Set before Sim::run() to have the kernel automatically call
    // loadConfig(configPath) at the start of each stage.
    void setConfigPath(const std::string& path) { configPath = path; }

    std::vector<State*> states;

    // Incremented by the kernel each time initialize() is called.
    // Use in initialize() to distinguish stage 0, 1, 2, ...
    int         initCount  = 0;
    std::string configPath;

    // Set by main.cpp before Sim::run(); available to all model overrides.
    std::string name;
    std::string outputDir;
    LogFormat   logFmt = LogFormat::CSV;
};
