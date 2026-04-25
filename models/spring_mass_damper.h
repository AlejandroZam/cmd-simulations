#pragma once
#include "sim.h"
#include "logger.h"
#include "noise.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>
#include <string>
#include <vector>

// Second-order spring-mass-damper:  m*x'' + c*x' + k*x = F_noise
// Two noise channels:
//   noise.force       — additive force disturbance injected into vel_dot
//   noise.measurement — additive position measurement noise (output only)
//
// Loggable signals (select via output.signals in model YAML):
//   t, pos, vel, pos_noisy
class SpringMassDamper : public Block {
public:
    std::string name;
    std::string outputDir;
    LogFormat   logFmt = LogFormat::CSV;

    SpringMassDamper()
        : pos(0.0), vel(0.0), pos_dot(0.0), vel_dot(0.0), posNoisy_(0.0),
          mass_(1.0), damping_(0.0), stiffness_(1.0),
          pos0_(0.0), vel0_(0.0), reportDt_(0.5)
    {
        addIntegrator(&pos, &pos_dot);
        addIntegrator(&vel, &vel_dot);

        // Register every loggable signal. open() will filter to output.signals.
        logger_.addSignal("t",         &State::t);
        logger_.addSignal("pos",       &pos);
        logger_.addSignal("vel",       &vel);
        logger_.addSignal("pos_noisy", &posNoisy_);
    }

    void loadConfig(const std::string& path) override {
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

    void seed(uint64_t s) override {
        noiseForce_.seed(s);
        noiseMeas_.seed(s);
    }

    void initialize() override {
        pos      = pos0_;
        vel      = vel0_;
        posNoisy_ = pos0_;

        if (initCount == 0) {
            std::printf("[%s]  m=%.2f  c=%.2f  k=%.2f\n",
                        name.c_str(), mass_, damping_, stiffness_);
            std::printf("%-8s  %-12s  %-12s  %-12s\n",
                        "time(s)", "pos(m)", "vel(m/s)", "pos_noisy");
            std::printf("%-54s\n",
                        "------------------------------------------------------");
        }
        logger_.close();
        if (!outputDir.empty()) {
            std::string base = outputDir + "/" + (name.empty() ? "smd" : name);
            logger_.open(base, logFmt, outputSignals_);
        }
    }

    void update() override {
        pos_dot = vel;
        vel_dot = -(damping_ / mass_) * vel
                  -(stiffness_ / mass_) * pos
                  + noiseForce_.sample() / mass_;
    }

    void report() override {
        posNoisy_ = pos + noiseMeas_.sample();
        if (State::sample(reportDt_) || State::tickfirst || State::ticklast) {
            std::printf("[%s] %8.4f  pos: %10.6f  vel: %10.6f  noisy: %10.6f\n",
                        name.c_str(), State::t, pos, vel, posNoisy_);
            if (logger_.isOpen()) logger_.write();
        }
    }

    ACCESS_FN(double, pos)
    ACCESS_FN(double, vel)
    ACCESS_FN(double, posNoisy_)

    double pos, vel, pos_dot, vel_dot;

private:
    double   posNoisy_;
    double   mass_, damping_, stiffness_;
    double   pos0_, vel0_, reportDt_;
    Logger   logger_;
    NoiseGen noiseForce_;
    NoiseGen noiseMeas_;
    std::vector<std::string> outputSignals_;
};
