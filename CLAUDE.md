# CMD Simulations — Session Context

## What this project is
An implementation of **C++ Model Developer (CMD)** — a U.S. Army AMRDEC open-source framework for building simulations of systems described by time-based differential equations (DEs). Reference doc: `~/Downloads/ADA433836.pdf` (Technical Report AMR-SG-05-12, April 2005).

## Directory layout
```
simulations/
└── cmd/
    ├── CMakeLists.txt           ← root build; add add_subdirectory() here for new examples
    ├── osk/                     ← Object-Oriented Simulation Kernel (do not modify)
    │   ├── state.h/cpp          ← State class
    │   ├── block.h/cpp          ← Block base class + ACCESS_FN macro
    │   └── sim.h/cpp            ← Sim executive + IntegMethod enum
    ├── models/                  ← reusable model headers (one model = one .h file)
    │   └── spring_mass_damper.h
    └── examples/
        └── ex_0/                ← spring-mass-damper demo
            ├── main.cpp
            ├── params.yaml                    ← sim-level config (tmax, dt, method)
            ├── spring_mass_damper_params.yaml ← model-level config (physics, rates)
            └── CMakeLists.txt
```

## OSK architecture (3 classes)

### State (`osk/state.h`)
Wraps a single integrator state variable and its derivative. Managed internally by Block.
- `static double t` — current simulation time
- `static bool tickfirst` — true only on the very first call per stage
- `static bool ticklast` — true on the final time step
- `static bool substep` — true during RK4 k2/k3/k4 sub-evaluations
- `static const double EVENT` — sentinel for one-time event scheduling
- `static bool sample(double dt, double t_event = 0.0)` — two modes:
  - **Periodic:** `State::sample(0.1)` — true every 0.1 s, never during substeps
  - **One-time event:** `State::sample(State::EVENT, 1.5)` — true once when `t == 1.5`

### Block (`osk/block.h`)
Abstract base class for all models. Derive from this.
- `void addIntegrator(double* x, double* xd)` — registers a state/derivative pair. Call in constructor.
- `void setConfigPath(const std::string& path)` — sets the YAML path the kernel passes to `loadConfig()` at each stage start
- `int initCount` — incremented by the kernel after each `initialize()` call; use in `initialize()` to distinguish stage 0, 1, 2, …
- `std::string configPath` — set via `setConfigPath()`
- `virtual void loadConfig(const std::string& path)` — override to load model YAML; called automatically by kernel before each `initialize()`
- `virtual void initialize()` — called once per stage before the time loop; use `initCount` to distinguish stages
- `virtual void update()` — define state derivatives here, called 4× per time step (RK4); gate discrete logic with `State::sample()`
- `virtual void report()` — called once per full time step after integration; gate output with `State::sample()`, `tickfirst`, `ticklast`
- `ACCESS_FN(type, var)` macro — defined in block.h; generates a read-only accessor `var_()` for model-to-model communication:
  ```cpp
  ACCESS_FN(double, pos)   // generates: double pos_() const { return pos; }
  ```

### Sim (`osk/sim.h`)
Simulation executive — runs the train-of-objects time loop.
- `static int stop` — set by models to control flow:
  - `Sim::stop = -1` → terminate simulation at end of current time step
  - `Sim::stop > 0`  → advance to next stage at end of current time step
- Constructor: `Sim(double* dts, double tmax, vector<vector<Block*>>& vStage, IntegMethod method = IntegMethod::RK4)`
  - `dts`: array of time-steps, one per stage
  - `vStage`: outer vector = stages, inner vector = models in that stage
  - `method`: `IntegMethod::EULER`, `IntegMethod::RK2`, or `IntegMethod::RK4`
- `void run()` — executes all stages

## Integration algorithms
Selectable via `IntegMethod` enum or `integration_method` in `params.yaml`:
- **`EULER`** — forward Euler (1st order)
- **`RK2`** — Heun's method (2nd order)
- **`RK4`** — 4th-order Runge-Kutta (default)

Per RK4 time step: save `x0` → k1 (`substep=false`) → k2 → k3 → k4 (`substep=true`) → combine → call `report()`.

## Kernel call sequence per stage
```
loadConfig()      ← reads model YAML (called if configPath is set)
initialize()      ← set ICs; initCount tells you which stage (0, 1, 2, ...)
  tickfirst=true
  update() + report()   ← initial tick
  tickfirst=false
  ── time loop ──
    step() [Euler/RK2/RK4]
    report()
  ── end loop ──
initCount++
```

## Train-of-objects (multi-stage) pattern
```
STATE → inside BLOCK → inside STAGE (vector<Block*>) → inside SIMULATION (vector<vector<Block*>>)
```
- Multiple models per stage: push them all into the stage vector; they execute in order
- Stage transition: set `Sim::stop = 1` inside `update()`; use `State::sample(State::EVENT, t)` for a one-time trigger
- Models reused across stages pick up from previous state; use `initCount` in `initialize()` to re-init selectively

## Model-to-model interaction pattern (from CMD spec §4.4)
```cpp
// In the consuming model's header — store a typed pointer:
class Autopilot : public Block {
public:
    void getsFrom(Missile* obj) { missile_ = obj; }
    ...
private:
    Missile* missile_ = nullptr;
};

// In the supplying model's header — expose read-only accessors:
class Missile : public Block {
public:
    ACCESS_FN(double, gamma)   // other models call missile->gamma_()
    ...
};

// In main.cpp — wire them up before Sim::run():
autopilot->getsFrom(missile);
missile->getsFrom(autopilot);
```

