#pragma once
#include "block.h"
#include "trackable.h"
#include "noise.h"
#include <Eigen/Dense>
#include <string>
#include <vector>

// Constant-velocity ground vehicle (z = 0 enforced).
// Noise channels noise.lateral_x / noise.lateral_y simulate terrain effects.
// Implements Trackable so Missile can hold a Trackable* without knowing this type.
class Target : public Block, public Trackable {
public:
    Target();

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

    double   reportDt_ = 1.0;
    Logger   logger_;
    NoiseGen noiseX_, noiseY_;
    std::vector<std::string> outputSignals_;
};
