#pragma once
#include <Eigen/Dense>

// Abstract interface for anything a seeker/guidance model can track.
// Target implements this; Missile holds a Trackable* so it is decoupled
// from any concrete model type.
// When FastDDS pub/sub lands this pointer is replaced by a topic subscription.
class Trackable {
public:
    virtual ~Trackable() = default;
    virtual const Eigen::Vector3d& pos3() const = 0;
    virtual const Eigen::Vector3d& vel3() const = 0;
};