## Per-model update rates (discrete/hybrid models)
The sim integration step runs at the global `dt`. Each model controls its own discrete rate inside `update()` using `State::sample()`:
```cpp
void update() override {
    // Discrete subsystem at 10 Hz:
    if (State::sample(0.1)) {
        a_cmd = k * (gamma_cmd - missile_->gamma_());
    }
    // Continuous physics (every substep):
    gammad = a_cmd / v;
}
```
Configure the discrete rate in the model's own YAML and read it in `loadConfig()`.

## Writing a new model — full template
```cpp
#pragma once
#include "sim.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

class MyModel : public Block {
public:
    MyModel() : x(0.0), xd(0.0) {
        addIntegrator(&x, &xd);
    }

    void loadConfig(const std::string& path) override {
        YAML::Node cfg = YAML::LoadFile(path);
        x0_       = cfg["model"]["initial_x"].as<double>(0.0);
        reportDt_ = 1.0 / cfg["model"]["report_rate_hz"].as<double>(1.0);
    }

    void initialize() override {
        x = x0_;
        if (initCount == 0) std::printf("MyModel starting\n");
    }

    void update() override {
        xd = /* f(x, t) */;
    }

    void report() override {
        if (State::sample(reportDt_) || State::tickfirst || State::ticklast)
            std::printf("%8.4f  x: %f\n", State::t, x);
    }

    ACCESS_FN(double, x)   // exposes x_() for other models

    double x, xd;
private:
    double x0_ = 0.0, reportDt_ = 0.5;
};
```

## YAML split — sim vs model
`params.yaml` (sim-level, read in `main.cpp`):
```yaml
simulation:
  tmax: 10.0
  dt: 0.01
  integration_method: RK4   # EULER, RK2, or RK4
```

`<model_name>_params.yaml` (model-level, read in `loadConfig()`):
```yaml
model:
  initial_position: 1.0
  update_rate_hz: 100.0   # discrete subsystem rate
  report_rate_hz: 2.0     # output rate
  # ...physics params...
```

## Multiple named model instances (from main sim YAML)

`params.yaml` `models:` list drives instantiation. Each entry has:
```yaml
models:
  - name: smd_heavy           # unique instance name (used in output filenames and printf)
    type: SpringMassDamper    # registered with ModelFactory::reg<T>("TypeName")
    config: heavy_params.yaml # passed to loadConfig()
    enabled: true             # set false to skip this instance entirely
```

`main.cpp` registers types, iterates the list, skips disabled entries, creates instances via `ModelFactory::create(type)`, and casts to set `name` / `outputDir` / `logFmt`.

`ModelFactory` lives in `osk/factory.h`. Register once per type before the loop:
```cpp
ModelFactory::reg<SpringMassDamper>("SpringMassDamper");
```

## Output logging (`osk/logger.h`)

`Logger` class writes CSV or binary per model instance. Models hold a `Logger logger_` member.

In `initialize()` (stage 0 only):
```cpp
if (!outputDir.empty())
    logger_.open(outputDir + "/" + name, {"t","pos","vel"}, logFmt);
```

In `report()`:
```cpp
if (logger_.isOpen()) logger_.write({State::t, pos, vel});
```

**Binary format**: header = `uint32_t n_cols`, then for each col `uint32_t len` + `char[]` name; rows = packed `double[n_cols]`.

`main.cpp` reads `output.dir` and `output.format` (csv/bin) from `params.yaml` and calls `fs::create_directories(outDir)` before instantiating models.

## Adding a new example
1. `mkdir examples/ex_N`
2. Create `main.cpp`, `params.yaml`, model yaml files, `CMakeLists.txt` (copy ex_0 as template)
3. In `main.cpp`: register model types, iterate `cfg["models"]`, wire with `getsFrom` if multi-model
4. Add `add_subdirectory(examples/ex_N)` to root `CMakeLists.txt`
5. Add `configure_file(...)` entries in the example's `CMakeLists.txt` for each yaml

## Adding a new model
- One model = one header in `models/`
- Include `"sim.h"`, `"logger.h"`, `<yaml-cpp/yaml.h>`; no other OSK dependencies
- Override `loadConfig`, `initialize`, `update`, `report`
- Add public fields: `std::string name`, `std::string outputDir`, `LogFormat logFmt`
- Use `ACCESS_FN` to expose outputs; define a typed `getsFrom()` for inputs
- CMake include path already covers `models/` and `osk/` for all examples

## Build system
CMake + yaml-cpp. Build:
```bash
cd cmd/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./examples/ex_0/ex_0   # runs from build/examples/ex_0/
```
Dependency: `sudo apt-get install -y libyaml-cpp-dev`

## Status (as of session end)
- OSK kernel: complete, compiles cleanly
- `osk/factory.h`: ModelFactory — registers and creates Block subtypes by string name
- `osk/logger.h`: Logger — CSV and binary file output per model instance
- ex_0: two named SpringMassDamper instances (smd_heavy, smd_light), both writing CSV to `output/`
- Features implemented: `initialize()`, `loadConfig()`, `ACCESS_FN`, `initCount`, `State::EVENT`, `IntegMethod` (EULER/RK2/RK4), per-model YAML, model-to-model interaction, named instances from YAML, enable/disable per model, CSV/binary logging
- Next steps: add more examples/models demonstrating multi-model stages and model-to-model wiring
