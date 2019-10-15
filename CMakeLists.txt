cmake_minimum_required(VERSION 3.14)

if(NOT DEFINED PROJECT_NAME)
    set(CPP_INDIRECT_IS_SUBPROJECT ON)
endif()

set(CPP_INDIRECT_VALUE_VERSION "0.0.0")

# FIXME(twon) use CMake features checks instead.
# Needed for `std::in_place_t`.
set(CMAKE_CXX_STANDARD 17) 

project(indirect VERSION ${CPP_INDIRECT_VALUE_VERSION})

option(ENABLE_SANITIZERS "Enable Address Sanitizer and Undefined Behaviour Sanitizer if available" OFF)

add_library(indirect INTERFACE)
target_include_directories(indirect
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_sources(indirect
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect.h>
        $<INSTALL_INTERFACE:include/indirect.h>
)

add_library(indirect::indirect ALIAS indirect)

if (${CPP_INDIRECT_IS_SUBPROJECT})

    include(FetchContent)
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    )

    FetchContent_GetProperties(catch2)
    if(NOT googletest_POPULATED)
        FetchContent_Populate(catch2)
        add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
    endif()

    add_executable(test_indirect test_indirect.cpp)
    target_link_libraries(test_indirect
        PRIVATE
            indirect::indirect
            Catch2::Catch2
    )

    target_compile_options(test_indirect
        PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
            $<$<CXX_COMPILER_ID:MSVC>:/W4>
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-Werror;-Wall;-Wno-self-assign-overloaded;-Wno-unknown-warning-option>
    )

    set_target_properties(test_indirect PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    if (ENABLE_SANITIZERS)
        set(SANITIZER_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")
        set(SANITIZER_FLAGS_UBSAN "-fsanitize=undefined")

        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("${SANITIZER_FLAGS_ASAN}" COMPILER_SUPPORTS_ASAN)
        check_cxx_compiler_flag("${SANITIZER_FLAGS_UBSAN}" COMPILER_SUPPORTS_UBSAN)

        if (COMPILER_SUPPORTS_ASAN)
            add_library(asan INTERFACE IMPORTED)
            set_target_properties(asan PROPERTIES
                INTERFACE_COMPILE_OPTIONS "${SANITIZER_FLAGS_ASAN}"
                INTERFACE_LINK_OPTIONS "${SANITIZER_FLAGS_ASAN}"
            )
            target_link_libraries(test_indirect
                PRIVATE
                    asan
            )
        endif(COMPILER_SUPPORTS_ASAN)

        if (COMPILER_SUPPORTS_UBSAN)
            add_library(ubsan INTERFACE IMPORTED)
            set_target_properties(ubsan PROPERTIES
                INTERFACE_COMPILE_OPTIONS "${SANITIZER_FLAGS_UBSAN}"
                INTERFACE_LINK_OPTIONS "${SANITIZER_FLAGS_UBSAN}"
            )
            target_link_libraries(test_indirect
                PRIVATE
                    ubsan
            )
        endif(COMPILER_SUPPORTS_UBSAN)
    endif(ENABLE_SANITIZERS)

    enable_testing()
    add_test(
        NAME test_indirect
        COMMAND test_indirect
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
    include(Catch)
    catch_discover_tests(test_indirect)

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/indirect.h"
        DESTINATION
            include
    )

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
        DESTINATION
            ./
    )

    install(
        TARGETS indirect
        EXPORT indirect_target
        DESTINATION lib/cmake/indirect
    )

    install(
        EXPORT indirect_target
        NAMESPACE indirect::
        DESTINATION lib/cmake/indirect
	    FILE indirect-target.cmake
    )

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/indirect-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/indirect-config.cmake
        INSTALL_DESTINATION
            lib/cmake/indirect
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/indirect-version.cmake
        VERSION ${indirect_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/indirect-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/indirect-version.cmake
        DESTINATION
            lib/cmake/indirect
    )
endif()
