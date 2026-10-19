include(FetchContent)
if(NOT DEFINED METAMOD_SOURCE_PATH)
    set(METAMOD_SOURCE_PATH "$ENV{METAMOD_SOURCE_PATH}" CACHE PATH "Local Metamod SDK")
endif()
if(METAMOD_SOURCE_PATH)
    get_filename_component(METAMOD_SOURCE_PATH "${METAMOD_SOURCE_PATH}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
else()
    include("${CMAKE_CURRENT_LIST_DIR}/Revisions.cmake")
    FetchContent_Declare(mmfg_metamod
        GIT_REPOSITORY https://github.com/metamod-fallguys/metamod.git
        GIT_TAG "${MMFG_METAMOD_REVISION}"
        GIT_SUBMODULES "" GIT_SUBMODULES_RECURSE FALSE SOURCE_SUBDIR mmfg-source-only)
    FetchContent_MakeAvailable(mmfg_metamod)
    set(METAMOD_SOURCE_PATH "${mmfg_metamod_SOURCE_DIR}")
endif()
include("${METAMOD_SOURCE_PATH}/cmake/Platform.cmake")
