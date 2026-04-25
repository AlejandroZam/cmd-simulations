add_executable(ex_0 main.cpp)

target_link_libraries(ex_0 PRIVATE osk)

target_include_directories(ex_0 PRIVATE
    ${CMAKE_SOURCE_DIR}/osk
    ${CMAKE_SOURCE_DIR}/models
)

# Copy yaml configs next to the binary so they're found at runtime
foreach(yaml_file
    params.yaml
    spring_mass_damper_heavy_params.yaml
    spring_mass_damper_light_params.yaml
)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${yaml_file}
        ${CMAKE_CURRENT_BINARY_DIR}/${yaml_file}
        COPYONLY
    )
endforeach()
