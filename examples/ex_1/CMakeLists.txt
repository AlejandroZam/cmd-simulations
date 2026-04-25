add_executable(ex_1 main.cpp)

target_link_libraries(ex_1 PRIVATE models)

foreach(yaml_file
    params.yaml
    missile_params.yaml
    target_params.yaml
)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${yaml_file}
        ${CMAKE_CURRENT_BINARY_DIR}/${yaml_file}
        COPYONLY
    )
endforeach()
