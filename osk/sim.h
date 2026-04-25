#pragma once
#include "block.h"
#include <vector>

class Sim {
public:
    static int stop;

    // dts:    array of time-steps, one per stage
    // tmax:   simulation end time
    // vStage: train-of-objects — outer vector is stages, inner is models
    Sim(double* dts, double tmax, std::vector<std::vector<Block*>>& vStage);
    void run();

private:
    double*                           dts_;
    double                            tmax_;
    std::vector<std::vector<Block*>>& vStage_;

    std::vector<State*> collectStates(std::vector<Block*>& stage);
    void stepRK4(std::vector<Block*>& stage, std::vector<State*>& states, double dt);
    void callUpdate(std::vector<Block*>& stage);
    void callRpt(std::vector<Block*>& stage);
};
