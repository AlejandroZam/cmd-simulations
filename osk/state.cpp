#include "state.h"
#include <cmath>

double State::t         = 0.0;
bool   State::tickfirst = false;
bool   State::ticklast  = false;
bool   State::substep   = false;

State::State(double* x, double* xd)
    : x(x), xd(xd), x0(0.0), k1(0.0), k2(0.0), k3(0.0), k4(0.0) {}

bool State::sample(double dt) {
    if (substep) return false;
    double tol = dt * 1e-8;
    double rem = std::fmod(t + tol, dt);
    return rem < 2.0 * tol;
}
