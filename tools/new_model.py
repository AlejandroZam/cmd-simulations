#!/usr/bin/env python3
"""Scaffold a new CMD model.

Usage (from repo root):
    python tools/new_model.py MyModel

Creates:
    models/MyModel/
        CMakeLists.txt
        README.md
        include/my_model.h
        src/my_model.cpp
        Config/default_params.yaml
        doc/model_description.md

Also patches (automatically — no manual edits needed):
    src/registered_models.cpp  — adds #include and ModelFactory::reg<>
    src/CMakeLists.txt         — adds library to target_link_libraries
    CMakeLists.txt             — adds add_subdirectory for the new model
"""
import re
import sys
from pathlib import Path


# ── Helpers ────────────────────────────────────────────────────────────────────

def to_snake(name: str) -> str:
    """MyModel -> my_model"""
    return re.sub(r'(?<!^)(?=[A-Z])', '_', name).lower()

def to_lib(name: str) -> str:
    """MyModel -> mymodel  (CMake target name)"""
    return name.lower()

def insert_before_sentinel(path: Path, sentinel: str, new_line: str):
    text = path.read_text()
    if new_line in text:
        return  # already present
    text = text.replace(sentinel, new_line + '\n' + sentinel)
    path.write_text(text)

def append_to_sentinel(path: Path, sentinel: str, new_line: str):
    """Insert new_line on the line before the closing sentinel."""
    insert_before_sentinel(path, sentinel, new_line)


# ── Templates ──────────────────────────────────────────────────────────────────

def cmake_template(class_name: str, snake: str, lib: str) -> str:
    return f"""\
cmake_minimum_required(VERSION 3.15)
project(cmd-model-{snake.replace('_', '-')} VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library({lib} STATIC src/{snake}.cpp)
target_include_directories({lib} PUBLIC
    $<BUILD_INTERFACE:${{CMAKE_CURRENT_SOURCE_DIR}}/include>
)
target_link_libraries({lib} PUBLIC osk)
"""

def header_template(class_name: str, snake: str) -> str:
    return f"""\
#pragma once
#include "block.h"
#include "noise.h"
#include <string>
#include <vector>

class {class_name} : public Block {{
public:
    {class_name}();

    void loadConfig(const std::string& path) override;
    void seed(uint64_t s) override;
    void initialize() override;
    void eventUpdate() override;
    void derivatives() override;
    void report() override;

    double x = 0.0, xd = 0.0;

private:
    double   x0_       = 0.0;
    double   reportDt_ = 0.5;
    Logger   logger_;
    NoiseGen noise_;
    std::vector<std::string> outputSignals_;
}};
"""

def source_template(class_name: str, snake: str) -> str:
    return f"""\
#include "{snake}.h"
#include "sim.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

{class_name}::{class_name}() {{
    addIntegrator(&x, &xd);
    logger_.addSignal("t", &State::t);
    logger_.addSignal("x", &x);
}}

void {class_name}::loadConfig(const std::string& path) {{
    YAML::Node cfg = YAML::LoadFile(path);
    x0_       = cfg["model"]["initial_x"].as<double>(0.0);
    reportDt_ = 1.0 / cfg["model"]["report_rate_hz"].as<double>(2.0);
    noise_.loadConfig(cfg["noise"]["force"]);
    if (cfg["output"] && cfg["output"]["signals"])
        for (auto s : cfg["output"]["signals"])
            outputSignals_.push_back(s.as<std::string>());
}}

void {class_name}::seed(uint64_t s) {{ noise_.seed(s); }}

void {class_name}::initialize() {{
    x = x0_;
    logger_.close();
    if (!outputDir.empty())
        logger_.open(outputDir + "/" + name, logFmt, outputSignals_);
}}

void {class_name}::eventUpdate() {{
    // sample noise / read DDS / check events
}}

void {class_name}::derivatives() {{
    xd = 0.0; // TODO: implement dynamics
}}

void {class_name}::report() {{
    if (State::sample(reportDt_) || State::tickfirst || State::ticklast) {{
        std::printf("[%s] t=%8.4f  x=%f\\n", name.c_str(), State::t, x);
        if (logger_.isOpen()) logger_.write();
    }}
}}
"""

def params_template() -> str:
    return """\
model:
  initial_x:     0.0
  report_rate_hz: 2.0

noise:
  force:
    distribution: gaussian
    mean: 0.0
    stddev: 0.0

output:
  signals: [t, x]
"""

