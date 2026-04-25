# CMD Simulations — Session Context

## What this project is
An implementation of **C++ Model Developer (CMD)** — a U.S. Army AMRDEC open-source framework for building simulations of systems described by time-based differential equations (DEs). Reference doc: `~/Downloads/ADA433836.pdf` (Technical Report AMR-SG-05-12, April 2005).

## Directory layout
```
CMD/                              ← repo root (cmd-simulations, tagged v1.0.0)
├── CMakeLists.txt                ← root build; version 1.0.0, OSK install rules,
│                                    define_dependency calls for all models
├── cmake/
│   ├── define_dependency.cmake   ← fetch-or-error model resolver
│   └── osk-config.cmake.in       ← installed package config template
├── osk/                          ← Object-Oriented Simulation Kernel (do not modify)
│   ├── state.h/cpp               ← State class
│   ├── block.h/cpp               ← Block base class + ACCESS_FN macro
│   ├── sim.h/cpp                 ← Sim executive + IntegMethod enum
│   ├── trackable.h               ← Abstract interface: pos3()/vel3() for seeker models
│   ├── factory.h                 ← ModelFactory — register and create Block subtypes
│   ├── logger.h                  ← Signal-registration CSV/binary logger
│   ├── noise.h                   ← NoiseGen — Gaussian/Uniform/Laplace/Weibull
│   ├── montecarlo.h              ← MonteCarlo run control
│   ├── yaml_eigen.h              ← yamlToVector<Eigen::VectorXd> helper
│   ├── simdds.h/cpp              ← SimDDS singleton (FastDDS domain participant)
│   ├── dds_types.h               ← StateMsg POD + StateMsgType (memcpy TypeSupport)
│   ├── dds_pub.h                 ← SimPublisher: init() in initialize(), publish() in report()
│   └── dds_sub.h                 ← SimSubscriber: init() in initialize(), take() in update()
├── src/
│   ├── CMakeLists.txt
│   └── main.cpp                  ← Generic scenario runner (one binary for all scenarios)
├── external/                     ← gitignored; model repos checked out here
│   ├── cmd-model-smd/            ← github.com/AlejandroZam/cmd-model-smd    v0.1.0
│   ├── cmd-model-target/         ← github.com/AlejandroZam/cmd-model-target v0.2.0
│   └── cmd-model-missile/        ← github.com/AlejandroZam/cmd-model-missile v0.2.0
├── INPUT_DATA/                   ← committed; all scenario config files
│   ├── ex_0/
│   │   ├── scenario.yaml         ← sim settings + model list (the ONE input to sim)
│   │   ├── smd_heavy_params.yaml
│   │   └── smd_light_params.yaml
│   └── ex_1/
│       ├── scenario.yaml
│       ├── missile_params.yaml
│       └── target_params.yaml
├── OUTPUT_DATA/                  ← gitignored; written at runtime by sim
│   ├── ex_0/run0/ … run2/
│   └── ex_1/run0/ … run2/
└── tools/
    └── visualise_ex1.py          ← matplotlib visualiser for ex_1 output
```

## OSK architecture

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
- `void setConfigPath(const std::string& path)` — sets the YAML path the kernel passes to `loadConfig()`
- `int initCount` — incremented by the kernel after each `initialize()` call
- `std::string name`, `std::string outputDir`, `LogFormat logFmt` — set by main.cpp before `Sim::run()`
- `virtual void loadConfig(const std::string& path)` — override to load model YAML
- `virtual void initialize()` — called once per stage; use `initCount` to distinguish stages
- `virtual void update()` — define state derivatives; called 4× per step (RK4)
- `virtual void report()` — called once per full time step after integration
- `virtual void seed(uint64_t s)` — override to reseed all NoiseGen members per MC run
- `ACCESS_FN(type, var)` macro — generates a read-only accessor `var_()`:
  ```cpp
  ACCESS_FN(double, pos)   // generates: double pos_() const { return pos; }
  ```

