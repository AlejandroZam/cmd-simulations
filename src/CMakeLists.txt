add_executable(sim main.cpp registered_models.cpp)

target_link_libraries(sim PRIVATE
    # === MODEL_LIBS_BEGIN ===
    smd model_target missile
    # === MODEL_LIBS_END ===
)
