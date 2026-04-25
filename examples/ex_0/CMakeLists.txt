add_executable(ex_0 main.cpp)

target_link_libraries(ex_0 PRIVATE osk yaml-cpp)

target_include_directories(ex_0 PRIVATE
    ${CMAKE_SOURCE_DIR}/osk
    ${CMAKE_SOURCE_DIR}/models
)

# Copy yaml configs next to the binary so they're found at runtime
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/params.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/params.yaml
    COPYONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/spring_mass_damper_params.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/spring_mass_damper_params.yaml
    COPYONLY
)