### Trackable (`osk/trackable.h`)
Abstract interface for anything a seeker/guidance model can track.
```cpp
class Trackable {
public:
    virtual const Eigen::Vector3d& pos3() const = 0;
    virtual const Eigen::Vector3d& vel3() const = 0;
};
```
- `Target` implements `Trackable`
- `Missile` holds a `Trackable*` — no direct coupling to `Target`
- When FastDDS pub/sub lands, this pointer is replaced by a topic subscription

### Sim (`osk/sim.h`)
Simulation executive — runs the train-of-objects time loop.
- `static int stop` — set by models to control flow:
  - `Sim::stop = -1` → terminate at end of current time step
  - `Sim::stop > 0`  → advance to next stage
- Constructor: `Sim(double* dts, double tmax, vector<vector<Block*>>& vStage, IntegMethod method)`

## Integration algorithms
Selectable via `integration_method` in `scenario.yaml`:
- **`EULER`** — forward Euler (1st order)
- **`RK2`** — Heun's method (2nd order)
- **`RK4`** — 4th-order Runge-Kutta (default)

## Model dependency system (`cmake/define_dependency.cmake`)

Models must be checked out in `external/` before running cmake — **no automatic fetching**.
If a model is missing, cmake aborts with the exact clone command:

```cmake
# In CMakeLists.txt:
define_dependency(
    NAME    cmd-model-missile
    REPO    https://github.com/AlejandroZam/cmd-model-missile.git
    VERSION 0.1.0
)
```

**Error output when missing:**
```
CMake Error: [define_dependency] Missing model: cmd-model-missile v0.1.0

  Not found in: /path/to/CMD/external/cmd-model-missile

  Clone it with:
    git clone https://github.com/AlejandroZam/cmd-model-missile.git --branch v0.1.0 external/cmd-model-missile

  Then re-run cmake.
```

**Success output:**
```
-- [define_dependency] cmd-model-missile v0.1.0: found at .../external/cmd-model-missile
```

## Scenario YAML format (`INPUT_DATA/<name>/scenario.yaml`)

This is the single file passed to `sim`. Model configs are resolved relative to the scenario directory.

```yaml
simulation:
  tmax: 30.0
  dt: 0.01
  integration_method: RK4

monte_carlo:
  runs: 3
  base_seed: 7

output:
  format: csv          # csv or bin

models:
  - name: target
    type: Target        # registered with ModelFactory::reg<T>("TypeName")
    config: target_params.yaml
    enabled: true

  - name: missile
    type: Missile
    config: missile_params.yaml
    enabled: true
```

## Generic scenario runner (`src/main.cpp`)

One binary handles all scenarios. Run from the repo root:
```bash
./build/src/sim ex_1
# Reads:  INPUT_DATA/ex_1/scenario.yaml
# Writes: OUTPUT_DATA/ex_1/run0/, run1/, run2/
```

`main.cpp` responsibilities:
1. Register all known model types with `ModelFactory`
2. Instantiate models from scenario YAML
3. Resolve model configs relative to `INPUT_DATA/<scenario>/`
4. Wire inter-model dependencies (temporary — FastDDS will replace this)
5. Run MC loop; output to `OUTPUT_DATA/<scenario>/`

**Note:** Adding a new model type requires registering it in `src/main.cpp`:
```cpp
ModelFactory::reg<MyModel>("MyModel");
```

## Adding a new model

1. Create a GitHub repo `cmd-model-<name>` with this layout:
   ```
   cmd-model-<name>/
   ├── CMakeLists.txt       ← see pattern below
   ├── my_model.h           ← class declaration only
   ├── my_model.cpp         ← all method definitions
   └── default_params.yaml
   ```

2. Model `CMakeLists.txt` pattern:
   ```cmake
   cmake_minimum_required(VERSION 3.15)
   project(cmd-model-<name> VERSION 0.1.0 LANGUAGES CXX)
   set(CMAKE_CXX_STANDARD 17)

   # Use osk from parent CMD project, or find installed package for standalone builds
   if(NOT TARGET osk)
       find_package(osk 1.0.0 REQUIRED)
   endif()

   add_library(<name> STATIC my_model.cpp)
   target_include_directories(<name> PUBLIC
       $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
       $<INSTALL_INTERFACE:include>
   )
   target_link_libraries(<name> PUBLIC osk)
   ```

