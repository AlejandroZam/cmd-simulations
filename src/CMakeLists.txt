add_executable(sim main.cpp)
target_link_libraries(sim PRIVATE smd model_target missile)
