# M17 ChiragRathi Autonomous Celestial Discovery Framework
# High-Performance Multi-Language Research Implementation
#
# Author: ChiragRathi
# Version: 2.0.0
# Date: 2026

cmake_minimum_required(VERSION 3.20)
project(M17_ChiragRathi 
    VERSION 2.0.0
    DESCRIPTION "ChiragRathi M17 Autonomous Celestial Discovery Framework"
    HOMEPAGE_URL "https://github.com/chiragrathiresearcher/m17-framework"
    LANGUAGES CXX C Fortran
)

# Project configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING 
        "Choose the type of build (Debug Release RelWithDebInfo MinSizeRel)" FORCE)
endif()

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG -fsanitize=address")
    
    # Enable vectorization reports for performance analysis
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ftree-vectorize -fopt-info-vec")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Rpass=loop-vectorize")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG /arch:AVX2")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /DDEBUG")
endif()

# Feature detection
include(CheckCXXSourceCompiles)
include(CheckIncludeFile)

# Check for SIMD support
check_cxx_source_compiles("
#include <immintrin.h>
int main() { __m256d v = _mm256_setzero_pd(); return 0; }" 
HAS_AVX2)

check_cxx_source_compiles("
#include <immintrin.h>
int main() { __m512d v = _mm512_setzero_pd(); return 0; }" 
HAS_AVX512)

# Check for OpenMP
find_package(OpenMP REQUIRED)

# Options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(BUILD_EXAMPLES "Build usage examples" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(BUILD_DOCUMENTATION "Build documentation" OFF)
option(ENABLE_SIMD "Enable SIMD optimizations" ON)
option(ENABLE_GPU "Enable GPU acceleration" ON)
option(ENABLE_PROFILING "Enable performance profiling" OFF)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Eigen3 3.4 REQUIRED NO_MODULE)
find_package(Boost 1.80 REQUIRED COMPONENTS 
    system filesystem program_options numeric_odeint)

# Optional packages
find_package(TBB QUIET)
if(TBB_FOUND)
    message(STATUS "Intel TBB found - enabling parallel algorithms")
    add_compile_definitions(HAS_TBB)
endif()

# GPU support
if(ENABLE_GPU)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        enable_language(CUDA)
        set(CMAKE_CUDA_STANDARD 17)
        set(CMAKE_CUDA_STANDARD_REQUIRED ON)
        message(STATUS "CUDA found - enabling GPU acceleration")
        add_compile_definitions(HAS_CUDA)
    endif()
endif()

# Fortran configuration for numerical routines
if(CMAKE_Fortran_COMPILER)
    set(CMAKE_Fortran_FLAGS_RELEASE "-O3 -march=native -fopenmp")
    set(CMAKE_Fortran_FLAGS_DEBUG "-O0 -g -fcheck=all -fopenmp")
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external
    ${EIGEN3_INCLUDE_DIRS}
)

# Compiler definitions
if(ENABLE_SIMD AND HAS_AVX512)
    add_compile_definitions(HAS_AVX512)
    message(STATUS "AVX-512 support enabled")
elseif(ENABLE_SIMD AND HAS_AVX2)
    add_compile_definitions(HAS_AVX2)
    message(STATUS "AVX2 support enabled")
endif()

if(ENABLE_PROFILING)
    add_compile_definitions(ENABLE_PROFILING)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    endif()
endif()

# External dependencies
add_subdirectory(external)

# Core library components
add_subdirectory(src/core)

# Fortran numerical routines
if(CMAKE_Fortran_COMPILER)
    add_subdirectory(src/fortran)
endif()

# Python bindings
if(BUILD_PYTHON_BINDINGS)
    find_package(pybind11 CONFIG)
    if(pybind11_FOUND)
        add_subdirectory(src/python)
        message(STATUS "Python bindings will be built")
    else()
        message(WARNING "pybind11 not found - Python bindings disabled")
        set(BUILD_PYTHON_BINDINGS OFF)
    endif()
endif()

# Rust components (external build)
find_program(CARGO_EXECUTABLE cargo)
if(CARGO_EXECUTABLE)
    message(STATUS "Cargo found - Rust components available")
    add_custom_target(rust_components
        COMMAND ${CARGO_EXECUTABLE} build --release
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/rust
        COMMENT "Building Rust components"
    )
endif()

# Main executable
add_executable(m17_core
    src/main.cpp
)

