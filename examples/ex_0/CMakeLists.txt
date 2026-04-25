add_executable(ex_0 main.cpp)

target_link_libraries(ex_0 PRIVATE osk yaml-cpp)

target_include_directories(ex_0 PRIVATE
    ${CMAKE_SOURCE_DIR}/osk
    ${CMAKE_SOURCE_DIR}/models
)

# Copy params.yaml next to the binary so it's found at runtime
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/params.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/params.yaml
    COPYONLY
)
