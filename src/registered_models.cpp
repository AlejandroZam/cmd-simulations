#include "factory.h"
// === MODEL_INCLUDES_BEGIN ===
#include "spring_mass_damper.h"
#include "target.h"
#include "missile.h"
// === MODEL_INCLUDES_END ===

void registerAllModels() {
// === MODEL_REGS_BEGIN ===
    ModelFactory::reg<SpringMassDamper>("SpringMassDamper");
    ModelFactory::reg<Target>("Target");
    ModelFactory::reg<Missile>("Missile");
// === MODEL_REGS_END ===
}
