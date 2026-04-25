# define_dependency(NAME <name> REPO <url> VERSION <semver>)
#
# Resolves a model dependency from CMD/external/<NAME>/.
# The model MUST be checked out locally — no automatic network fetching.
# If the model is missing, CMake aborts with the exact clone command to run.
#
# The resolved target is available as <NAME> after this call.
function(define_dependency)
    cmake_parse_arguments(DEP "" "NAME;REPO;VERSION" "" ${ARGN})

    if(NOT DEP_NAME OR NOT DEP_REPO OR NOT DEP_VERSION)
        message(FATAL_ERROR "define_dependency: NAME, REPO, and VERSION are all required.")
    endif()

    set(_local "${CMAKE_SOURCE_DIR}/external/${DEP_NAME}")

    if(EXISTS "${_local}/CMakeLists.txt")
        message(STATUS "[define_dependency] ${DEP_NAME} v${DEP_VERSION}: found at ${_local}")
        add_subdirectory("${_local}" "${CMAKE_BINARY_DIR}/external/${DEP_NAME}")
    else()
        message(FATAL_ERROR
            "\n"
            "  [define_dependency] Missing model: ${DEP_NAME} v${DEP_VERSION}\n"
            "\n"
            "  Not found in: ${_local}\n"
            "\n"
            "  Clone it with:\n"
            "    git clone ${DEP_REPO} --branch v${DEP_VERSION} external/${DEP_NAME}\n"
            "\n"
            "  Then re-run cmake.\n"
        )
    endif()
endfunction()
