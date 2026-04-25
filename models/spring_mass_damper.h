#pragma once
#include "sim.h"
#include <cstdio>

// Second-order spring-mass-damper:  m*x'' + c*x' + k*x = 0
// States: pos (position), vel (velocity)
class SpringMassDamper : public Block {
public:
    SpringMassDamper(double mass, double damping, double stiffness,
                     double x0, double v0, double reportInterval)
        : mass_(mass), damping_(damping), stiffness_(stiffness),
          pos(x0), vel(v0), pos_dot(0.0), vel_dot(0.0),
          reportInterval_(reportInterval)
    {
        addIntegrator(&pos, &pos_dot);
        addIntegrator(&vel, &vel_dot);
    }

    void update() override {
        pos_dot = vel;
        vel_dot = -(damping_ / mass_) * vel - (stiffness_ / mass_) * pos;
    }

    void rpt() override {
        if (State::sample(reportInterval_) || State::tickfirst || State::ticklast)
            std::printf("%8.4f  pos: %10.6f  vel: %10.6f\n", State::t, pos, vel);
    }

    double pos, vel;
    double pos_dot, vel_dot;

private:
    double mass_, damping_, stiffness_, reportInterval_;
};
