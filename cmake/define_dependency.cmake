# define_dependency(NAME <name> REPO <url> VERSION <semver>)
#
# Resolves a model dependency using a local-first strategy:
#   1. If CMD/external/<NAME>/CMakeLists.txt exists → add_subdirectory (local override)
#   2. Otherwise → FetchContent from REPO at tag v<VERSION>
#
# The resolved target is available as <NAME> after this call.
function(define_dependency)
    cmake_parse_arguments(DEP "" "NAME;REPO;VERSION" "" ${ARGN})

    if(NOT DEP_NAME OR NOT DEP_REPO OR NOT DEP_VERSION)
        message(FATAL_ERROR "define_dependency: NAME, REPO, and VERSION are all required.")
    endif()

    set(_local "${CMAKE_SOURCE_DIR}/external/${DEP_NAME}")

    if(EXISTS "${_local}/CMakeLists.txt")
        message(STATUS "[define_dependency] ${DEP_NAME}: using local override at ${_local}")
        add_subdirectory("${_local}" "${CMAKE_BINARY_DIR}/external/${DEP_NAME}")
    else()
        message(STATUS "[define_dependency] ${DEP_NAME}: fetching v${DEP_VERSION} from ${DEP_REPO}")
        include(FetchContent)
        FetchContent_Declare(${DEP_NAME}
            GIT_REPOSITORY "${DEP_REPO}"
            GIT_TAG        "v${DEP_VERSION}"
        )
        FetchContent_MakeAvailable(${DEP_NAME})
    endif()
endfunction()
