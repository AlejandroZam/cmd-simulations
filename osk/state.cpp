#include "state.h"
#include <cmath>

double State::t         = 0.0;
bool   State::tickfirst = false;
bool   State::ticklast  = false;
bool   State::substep   = false;

const double State::EVENT = -1.0;

State::State(double* x, double* xd)
    : x(x), xd(xd), x0(0.0), k1(0.0), k2(0.0), k3(0.0), k4(0.0) {}

bool State::sample(double dt, double t_event) {
    if (substep) return false;
    if (dt == EVENT) {
        double ref = (t_event > 1e-12) ? t_event : 1.0;
        double tol = ref * 1e-8;
        return std::fabs(t - t_event) < 2.0 * tol;
    }
    double tol = dt * 1e-8;
    double rem = std::fmod(t + tol, dt);
    return rem < 2.0 * tol;
}