3. Clone into `external/`:
   ```bash
   git clone https://github.com/AlejandroZam/cmd-model-<name>.git --branch v0.1.0 external/cmd-model-<name>
   ```

4. Add `define_dependency` to `CMD/CMakeLists.txt`

5. Add library to `src/CMakeLists.txt`: `target_link_libraries(sim PRIVATE ... <name>)`

6. Register type in `src/main.cpp`: `ModelFactory::reg<MyModel>("MyModel");`

7. Add `#include "my_model.h"` to `src/main.cpp`

## Adding a new scenario

1. Create `INPUT_DATA/<name>/scenario.yaml` + model param files
2. Run `./build/src/sim <name>` from repo root — no CMake changes needed

## Model template (header + cpp)

**my_model.h:**
```cpp
#pragma once
#include "block.h"
#include "noise.h"
#include <string>
#include <vector>

class MyModel : public Block {
public:
    MyModel();
    void loadConfig(const std::string& path) override;
    void seed(uint64_t s) override;
    void initialize() override;
    void update() override;
    void report() override;

    ACCESS_FN(double, x)
    double x = 0.0, xd = 0.0;

private:
    double   x0_ = 0.0, reportDt_ = 0.5;
    Logger   logger_;
    NoiseGen noise_;
    std::vector<std::string> outputSignals_;
};
```

**my_model.cpp:**
```cpp
#include "my_model.h"
#include "sim.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

MyModel::MyModel() {
    addIntegrator(&x, &xd);
    logger_.addSignal("t", &State::t);
    logger_.addSignal("x", &x);
}

void MyModel::loadConfig(const std::string& path) {
    YAML::Node cfg = YAML::LoadFile(path);
    x0_       = cfg["model"]["initial_x"].as<double>(0.0);
    reportDt_ = 1.0 / cfg["model"]["report_rate_hz"].as<double>(2.0);
    noise_.loadConfig(cfg["noise"]["force"]);
    if (cfg["output"] && cfg["output"]["signals"])
        for (auto s : cfg["output"]["signals"])
            outputSignals_.push_back(s.as<std::string>());
}

void MyModel::seed(uint64_t s) { noise_.seed(s); }

void MyModel::initialize() {
    x = x0_;
    logger_.close();
    if (!outputDir.empty())
        logger_.open(outputDir + "/" + name, logFmt, outputSignals_);
}

void MyModel::update() { xd = /* f(x, t) */ + noise_.sample(); }

void MyModel::report() {
    if (State::sample(reportDt_) || State::tickfirst || State::ticklast) {
        std::printf("[%s] t=%8.4f  x=%f\n", name.c_str(), State::t, x);
        if (logger_.isOpen()) logger_.write();
    }
}
```

## OSK install / versioning

- `cmd-simulations` is tagged `v1.0.0`. OSK installs via:
  ```bash
  cmake --install build/
  ```
  Generates `osk-config.cmake` + `osk-config-version.cmake` for `find_package(osk 1.0.0)`.

- Model repos are tagged independently (e.g. `v0.1.0`). Each declares:
  ```cmake
  project(cmd-model-<name> VERSION 0.1.0 LANGUAGES CXX)
  ```

## Build

```bash
# First time: clone required models into external/
git clone https://github.com/AlejandroZam/cmd-model-smd.git     --branch v0.1.0 external/cmd-model-smd
git clone https://github.com/AlejandroZam/cmd-model-target.git  --branch v0.2.0 external/cmd-model-target
git clone https://github.com/AlejandroZam/cmd-model-missile.git --branch v0.2.0 external/cmd-model-missile

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run scenarios from repo root:
cd ..
./build/src/sim ex_0
./build/src/sim ex_1

# Visualise ex_1:
python tools/visualise_ex1.py
```

