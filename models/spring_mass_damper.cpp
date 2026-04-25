#include "spring_mass_damper.h"
#include "sim.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

SpringMassDamper::SpringMassDamper() {
    addIntegrator(&pos, &pos_dot);
    addIntegrator(&vel, &vel_dot);

    logger_.addSignal("t",         &State::t);
    logger_.addSignal("pos",       &pos);
    logger_.addSignal("vel",       &vel);
    logger_.addSignal("pos_noisy", &posNoisy_);
}

void SpringMassDamper::loadConfig(const std::string& path) {
    YAML::Node cfg = YAML::LoadFile(path);
    auto m      = cfg["model"];
    mass_       = m["mass"].as<double>(mass_);
    damping_    = m["damping"].as<double>(damping_);
    stiffness_  = m["stiffness"].as<double>(stiffness_);
    pos0_       = m["initial_position"].as<double>(pos0_);
    vel0_       = m["initial_velocity"].as<double>(vel0_);
    double rHz  = m["report_rate_hz"].as<double>(1.0 / reportDt_);
    reportDt_   = 1.0 / rHz;

    noiseForce_.loadConfig(cfg["noise"]["force"]);
    noiseMeas_.loadConfig(cfg["noise"]["measurement"]);

    outputSignals_.clear();
    if (cfg["output"] && cfg["output"]["signals"])
        for (auto s : cfg["output"]["signals"])
            outputSignals_.push_back(s.as<std::string>());
}

void SpringMassDamper::seed(uint64_t s) {
    noiseForce_.seed(s);
    noiseMeas_.seed(s);
}

void SpringMassDamper::initialize() {
    pos      = pos0_;
    vel      = vel0_;
    posNoisy_ = pos0_;

    if (initCount == 0) {
        std::printf("[%s]  m=%.2f  c=%.2f  k=%.2f\n",
                    name.c_str(), mass_, damping_, stiffness_);
        std::printf("%-8s  %-12s  %-12s  %-12s\n",
                    "time(s)", "pos(m)", "vel(m/s)", "pos_noisy");
        std::printf("%-54s\n", "------------------------------------------------------");
    }
    logger_.close();
    if (!outputDir.empty())
        logger_.open(outputDir + "/" + (name.empty() ? "smd" : name),
                     logFmt, outputSignals_);
}

void SpringMassDamper::update() {
    pos_dot = vel;
    vel_dot = -(damping_ / mass_) * vel
              -(stiffness_ / mass_) * pos
              + noiseForce_.sample() / mass_;
}

void SpringMassDamper::report() {
    posNoisy_ = pos + noiseMeas_.sample();
    if (State::sample(reportDt_) || State::tickfirst || State::ticklast) {
        std::printf("[%s] %8.4f  pos: %10.6f  vel: %10.6f  noisy: %10.6f\n",
                    name.c_str(), State::t, pos, vel, posNoisy_);
        if (logger_.isOpen()) logger_.write();
    }
}
