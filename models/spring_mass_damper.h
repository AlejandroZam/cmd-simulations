#pragma once
#include "sim.h"
#include "logger.h"
#include "noise.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>
#include <string>

// Second-order spring-mass-damper:  m*x'' + c*x' + k*x = F_noise
// States: pos (position), vel (velocity)
// Noise is injected as an additive force on the velocity derivative.
class SpringMassDamper : public Block {
public:
    std::string name;
    std::string outputDir;
    LogFormat   logFmt = LogFormat::CSV;

    SpringMassDamper()
        : pos(0.0), vel(0.0), pos_dot(0.0), vel_dot(0.0),
          mass_(1.0), damping_(0.0), stiffness_(1.0),
          pos0_(0.0), vel0_(0.0), reportDt_(0.5)
    {
        addIntegrator(&pos, &pos_dot);
        addIntegrator(&vel, &vel_dot);
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
        noise_.loadConfig(cfg["noise"]);
    }

    // Called by the MC loop before each run to reseed noise deterministically.
    void seed(uint64_t s) { noise_.seed(s); }

    void initialize() override {
        pos = pos0_;
        vel = vel0_;
        if (initCount == 0) {
            std::printf("[%s]  m=%.2f  c=%.2f  k=%.2f\n",
                        name.c_str(), mass_, damping_, stiffness_);
            std::printf("%-8s  %-12s  %-12s\n", "time(s)", "position(m)", "velocity(m/s)");
            std::printf("%-38s\n", "--------------------------------------");
        }
        if (!outputDir.empty()) {
            std::string base = outputDir + "/" + (name.empty() ? "smd" : name);
            logger_.open(base, {"t", "pos", "vel"}, logFmt);
        }
    }

    void update() override {
        pos_dot = vel;
        vel_dot = -(damping_ / mass_) * vel
                  -(stiffness_ / mass_) * pos
                  + noise_.sample() / mass_;
    }

    void report() override {
        if (State::sample(reportDt_) || State::tickfirst || State::ticklast) {
            std::printf("[%s] %8.4f  pos: %10.6f  vel: %10.6f\n",
                        name.c_str(), State::t, pos, vel);
            if (logger_.isOpen())
                logger_.write({State::t, pos, vel});
        }
    }

    ACCESS_FN(double, pos)
    ACCESS_FN(double, vel)

    double pos, vel, pos_dot, vel_dot;

private:
    double   mass_, damping_, stiffness_;
    double   pos0_, vel0_, reportDt_;
    Logger   logger_;
    NoiseGen noise_;
};
