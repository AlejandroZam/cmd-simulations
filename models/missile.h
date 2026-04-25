#pragma once
#include "block.h"
#include "trackable.h"
#include "noise.h"
#include <Eigen/Dense>
#include <string>
#include <vector>

// Point-mass missile with proportional navigation (PN) guidance.
// States: 3-D position and velocity (6 integrators).
// Noise channels noise.ax / noise.ay / noise.az simulate IMU/actuator error.
// Depends only on the Trackable interface — no direct coupling to Target.
class Missile : public Block {
public:
    Missile();

    void getsFrom(Trackable* t) { target_ = t; }

    void loadConfig(const std::string& path) override;
    void seed(uint64_t s) override;
    void initialize() override;
    void update() override;
    void report() override;

    const Eigen::Vector3d& pos3() const { return pos_; }
    const Eigen::Vector3d& vel3() const { return vel_; }

private:
    Eigen::Vector3d pos_  = Eigen::Vector3d::Zero();
    Eigen::Vector3d vel_  = Eigen::Vector3d::Zero();
    Eigen::Vector3d acc_  = Eigen::Vector3d::Zero();
    Eigen::Vector3d pos0_ = Eigen::Vector3d::Zero();
    Eigen::Vector3d vel0_ = Eigen::Vector3d::Zero();

    double   range_     = 0.0;
    double   navRatio_  = 4.0;
    double   aMax_      = 200.0;
    double   missDist_  = 20.0;
    double   reportDt_  = 1.0;
    bool       intercept_ = false;
    Trackable* target_    = nullptr;

    Logger   logger_;
    NoiseGen noiseAx_, noiseAy_, noiseAz_;
    std::vector<std::string> outputSignals_;
};
