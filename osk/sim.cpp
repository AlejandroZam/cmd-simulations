#include "sim.h"

int Sim::stop = 0;

Sim::Sim(double* dts, double tmax, std::vector<std::vector<Block*>>& vStage, IntegMethod method)
    : dts_(dts), tmax_(tmax), vStage_(vStage), method_(method) {}

std::vector<State*> Sim::collectStates(std::vector<Block*>& stage) {
    std::vector<State*> all;
    for (auto* b : stage)
        for (auto* s : b->states)
            all.push_back(s);
    return all;
}

void Sim::callInitialize(std::vector<Block*>& stage) {
    for (auto* b : stage) {
        if (!b->configPath.empty())
            b->loadConfig(b->configPath);
        b->initialize();
        b->initCount++;
    }
}

void Sim::callUpdate(std::vector<Block*>& stage) {
    for (auto* b : stage) b->update();
}

void Sim::callReport(std::vector<Block*>& stage) {
    for (auto* b : stage) b->report();
}

void Sim::stepEuler(std::vector<Block*>& stage, std::vector<State*>& states, double dt) {
    double t0 = State::t;

    State::substep = false;
    callUpdate(stage);
    for (auto* s : states)
        *s->x = *s->x + *s->xd * dt;

    State::t       = t0 + dt;
    State::substep = false;
}

void Sim::stepRK2(std::vector<Block*>& stage, std::vector<State*>& states, double dt) {
    double t0 = State::t;

    for (auto* s : states) s->x0 = *s->x;

    // k1
    State::substep = false;
    callUpdate(stage);
    for (auto* s : states) { s->k1 = *s->xd; *s->x = s->x0 + s->k1 * dt; }

    // k2
    State::t       = t0 + dt;
    State::substep = true;
    callUpdate(stage);
    for (auto* s : states) s->k2 = *s->xd;

    // Combine (Heun's method)
    for (auto* s : states)
        *s->x = s->x0 + (s->k1 + s->k2) * dt * 0.5;

    State::t       = t0 + dt;
    State::substep = false;
}

void Sim::stepRK4(std::vector<Block*>& stage, std::vector<State*>& states, double dt) {
    double t0 = State::t;

    for (auto* s : states) s->x0 = *s->x;

    // k1
    State::substep = false;
    callUpdate(stage);
    for (auto* s : states) { s->k1 = *s->xd; *s->x = s->x0 + s->k1 * dt * 0.5; }

    // k2
    State::t       = t0 + dt * 0.5;
    State::substep = true;
    callUpdate(stage);
    for (auto* s : states) { s->k2 = *s->xd; *s->x = s->x0 + s->k2 * dt * 0.5; }

    // k3
    callUpdate(stage);
    for (auto* s : states) { s->k3 = *s->xd; *s->x = s->x0 + s->k3 * dt; }

    // k4
    State::t = t0 + dt;
    callUpdate(stage);
    for (auto* s : states) s->k4 = *s->xd;

    // Combine
    for (auto* s : states)
        *s->x = s->x0 + (s->k1 + 2.0*s->k2 + 2.0*s->k3 + s->k4) * dt / 6.0;

    State::t       = t0 + dt;
    State::substep = false;
}

void Sim::step(std::vector<Block*>& stage, std::vector<State*>& states, double dt) {
    switch (method_) {
        case IntegMethod::EULER: stepEuler(stage, states, dt); break;
        case IntegMethod::RK2:   stepRK2  (stage, states, dt); break;
        case IntegMethod::RK4:   stepRK4  (stage, states, dt); break;
    }
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

        callInitialize(stage);

        // Initial tick for this stage
        State::tickfirst = true;
        callUpdate(stage);
        callReport(stage);
        State::tickfirst = false;

        Sim::stop = 0;

        while (State::t < tmax_ - dt * 0.5 && Sim::stop == 0) {
            step(stage, states, dt);

            bool last = (State::t >= tmax_ - dt * 0.5) || (Sim::stop < 0);
            State::ticklast = last;
            callReport(stage);
            State::ticklast = false;

            if (Sim::stop < 0) break;
        }

        if (Sim::stop < 0) break;
        Sim::stop = 0;
    }
}