def doc_template(class_name: str) -> str:
    return f"""\
# {class_name} — Model Description

## Dynamics

TODO: describe the equations of motion.

## Noise Channels

| Key           | Applied to | Description |
|---------------|------------|-------------|
| `noise.force` | `xd`       | TODO        |

## Outputs

| Signal | Units | Description     |
|--------|-------|-----------------|
| `t`    | s     | Simulation time |
| `x`    | —     | State variable  |

## Configuration Reference

```yaml
model:
  initial_x:      0.0
  report_rate_hz: 2.0

noise:
  force:
    distribution: gaussian
    mean: 0.0
    stddev: 0.0
```
"""

def readme_template(class_name: str, snake: str) -> str:
    return f"""\
# cmd-model-{snake.replace('_', '-')}

TODO: one-line description of what this model simulates.

## Usage

```yaml
- name: my_{snake}
  type: {class_name}
  config: {snake}_params.yaml
  enabled: true
  order: 0
```

## Layout

```
include/   {snake}.h
src/       {snake}.cpp
Config/    default_params.yaml
doc/       model_description.md
```

## Dependencies

Requires the `osk` package from [cmd-simulations](https://github.com/AlejandroZam/cmd-simulations).
"""


# ── Main ───────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) != 2:
        print("Usage: python tools/new_model.py <ModelName>")
        print("  ModelName must be CamelCase, e.g. MyModel")
        sys.exit(1)

    class_name = sys.argv[1]
    if not class_name[0].isupper():
        print(f"Error: ModelName must start with an uppercase letter (got '{class_name}')")
        sys.exit(1)

    snake = to_snake(class_name)
    lib   = to_lib(class_name)

    repo_root = Path(__file__).resolve().parent.parent
    model_dir = repo_root / "models" / class_name

    if model_dir.exists():
        print(f"Error: {model_dir} already exists.")
        sys.exit(1)

    # ── Create directory tree ──────────────────────────────────────────────────
    for subdir in ("include", "src", "Config", "doc"):
        (model_dir / subdir).mkdir(parents=True)

    # ── Write files ────────────────────────────────────────────────────────────
    (model_dir / "CMakeLists.txt"           ).write_text(cmake_template(class_name, snake, lib))
    (model_dir / "README.md"                ).write_text(readme_template(class_name, snake))
    (model_dir / f"include/{snake}.h"       ).write_text(header_template(class_name, snake))
    (model_dir / f"src/{snake}.cpp"         ).write_text(source_template(class_name, snake))
    (model_dir / "Config/default_params.yaml").write_text(params_template())
    (model_dir / "doc/model_description.md" ).write_text(doc_template(class_name))

    print(f"[new_model] Created {model_dir.relative_to(repo_root)}/")

    # ── Patch registered_models.cpp ───────────────────────────────────────────
    reg_file = repo_root / "src" / "registered_models.cpp"
    append_to_sentinel(reg_file, "// === MODEL_INCLUDES_END ===",
                       f'#include "{snake}.h"')
    append_to_sentinel(reg_file, "// === MODEL_REGS_END ===",
                       f'    ModelFactory::reg<{class_name}>("{class_name}");')
    print(f"[new_model] Patched src/registered_models.cpp")

    # ── Patch src/CMakeLists.txt ───────────────────────────────────────────────
    src_cmake = repo_root / "src" / "CMakeLists.txt"
    append_to_sentinel(src_cmake, "    # === MODEL_LIBS_END ===",
                       f"    {lib}")
    print(f"[new_model] Patched src/CMakeLists.txt")

    # ── Patch root CMakeLists.txt ──────────────────────────────────────────────
    root_cmake = repo_root / "CMakeLists.txt"
    rel = f"models/{class_name}"
    append_to_sentinel(root_cmake, "# === LOCAL_MODEL_SUBDIRS_END ===",
                       f"add_subdirectory({rel})")
    print(f"[new_model] Patched CMakeLists.txt")

    print(f"""
Done. Next steps:
  1. Edit models/{class_name}/include/{snake}.h   — declare your class
  2. Edit models/{class_name}/src/{snake}.cpp      — implement dynamics
  3. Edit models/{class_name}/Config/default_params.yaml
  4. cmake --build build
  5. Add the model to your scenario YAML with type: {class_name}
""")


if __name__ == "__main__":
    main()
