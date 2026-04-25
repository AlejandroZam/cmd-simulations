#include "missile.h"
#include "sim.h"
#include "yaml_eigen.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

Missile::Missile() {
    addIntegrator(&pos_.x(), &vel_.x());
    addIntegrator(&pos_.y(), &vel_.y());
    addIntegrator(&pos_.z(), &vel_.z());
    addIntegrator(&vel_.x(), &acc_.x());
    addIntegrator(&vel_.y(), &acc_.y());
    addIntegrator(&vel_.z(), &acc_.z());

    logger_.addSignal("t",     &State::t);
    logger_.addSignal("px",    &pos_.x());
    logger_.addSignal("py",    &pos_.y());
    logger_.addSignal("pz",    &pos_.z());
    logger_.addSignal("vx",    &vel_.x());
    logger_.addSignal("vy",    &vel_.y());
    logger_.addSignal("vz",    &vel_.z());
    logger_.addSignal("range", &range_);
}

void Missile::loadConfig(const std::string& path) {
    YAML::Node cfg = YAML::LoadFile(path);
    auto m     = cfg["model"];
    pos0_      = yamlToVector<Eigen::Vector3d>(m["initial_position"]);
    vel0_      = yamlToVector<Eigen::Vector3d>(m["initial_velocity"]);
    navRatio_  = m["nav_ratio"].as<double>(navRatio_);
    aMax_      = m["max_accel"].as<double>(aMax_);
    missDist_  = m["miss_distance"].as<double>(missDist_);
    double rHz = m["report_rate_hz"].as<double>(1.0 / reportDt_);
    reportDt_  = 1.0 / rHz;

    noiseAx_.loadConfig(cfg["noise"]["ax"]);
    noiseAy_.loadConfig(cfg["noise"]["ay"]);
    noiseAz_.loadConfig(cfg["noise"]["az"]);

    outputSignals_.clear();
    if (cfg["output"] && cfg["output"]["signals"])
        for (auto s : cfg["output"]["signals"])
            outputSignals_.push_back(s.as<std::string>());
}

void Missile::seed(uint64_t s) {
    noiseAx_.seed(s);
    noiseAy_.seed(s);
    noiseAz_.seed(s);
}

void Missile::initialize() {
    pos_      = pos0_;
    vel_      = vel0_;
    acc_      = Eigen::Vector3d::Zero();
    range_    = target_ ? (target_->pos3() - pos_).norm() : 0.0;
    intercept_ = false;

    if (initCount == 0) {
        std::printf("[%s] Missile  IC pos=(%.0f, %.0f, %.0f)  vel=(%.1f, %.1f, %.1f)  "
                    "N=%.1f  aMax=%.0f m/s²\n",
                    name.c_str(),
                    pos_.x(), pos_.y(), pos_.z(),
                    vel_.x(), vel_.y(), vel_.z(),
                    navRatio_, aMax_);
        std::printf("%-7s  %-8s  %-28s  %-28s\n",
                    "t(s)", "range(m)", "missile pos (x,y,z)", "target  pos (x,y,z)");
        std::printf("%-74s\n", std::string(74, '-').c_str());
    }
    logger_.close();
    if (!outputDir.empty())
        logger_.open(outputDir + "/" + name, logFmt, outputSignals_);
}

void Missile::update() {
    if (!target_) { acc_ = Eigen::Vector3d::Zero(); return; }

    Eigen::Vector3d los = target_->pos3() - pos_;
    range_ = los.norm();

    if (range_ < missDist_) {
        intercept_ = true;
        Sim::stop  = -1;
        acc_       = Eigen::Vector3d::Zero();
        return;
    }

    Eigen::Vector3d losHat  = los / range_;
    Eigen::Vector3d relVel  = target_->vel3() - vel_;
    double          Vc      = -relVel.dot(losHat);
    Eigen::Vector3d losRate = (relVel - relVel.dot(losHat) * losHat) / range_;

    acc_ = navRatio_ * Vc * losRate;

    double aMag = acc_.norm();
    if (aMag > aMax_) acc_ *= aMax_ / aMag;

    acc_ += Eigen::Vector3d(noiseAx_.sample(), noiseAy_.sample(), noiseAz_.sample());
}

void Missile::report() {
    if (State::sample(reportDt_) || State::tickfirst || State::ticklast) {
        Eigen::Vector3d tpos = target_ ? target_->pos3() : Eigen::Vector3d::Zero();
        std::printf("[%s] t=%7.3f  range=%8.1f  "
                    "m=(%8.1f,%7.1f,%7.1f)  t=(%8.1f,%7.1f,%5.1f)\n",
                    name.c_str(), State::t, range_,
                    pos_.x(), pos_.y(), pos_.z(),
                    tpos.x(), tpos.y(), tpos.z());
        if (logger_.isOpen()) logger_.write();
    }
    if (intercept_ && (State::ticklast || !State::substep)) {
        Eigen::Vector3d tpos = target_ ? target_->pos3() : Eigen::Vector3d::Zero();
        std::printf("\n*** [%s] INTERCEPT at t=%.3f s  range=%.2f m  "
                    "pos=(%.1f, %.1f, %.1f) ***\n\n",
                    name.c_str(), State::t, range_,
                    tpos.x(), tpos.y(), tpos.z());
    }
}
