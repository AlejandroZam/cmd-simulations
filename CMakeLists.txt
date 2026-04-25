cmake_minimum_required(VERSION 3.15)
project(cmd_simulations LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(yaml-cpp REQUIRED)
find_package(Eigen3 REQUIRED NO_MODULE)

# ── OSK kernel (static library shared by all simulations) ────────────────────
add_library(osk STATIC
    osk/state.cpp
    osk/block.cpp
    osk/sim.cpp
)
target_include_directories(osk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/osk)
target_link_libraries(osk PUBLIC Eigen3::Eigen yaml-cpp)

# ── Models (static library shared by all examples) ───────────────────────────
add_library(models STATIC
    models/spring_mass_damper.cpp
    models/missile.cpp
    models/target.cpp
)
target_include_directories(models PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/models)
target_link_libraries(models PUBLIC osk)

# ── Examples ──────────────────────────────────────────────────────────────────
add_subdirectory(examples/ex_0)
add_subdirectory(examples/ex_1)

# To add a new example:
#   1. Create examples/ex_N/ with main.cpp and CMakeLists.txt
#   2. add_subdirectory(examples/ex_N) here
#
# To add a new model:
#   1. Drop a header into models/  (derive from Block, include "sim.h")
#   2. Add the models/ include path in that example's CMakeLists.txt