Dependencies: `sudo apt-get install -y libyaml-cpp-dev libeigen3-dev`

## Output logging (`osk/logger.h`)

Signal-registration logger — models register signals once in the constructor, `open()` filters to the subset declared in `output.signals` in the model YAML.

```cpp
// Constructor:
logger_.addSignal("t",   &State::t);
logger_.addSignal("pos", &pos);

// initialize():
logger_.open(outputDir + "/" + name, logFmt, outputSignals_);

// report():
if (logger_.isOpen()) logger_.write();
```

## Noise (`osk/noise.h`)

One `NoiseGen` instance per channel. Configure from YAML subkey:
```yaml
noise:
  force:
    distribution: gaussian   # gaussian / uniform / laplace / weibull
    mean: 0.0
    stddev: 0.05
```
Call `noise_.seed(runSeed)` in `Block::seed()` for deterministic MC runs.

## Monte Carlo (`osk/montecarlo.h`)

```yaml
monte_carlo:
  runs: 3
  base_seed: 42
```
Seed for run N = `baseSeed + N`. Output goes to `OUTPUT_DATA/<scenario>/run<N>/`.

## FastDDS pub/sub (inter-model communication)

Models communicate via DDS topics instead of direct pointer wiring.

**Topic naming:** `"sim.<model_name>.state"` (e.g. `"sim.target.state"`)

**Publishing (Target):**
```cpp
// header: SimPublisher publisher_;
// initialize(): publisher_.init("sim." + name + ".state");
// report():
StateMsg msg{State::t, pos_.x(), pos_.y(), pos_.z(), vel_.x(), vel_.y(), vel_.z()};
publisher_.publish(msg);
```

**Subscribing (Missile):**
```cpp
// header: SimSubscriber subscriber_; Eigen::Vector3d tpos_, tvel_; bool hasTargetData_;
// model YAML: target_topic: target
// initialize(): subscriber_.init("sim." + targetTopic_ + ".state");
// update():
StateMsg msg;
if (subscriber_.take(msg)) { tpos_ = {msg.px,msg.py,msg.pz}; tvel_ = {msg.vx,msg.vy,msg.vz}; hasTargetData_ = true; }
if (!hasTargetData_) return;
// ... guidance using tpos_, tvel_
```

**Key implementation note:** `StateMsgType::max_serialized_type_size` MUST be set in the constructor (= sizeof(StateMsg)). If left at 0 (default), FastDDS cannot pre-allocate the serialization buffer and `write()` returns RETCODE_ERROR silently. Also implement `construct_sample()` for intraprocess optimization.

**ZOH behaviour:** Target publishes every `report()` call. Missile reads in `update()`. Within an RK4 step, `take()` fires on k1 (new data), then returns false on k2/k3/k4 (missile keeps last tpos_/tvel_). One-timestep delay is inherent and acceptable.

**main.cpp:** Call `SimDDS::get().init()` before the MC loop, `SimDDS::get().shutdown()` after.

## Status (as of session end)
- OSK kernel: complete, versioned at v1.0.0, install rules in place
- FastDDS 3.2.2 integrated: SimDDS singleton, SimPublisher, SimSubscriber, StateMsg type
- Model repos on GitHub:
  - `AlejandroZam/cmd-model-smd` v0.1.0
  - `AlejandroZam/cmd-model-target` v0.2.0 — publishes state via DDS
  - `AlejandroZam/cmd-model-missile` v0.2.0 — subscribes to target via DDS, no Trackable pointer
- `cmake/define_dependency.cmake`: errors with exact clone command if model missing
- `src/main.cpp`: generic scenario runner; no model-to-model pointer wiring needed
- `INPUT_DATA/`: ex_0 (dual SMD, MC) and ex_1 (missile/target intercept, MC)
- `tools/visualise_ex1.py`: 6-panel matplotlib plot from OUTPUT_DATA/ex_1
- Next: update define_dependency clone commands to v0.2.0, Gazebo visualisation, model testing
