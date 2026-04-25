#include "block.h"

Block::~Block() {
    for (auto* s : states) delete s;
}

void Block::addIntegrator(double* x, double* xd) {
    states.push_back(new State(x, xd));
}
