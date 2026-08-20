# We dont want test dependencies
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(antlr)
