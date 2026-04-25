#pragma once

class State {
public:
    static double t;
    static bool   tickfirst;
    static bool   ticklast;
    static bool   substep;   // true during RK4 k2/k3/k4 sub-evaluations

    static bool sample(double dt);

    double* x;
    double* xd;

    double x0;
    double k1, k2, k3, k4;

    State(double* x, double* xd);
};
