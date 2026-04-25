# CMD Simulations

## Build

```bash
mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc) && cd ..
```

## Run a scenario

```bash
./build/src/sim ex_1
```

Reads `INPUT_DATA/ex_1/scenario.yaml`. Output lands in `OUTPUT_DATA/ex_1/run0/`, `run1/`, etc.

No rebuild needed after editing `scenario.yaml`.

## Configure via scenario.yaml

```yaml
simulation:
  tmax: 30.0
  dt: 0.01
  integration_method: RK4   # EULER, RK2, or RK4

monte_carlo:
  runs: 3
  base_seed: 7

output:
  format: csv   # csv or bin
```

## Visualise ex_1

```bash
python tools/visualise_ex1.py
```
