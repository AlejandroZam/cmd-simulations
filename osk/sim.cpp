#include "sim.h"

int Sim::stop = 0;

Sim::Sim(double* dts, double tmax, std::vector<std::vector<Block*>>& vStage)
    : dts_(dts), tmax_(tmax), vStage_(vStage) {}

std::vector<State*> Sim::collectStates(std::vector<Block*>& stage) {
    std::vector<State*> all;
    for (auto* b : stage)
        for (auto* s : b->states)
            all.push_back(s);
    return all;
}

void Sim::callUpdate(std::vector<Block*>& stage) {
    for (auto* b : stage) b->update();
}

void Sim::callRpt(std::vector<Block*>& stage) {
    for (auto* b : stage) b->rpt();
}

void Sim::stepRK4(std::vector<Block*>& stage, std::vector<State*>& states, double dt) {
    double t0 = State::t;

    for (auto* s : states) s->x0 = *s->x;

    // k1 — full-step evaluation, sample() is valid here
    State::substep = false;
    callUpdate(stage);
    for (auto* s : states) { s->k1 = *s->xd; *s->x = s->x0 + s->k1 * dt * 0.5; }

    // k2
    State::t = t0 + dt * 0.5;
    State::substep = true;
    callUpdate(stage);
    for (auto* s : states) { s->k2 = *s->xd; *s->x = s->x0 + s->k2 * dt * 0.5; }

    // k3 — same t as k2
    callUpdate(stage);
    for (auto* s : states) { s->k3 = *s->xd; *s->x = s->x0 + s->k3 * dt; }

    // k4
    State::t = t0 + dt;
    callUpdate(stage);
    for (auto* s : states) s->k4 = *s->xd;

    // Combine and finalise
    for (auto* s : states)
        *s->x = s->x0 + (s->k1 + 2.0*s->k2 + 2.0*s->k3 + s->k4) * dt / 6.0;

    State::t      = t0 + dt;
    State::substep = false;
}

void Sim::run() {
    State::t         = 0.0;
    State::tickfirst = false;
    State::ticklast  = false;
    State::substep   = false;
    Sim::stop        = 0;

    int numStages = static_cast<int>(vStage_.size());

    for (int si = 0; si < numStages; ++si) {
        auto& stage  = vStage_[si];
        double dt    = dts_[si];
        auto  states = collectStates(stage);

        // Initial tick for this stage: report current state before advancing
        State::tickfirst = true;
        callUpdate(stage);
        callRpt(stage);
        State::tickfirst = false;

        Sim::stop = 0;

        while (State::t < tmax_ - dt * 0.5 && Sim::stop == 0) {
            stepRK4(stage, states, dt);

            bool last = (State::t >= tmax_ - dt * 0.5) || (Sim::stop < 0);
            State::ticklast = last;
            callRpt(stage);
            State::ticklast = false;

            if (Sim::stop < 0) break;
        }

        // Negative stop — terminate entirely
        if (Sim::stop < 0) break;

        // Positive stop — advance to next stage (reset so next stage runs)
        Sim::stop = 0;
    }
}
