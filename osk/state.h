#pragma once

class State {
public:
    static double t;
    static bool   tickfirst;
    static bool   ticklast;
    static bool   substep;

    // Pass as first arg to sample() to schedule a one-time event:
    //   if (State::sample(State::EVENT, t_event)) { ... }
    static const double EVENT;

    // Periodic:  sample(dt)          — true every dt seconds
    // One-time:  sample(EVENT, t_ev) — true once when t == t_ev
    static bool sample(double dt, double t_event = 0.0);

    double* x;
    double* xd;

    double x0;
    double k1, k2, k3, k4;

    State(double* x, double* xd);
};
