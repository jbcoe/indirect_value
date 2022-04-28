cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(NOT DEFINED PROJECT_NAME)
    set(CPP_INDIRECT_IS_SUBPROJECT ON)
endif()

set(CPP_INDIRECT_VALUE_VERSION 0.0.1)

project(indirect_value LANGUAGES CXX VERSION ${CPP_INDIRECT_VALUE_VERSION})

include(CTest)
include(FetchContent)
include(GNUInstallDirs)
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)

# Ensure code coverage is switched off for Mac OS until the code coverage library addresses the AppleClang issue
cmake_dependent_option(ENABLE_CODE_COVERAGE "Enable code coverage" ON "\"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"Clang\" OR \"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"GNU\"" OFF)
cmake_dependent_option(ENABLE_INCLUDE_NATVIS "Enable inclusion of a natvis file for debugging" ON "\"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"MSVC\"" OFF)
option(ENABLE_SANITIZERS "Enable Address Sanitizer and Undefined Behaviour Sanitizer if available" OFF)

add_subdirectory(documentation)

add_library(indirect_value INTERFACE)
target_include_directories(indirect_value
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(indirect_value
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect_value.h>
        # Only include natvis files in Visual Studio
        $<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/indirect_value.natvis>>
        $<INSTALL_INTERFACE:$<$<BOOL:${ENABLE_INCLUDE_NATVIS}>:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/indirect_value.natvis>>
)

target_compile_features(indirect_value
    INTERFACE
        cxx_std_17
)

add_library(indirect_value::indirect_value ALIAS indirect_value)

if (${CPP_INDIRECT_IS_SUBPROJECT})

    if (${BUILD_TESTING})
        FetchContent_Declare(
            catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v2.x
        )

        FetchContent_GetProperties(catch2)
        if(NOT catch2_POPULATED)
            FetchContent_Populate(catch2)
            add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
        endif()

        add_executable(test_indirect_value "")
        target_sources(test_indirect_value
            PRIVATE
                example_pimpl.h
                example_pimpl.cpp
                test_pimpl.cpp
                test_indirect_value.cpp
        )

        target_link_libraries(test_indirect_value
            PRIVATE
                indirect_value::indirect_value
                Catch2::Catch2
        )

        target_compile_options(test_indirect_value
            PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
                $<$<CXX_COMPILER_ID:MSVC>:/W4>
                $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-Werror;-Wall;-Wno-self-assign-overloaded;-Wno-unknown-warning-option>
        )

        set_target_properties(test_indirect_value PROPERTIES
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
                target_link_libraries(test_indirect_value
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
                target_link_libraries(test_indirect_value
                    PRIVATE
                        ubsan
                )
            endif(COMPILER_SUPPORTS_UBSAN)
        endif(ENABLE_SANITIZERS)

        enable_testing()
        add_test(
            NAME test_indirect_value
            COMMAND test_indirect_value
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
        include(Catch)
        catch_discover_tests(test_indirect_value)

        if (ENABLE_CODE_COVERAGE)
            FetchContent_Declare(
                codecoverage
                GIT_REPOSITORY https://github.com/RWTH-HPC/CMake-codecov.git
            )

            FetchContent_GetProperties(codecoverage)
            if(NOT codecoverage_POPULATED)
                FetchContent_Populate(codecoverage)
                list(APPEND CMAKE_MODULE_PATH ${codecoverage_SOURCE_DIR}/cmake)
            endif()

            set(ENABLE_COVERAGE ON CACHE BOOL "Enable coverage build." FORCE)
            find_package(codecov)
            add_coverage(test_indirect_value)
            list(APPEND LCOV_REMOVE_PATTERNS "'/usr/*'")
            coverage_evaluate()
        endif()
    endif(${BUILD_TESTING})

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/indirect_value.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/indirect_value.natvis"
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
        DESTINATION
            "${CMAKE_INSTALL_DATAROOTDIR}/licenses/indirect_value"
    )

    install(
        TARGETS indirect_value
        EXPORT indirect_value-export-set
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(
        EXPORT indirect_value-export-set
        NAMESPACE indirect_value::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/indirect_value"
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/indirect_value-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/indirect_value-config.cmake
        INSTALL_DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/indirect_value"
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/indirect_value-version.cmake
        VERSION ${CPP_INDIRECT_VALUE_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/indirect_value-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/indirect_value-version.cmake
        DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/indirect_value"
    )
endif()
