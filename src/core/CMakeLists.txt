# src/core/CMakeLists.txt
# Defines the chiragrathi_* component libraries.
#
# Modules with a .cpp are built as STATIC libraries; header-only modules
# are built as INTERFACE libraries so downstream targets can still
# target_link_libraries() against them uniformly.
#
# NOTE: "trust" (chiragrathi_trust) has no source directory yet
# (src/core/trust/ does not exist). It is added conditionally below so the
# build keeps working today and picks it up automatically the moment the
# directory + sources are added — no CMake changes needed at that point.

# --- uncertainty (compiled) ---------------------------------------------
add_library(chiragrathi_uncertainty STATIC
    uncertainty/uncertainty_engine.cpp
)
target_include_directories(chiragrathi_uncertainty PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(chiragrathi_uncertainty PUBLIC Eigen3::Eigen)

# --- physics (compiled) --------------------------------------------------
add_library(chiragrathi_physics STATIC
    physics/relativistic_physics.cpp
)
target_include_directories(chiragrathi_physics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(chiragrathi_physics PUBLIC Eigen3::Eigen)

# --- memory (header-only) ------------------------------------------------
add_library(chiragrathi_memory INTERFACE)
target_include_directories(chiragrathi_memory INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# --- fusion (header-only) -------------------------------------------------
add_library(chiragrathi_fusion INTERFACE)
target_include_directories(chiragrathi_fusion INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(chiragrathi_fusion INTERFACE chiragrathi_uncertainty)

# --- integration (header-only) -------------------------------------------
add_library(chiragrathi_integration INTERFACE)
target_include_directories(chiragrathi_integration INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(chiragrathi_integration INTERFACE
    chiragrathi_uncertainty
    chiragrathi_physics
)

# --- trust (NOT YET IMPLEMENTED) ------------------------------------------
# Expected layout once added: src/core/trust/{*.cpp,*.hpp}
# Add a matching add_library(chiragrathi_trust ...) block above this comment
# following the same pattern as "physics" or "uncertainty" once source
# files exist.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/trust)
    file(GLOB TRUST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/trust/*.cpp)
    if(TRUST_SOURCES)
        add_library(chiragrathi_trust STATIC ${TRUST_SOURCES})
        target_include_directories(chiragrathi_trust PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
        )
        message(STATUS "chiragrathi_trust: found sources, library will be built")
    else()
        message(WARNING "src/core/trust/ exists but has no .cpp files - chiragrathi_trust will NOT be built")
    endif()
else()
    message(WARNING "src/core/trust/ does not exist yet - chiragrathi_trust will NOT be built (see CONTRIBUTING.md)")
endif()
