#pragma once
#include "state.h"
#include <vector>

class Block {
public:
    virtual ~Block();
    virtual void initialize() {}
    virtual void update()     {}
    virtual void report()     {}

    void addIntegrator(double* x, double* xd);

    std::vector<State*> states;
};