target_link_libraries(m17_core
    PRIVATE
        chiragrathi_uncertainty
        chiragrathi_physics
        chiragrathi_memory
        chiragrathi_fusion
        chiragrathi_trust
        chiragrathi_integration
        Eigen3::Eigen
        Boost::system
        Boost::filesystem
        Boost::program_options
        OpenMP::OpenMP_CXX
        Threads::Threads
)

if(TBB_FOUND)
    target_link_libraries(m17_core PRIVATE TBB::tbb)
endif()

if(CUDAToolkit_FOUND)
    target_link_libraries(m17_core PRIVATE CUDA::cudart CUDA::cufft)
endif()

# Benchmarking executable
if(BUILD_BENCHMARKS)
    add_executable(m17_benchmark
        benchmarks/main_benchmark.cpp
        benchmarks/uncertainty_benchmark.cpp
        benchmarks/physics_benchmark.cpp
        benchmarks/memory_benchmark.cpp
    )
    
    target_link_libraries(m17_benchmark
        PRIVATE
            chiragrathi_uncertainty
            chiragrathi_physics
            chiragrathi_memory
            Eigen3::Eigen
            Boost::system
            OpenMP::OpenMP_CXX
    )
endif()

# Validation executable  
add_executable(m17_validate
    tools/validation/main_validation.cpp
    tools/validation/gaia_validation.cpp
    tools/validation/jpl_validation.cpp
    tools/validation/gps_validation.cpp
)

target_link_libraries(m17_validate
    PRIVATE
        chiragrathi_uncertainty
        chiragrathi_physics
        Eigen3::Eigen
        Boost::system
        Boost::program_options
)

# Testing
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include(GoogleTest)
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation
if(BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    else()
        message("Doxygen required to build documentation")
    endif()
endif()

# Installation
include(GNUInstallDirs)

# Install executables
install(TARGETS m17_core m17_validate
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_BENCHMARKS)
    install(TARGETS m17_benchmark
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Install libraries
install(TARGETS 
    chiragrathi_uncertainty
    chiragrathi_physics
    chiragrathi_memory
    chiragrathi_fusion
    chiragrathi_trust
    chiragrathi_integration
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install headers
install(DIRECTORY src/core/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chiragrathiresearcher/m17
    FILES_MATCHING PATTERN "*.hpp"
)

# Install data files
install(DIRECTORY data/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/chiragrathiresearcher/m17
    PATTERN "*.txt"
    PATTERN "*.dat"
    PATTERN "*.json"
)

# Package configuration
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/M17Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/M17Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/M17
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/M17ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/M17Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/M17ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/M17
)

# CPack configuration for distribution
set(CPACK_PACKAGE_NAME "M17-ChiragRathi")
set(CPACK_PACKAGE_VENDOR "ChiragRathi")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Autonomous Celestial Discovery Framework")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "chirag.rathi@example.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ;DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    
    # DEB package specific
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libeigen3-dev, libboost-all-dev")
    set(CPACK_DEBIAN_PACKAGE_SECTION "science")
    
    # RPM package specific  
    set(CPACK_RPM_PACKAGE_REQUIRES "eigen3-devel, boost-devel")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Science")
endif()

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "M17 ChiragRathi Configuration Summary:")
message(STATUS "  Version:              ${PROJECT_VERSION}")
message(STATUS "  Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:         ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  SIMD support:         ${ENABLE_SIMD} (AVX2: ${HAS_AVX2}, AVX-512: ${HAS_AVX512})")
message(STATUS "  GPU support:          ${ENABLE_GPU} (CUDA: ${CUDAToolkit_FOUND})")
message(STATUS "  OpenMP:               ${OpenMP_FOUND}")
message(STATUS "  Intel TBB:            ${TBB_FOUND}")
message(STATUS "  Fortran:              ${CMAKE_Fortran_COMPILER}")
message(STATUS "  Python bindings:      ${BUILD_PYTHON_BINDINGS}")
message(STATUS "  Rust components:      ${CARGO_EXECUTABLE}")
message(STATUS "")
message(STATUS "Build targets:")
message(STATUS "  Core library:         ON")
message(STATUS "  Tests:                ${BUILD_TESTS}")
message(STATUS "  Benchmarks:           ${BUILD_BENCHMARKS}")
message(STATUS "  Examples:             ${BUILD_EXAMPLES}")
message(STATUS "  Documentation:        ${BUILD_DOCUMENTATION}")
message(STATUS "")

# Performance optimization warnings
if(NOT ENABLE_SIMD)
    message(WARNING "SIMD optimizations disabled - performance will be reduced")
endif()

if(NOT OpenMP_FOUND)
    message(WARNING "OpenMP not found - parallel performance will be limited")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(WARNING "Debug build - performance will be significantly reduced")
endif()
