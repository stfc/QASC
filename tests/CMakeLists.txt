include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest
    GIT_TAG v2.5.3 # tag, branch or even commit SHA
)
FetchContent_MakeAvailable(doctest)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")

add_executable(test_qasc
    test_codegen.cpp
    test_compiler.cpp
    test_qasc_core.cpp
    test_init.cpp
)
target_compile_features(test_qasc PRIVATE cxx_std_20)

target_link_libraries(test_qasc
    PUBLIC
    qasc-lib
    antlr4_shared
    QuEST::QuEST
    doctest::doctest
)

if (QUEST_BUILT_WITH_MPI)
    find_package(MPI REQUIRED)
    target_link_libraries(test_qasc
        PRIVATE
        MPI::MPI_CXX
    )
endif()


doctest_discover_tests(test_qasc)
