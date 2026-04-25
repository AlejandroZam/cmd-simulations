cmake_minimum_required(VERSION 3.15)
project(cmd-osk VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── CMake modules ─────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(define_dependency)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(yaml-cpp REQUIRED)
find_package(Eigen3 REQUIRED NO_MODULE)

# ── OSK kernel (static library) ───────────────────────────────────────────────
add_library(osk STATIC
    osk/state.cpp
    osk/block.cpp
    osk/sim.cpp
)
target_include_directories(osk PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/osk>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/osk>
)
target_link_libraries(osk PUBLIC Eigen3::Eigen yaml-cpp)

# ── OSK install rules ─────────────────────────────────────────────────────────
install(TARGETS osk
    EXPORT osk-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY osk/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/osk
    FILES_MATCHING PATTERN "*.h"
)
install(EXPORT osk-targets
    FILE       osk-targets.cmake
    NAMESPACE  osk::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osk
)
configure_package_config_file(
    cmake/osk-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/osk-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osk
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/osk-config-version.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/osk-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/osk-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osk
)

# ── Models (static library — built-in reference implementations) ──────────────
add_library(models STATIC
    models/spring_mass_damper.cpp
    models/missile.cpp
    models/target.cpp
)
target_include_directories(models PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/models>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/models>
)
target_link_libraries(models PUBLIC osk)

# ── Scenario runner ───────────────────────────────────────────────────────────
add_subdirectory(src)
