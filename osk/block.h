#pragma once
#include "state.h"
#include <vector>

class Block {
public:
    virtual ~Block();
    virtual void update() {}
    virtual void rpt()    {}

    void addIntegrator(double* x, double* xd);

    std::vector<State*> states;
};
