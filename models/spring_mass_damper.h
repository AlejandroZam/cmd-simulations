#pragma once
#include "block.h"
#include "noise.h"
#include <string>
#include <vector>

// Second-order spring-mass-damper:  m*x'' + c*x' + k*x = F_noise
// Two noise channels:
//   noise.force       — additive force disturbance injected into vel_dot
//   noise.measurement — additive position measurement noise (output only)
class SpringMassDamper : public Block {
public:
    SpringMassDamper();

    void loadConfig(const std::string& path) override;
    void seed(uint64_t s) override;
    void initialize() override;
    void update() override;
    void report() override;

    ACCESS_FN(double, pos)
    ACCESS_FN(double, vel)
    ACCESS_FN(double, posNoisy_)

    double pos     = 0.0;
    double vel     = 0.0;
    double pos_dot = 0.0;
    double vel_dot = 0.0;

private:
    double   posNoisy_  = 0.0;
    double   mass_      = 1.0;
    double   damping_   = 0.0;
    double   stiffness_ = 1.0;
    double   pos0_      = 0.0;
    double   vel0_      = 0.0;
    double   reportDt_  = 0.5;
    Logger   logger_;
    NoiseGen noiseForce_;
    NoiseGen noiseMeas_;
    std::vector<std::string> outputSignals_;
};
