#pragma once
#include "sim.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>
#include <string>

// Second-order spring-mass-damper:  m*x'' + c*x' + k*x = 0
// States: pos (position), vel (velocity)
//
// Parameters are loaded from a YAML file via loadConfig().
// Other models can read pos and vel through the ACCESS_FN accessors:
//   double p = smd->pos_();
//   double v = smd->vel_();
class SpringMassDamper : public Block {
public:
    SpringMassDamper()
        : pos(0.0), vel(0.0), pos_dot(0.0), vel_dot(0.0),
          mass_(1.0), damping_(0.0), stiffness_(1.0),
          pos0_(0.0), vel0_(0.0), updateDt_(0.01), reportDt_(0.5)
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
        double uHz  = m["update_rate_hz"].as<double>(1.0 / updateDt_);
        double rHz  = m["report_rate_hz"].as<double>(1.0 / reportDt_);
        updateDt_   = 1.0 / uHz;
        reportDt_   = 1.0 / rHz;
    }

    void initialize() override {
        pos = pos0_;
        vel = vel0_;
        if (initCount == 0) {
            std::printf("Spring-Mass-Damper  m=%.2f  c=%.2f  k=%.2f\n",
                        mass_, damping_, stiffness_);
            std::printf("%-8s  %-12s  %-12s\n", "time(s)", "position(m)", "velocity(m/s)");
            std::printf("%-38s\n", "--------------------------------------");
        }
    }

    void update() override {
        // Continuous derivatives — computed every RK4 sub-step.
        // For models with a discrete subsystem, gate that part with:
        //   if (State::sample(updateDt_)) { ... discrete logic ... }
        pos_dot = vel;
        vel_dot = -(damping_ / mass_) * vel - (stiffness_ / mass_) * pos;
    }

    void report() override {
        if (State::sample(reportDt_) || State::tickfirst || State::ticklast)
            std::printf("%8.4f  pos: %10.6f  vel: %10.6f\n", State::t, pos, vel);
    }

    // Read-only accessors for model-to-model communication
    ACCESS_FN(double, pos)
    ACCESS_FN(double, vel)

    double pos, vel, pos_dot, vel_dot;

private:
    double mass_, damping_, stiffness_;
    double pos0_, vel0_;
    double updateDt_, reportDt_;
};
