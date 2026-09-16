cmake_minimum_required(VERSION 3.21)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(QASC VERSION 0.4.0 LANGUAGES C CXX)

option(
  BUILD_TESTING
  "Build tests using doctest testing framework. Turned ON by default."
  ON
)

message(STATUS "BUILD_TESTING turned ${BUILD_TESTING}.")

if (BUILD_TESTING)
option(
  QUEST_BUILT_WITH_MPI
  "Flag if QuEST was built in a distributed mode with MPI. Turned OFF by default."
  OFF
)
message(STATUS "QUEST_BUILT_WITH_MPI turned ${QUEST_BUILT_WITH_MPI}.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FetchContent)

find_package(QuEST)
if (QuEST_FOUND)
    get_target_property(QUEST_INCLUDE_DIRS QuEST::QuEST INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(QUEST_LIB_LOCATION QuEST::QuEST IMPORTED_LOCATION_RELEASE)

    # ensure we get something (either release or debug build)
    if(NOT QUEST_LIB_LOCATION)
        get_target_property(QUEST_LIB_LOCATION QuEST::QuEST IMPORTED_LOCATION_DEBUG)
    endif()

    if(NOT QUEST_LIB_LOCATION)
        get_target_property(QUEST_LIB_LOCATION QuEST::QuEST IMPORTED_LOCATION)
    endif()

    get_filename_component(QUEST_LIB_DIR "${QUEST_LIB_LOCATION}" DIRECTORY)
else()
    # TODO: once there is a new QuEST release remember to update it.
    FetchContent_Declare(
        QuEST
        GIT_REPOSITORY https://github.com/QuEST-Kit/QuEST
        GIT_TAG v4.2.0
    )
    FetchContent_MakeAvailable(QuEST)

    set(QUEST_LIB_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/_deps/quest-build"
    )

    set(QUEST_INCLUDE_DIRS
        "${CMAKE_CURRENT_BINARY_DIR}/_deps/quest-src/quest/include -I ${CMAKE_CURRENT_BINARY_DIR}/_deps/quest-src -I ${CMAKE_CURRENT_BINARY_DIR}/_deps/quest-build "
    )

    set(QuEST_VERSION_MINOR 2)
endif()

set(ANTLR4_WITH_STATIC_CRT OFF)
set(ANTLR_BUILD_SHARED ON)
set(ANTLR_TAG 4.13.2)
set(DISABLE_WARNINGS ON)
set(ANTLR_BUILD_CPP_TESTS OFF)
set(ANTLR_EXECUTABLE "${PROJECT_SOURCE_DIR}/antlr4-generated/antlr-${ANTLR_TAG}-complete.jar")

if(NOT EXISTS "${ANTLR_EXECUTABLE}")
    file(
        DOWNLOAD
        "https://www.antlr.org/download/antlr-${ANTLR_TAG}-complete.jar"
        "${ANTLR_EXECUTABLE}"
    )
endif()

FetchContent_Declare(
    antlr
    GIT_REPOSITORY https://github.com/antlr/antlr4
    GIT_TAG ${ANTLR_TAG}
    SOURCE_SUBDIR "runtime/Cpp"
)
FetchContent_MakeAvailable(antlr) # populates antlr_SOURCE_DIR
include(${antlr_SOURCE_DIR}/runtime/Cpp/cmake/FindANTLR.cmake) # populates ANTLR_TARGET

set(DEPS_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")

message(STATUS "QUEST_INCLUDE_DIRS ${QUEST_INCLUDE_DIRS}")
message(STATUS "QUEST_LIB_LOCATION ${QUEST_LIB_LOCATION}")
message(STATUS "QUEST_LIB_DIR ${QUEST_LIB_DIR}")
message(STATUS "QUEST_VERSION_MINOR ${QuEST_VERSION_MINOR}")
message(STATUS "DEPS_INCLUDE_DIRS ${DEPS_INCLUDE_DIRS}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config_paths.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/config_paths.h"
)

add_library(qasc-lib SHARED)
add_executable(qasc)

target_compile_features(qasc-lib PRIVATE cxx_std_20)
target_compile_options(qasc-lib PRIVATE -O2 -Wall)

target_compile_features(qasc PRIVATE cxx_std_20)
target_compile_options(qasc PRIVATE -O2 -Wall)

target_include_directories(qasc-lib PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/antlr4-generated>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(qasc-lib PUBLIC antlr4_shared)
target_link_libraries(qasc PRIVATE qasc-lib)

add_subdirectory(antlr4-generated)
add_subdirectory(src)

if (BUILD_TESTING)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
