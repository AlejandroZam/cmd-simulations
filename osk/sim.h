#pragma once
#include "block.h"
#include <vector>

enum class IntegMethod { EULER, RK2, RK4 };

class Sim {
public:
    static int stop;

    // dts:    array of time-steps, one per stage
    // tmax:   simulation end time
    // vStage: train-of-objects — outer vector is stages, inner is models
    // method: integration method (default RK4)
    Sim(double* dts, double tmax, std::vector<std::vector<Block*>>& vStage,
        IntegMethod method = IntegMethod::RK4);
    void run();

private:
    double*                           dts_;
    double                            tmax_;
    std::vector<std::vector<Block*>>& vStage_;
    IntegMethod                       method_;

    std::vector<State*> collectStates(std::vector<Block*>& stage);
    void step      (std::vector<Block*>& stage, std::vector<State*>& states, double dt);
    void stepEuler (std::vector<Block*>& stage, std::vector<State*>& states, double dt);
    void stepRK2   (std::vector<Block*>& stage, std::vector<State*>& states, double dt);
    void stepRK4   (std::vector<Block*>& stage, std::vector<State*>& states, double dt);
    void callInitialize  (std::vector<Block*>& stage);
    void callEventUpdate (std::vector<Block*>& stage);
    void callDerivatives (std::vector<Block*>& stage);
    void callReport      (std::vector<Block*>& stage);
};
