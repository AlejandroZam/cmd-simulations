#include "target.h"
#include "sim.h"
#include "yaml_eigen.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

Target::Target() {
    addIntegrator(&pos_.x(), &vel_.x());
    addIntegrator(&pos_.y(), &vel_.y());
    addIntegrator(&pos_.z(), &vel_.z());
    addIntegrator(&vel_.x(), &acc_.x());
    addIntegrator(&vel_.y(), &acc_.y());
    addIntegrator(&vel_.z(), &acc_.z());

    logger_.addSignal("t",  &State::t);
    logger_.addSignal("px", &pos_.x());
    logger_.addSignal("py", &pos_.y());
    logger_.addSignal("pz", &pos_.z());
    logger_.addSignal("vx", &vel_.x());
    logger_.addSignal("vy", &vel_.y());
    logger_.addSignal("vz", &vel_.z());
}

void Target::loadConfig(const std::string& path) {
    YAML::Node cfg = YAML::LoadFile(path);
    auto m     = cfg["model"];
    pos0_      = yamlToVector<Eigen::Vector3d>(m["initial_position"]);
    vel0_      = yamlToVector<Eigen::Vector3d>(m["initial_velocity"]);
    double rHz = m["report_rate_hz"].as<double>(1.0 / reportDt_);
    reportDt_  = 1.0 / rHz;

    noiseX_.loadConfig(cfg["noise"]["lateral_x"]);
    noiseY_.loadConfig(cfg["noise"]["lateral_y"]);

    outputSignals_.clear();
    if (cfg["output"] && cfg["output"]["signals"])
        for (auto s : cfg["output"]["signals"])
            outputSignals_.push_back(s.as<std::string>());
}

void Target::seed(uint64_t s) {
    noiseX_.seed(s);
    noiseY_.seed(s);
}

void Target::initialize() {
    pos_ = pos0_;
    vel_ = vel0_;
    acc_ = Eigen::Vector3d::Zero();
    pos_.z() = 0.0;
    vel_.z() = 0.0;

    if (initCount == 0) {
        std::printf("[%s] Target  IC pos=(%.0f, %.0f, %.0f)  vel=(%.1f, %.1f, %.1f)\n",
                    name.c_str(),
                    pos_.x(), pos_.y(), pos_.z(),
                    vel_.x(), vel_.y(), vel_.z());
    }
    logger_.close();
    if (!outputDir.empty())
        logger_.open(outputDir + "/" + name, logFmt, outputSignals_);
}

void Target::update() {
    acc_.x() = noiseX_.sample();
    acc_.y() = noiseY_.sample();
    acc_.z() = 0.0;
    pos_.z() = 0.0;
    vel_.z() = 0.0;
}

void Target::report() {
    if (State::sample(reportDt_) || State::tickfirst || State::ticklast) {
        std::printf("[%s] t=%7.3f  pos=(%8.1f, %7.1f, %5.1f)  vel=(%6.2f, %6.2f, %5.2f)\n",
                    name.c_str(), State::t,
                    pos_.x(), pos_.y(), pos_.z(),
                    vel_.x(), vel_.y(), vel_.z());
        if (logger_.isOpen()) logger_.write();
    }
}
