project(test_package LANGUAGES CXX)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(polymorphic_value REQUIRED)
find_package(Catch2 REQUIRED)

add_executable(test_indirect_value ../../test_indirect_value.cpp)
target_link_libraries(test_indirect_value PRIVATE indirect_value::indirect_value Catch2::Catch2)

target_compile_options(test_indirect_value
    PRIVATE
        #$<$<BOOL:${MINGW}>:-Wa,-mbig-obj>
)

enable_testing()
include(Catch)
catch_discover_tests(test_indirect_value)