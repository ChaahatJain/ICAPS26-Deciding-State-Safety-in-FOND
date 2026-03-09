cmake_minimum_required(VERSION 3.10)
project(PlaJA)

set(CMAKE_CXX_STANDARD 17) # using torch at least CMAKE_CXX_STANDARD 14 is recommended
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Modules ##############################################################################################################

SET(BUILD_PROB_SEARCH OFF) # PROB SEARCH ENGINE # NOT MAINTAINED
SET(BUILD_PA ON) # PREDICATE ABSTRACTION
SET(USE_ADVERSARIAL_ATTACK OFF) # SUB-CONFIG of PA that requires additional dependencies (CURRENTLY NOT MAINTAINED & PROBABLY NOT SUPPORTED)
SET(BUILD_BMC OFF) # BMC # MAINTAINED LAZILY
SET(BUILD_INVARIANT_STRENGTHENING OFF) # Invariant Strengthening # PROTOTYPE # NO LONGER MAINTAINED & SUPPORTED
SET(BUILD_SAFE_START_GENERATOR ON) # Safe Start Generator (requires PA)
SET(BUILD_FAULT_ANALYSIS ON)
SET(BUILD_POLICY_LEARNING ON) # Policy Learning
SET(BUILD_TO_NUXMV OFF) # 2-nuXmv translator.
SET(BUILD_TESTS OFF) # TESTS via CXX_TESTS
SET(BUILD_BDD_SUPPORT OFF) # To compute the unsafe region for fault analysis using CUDD

# Dependencies #########################################################################################################
# (set non-existing path to disable)
# (conversely may also try set to any valid path to enable flag while relying on cmake to find actual dependency path) #

set(DEPENDENCIES_PATH /workspace/dependencies/)

if (NOT DEFINED DEPENDENCIES_PATH OR NOT EXISTS ${DEPENDENCIES_PATH})
    if (DEFINED DEPENDENCIES_PATH AND NOT EXISTS ${DEPENDENCIES_PATH})
        message(WARNING "Dependencies path does not exist or not specified (${DEPENDENCIES_PATH}) ... using default.")
        message(WARNING "Hint: You must specify total path.")
    endif ()
    set(DEPENDENCIES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/)
    # external path as default for convenience
    # local fall-back default installation path (${CMAKE_CURRENT_SOURCE_DIR}/dependencies) would ne more naturally
endif ()

SET(BUILD_STATIC_PLAJA OFF) # How to link libraries; OFF due to shared objects in usage
SET(BUILD_STATIC_LIBRARY_FOR_DEBUG OFF) # Build static (or shared) library in debug mode. # NOTE: USING static library here can help with debugger step support.

set(NLOHMANN_PATH ${DEPENDENCIES_PATH}/json/single_include/nlohmann)
set(CXXTEST_PATH ${DEPENDENCIES_PATH}/cxxtest) # Warning: This is required for includes of Marabou!
set(Z3_DIR ${DEPENDENCIES_PATH}/z3)
set(Z3_BUILD_DIR ${Z3_DIR}/build)
# Marabou-related
set(Marabou_DIR ${DEPENDENCIES_PATH}/Marabou)
set(Marabou_BUILD_DIR ${Marabou_DIR}/build)
#
set(Openblas_DIR ${Marabou_DIR}/tools/OpenBLAS-0.3.19/installed/lib) # Marabou default installation [June 2022] (local)
#set(Openblas_DIR /data/common/opt/OpenBLAS/v0-3-27-sandybridge-fai0x/installed/lib) # On cluster.

#
set(VERITAS_DIR ${DEPENDENCIES_PATH}/veritas)
set(VERITAS_PATH ${VERITAS_DIR}/src/cpp/)

set(GUROBI_DIR /workspace/dependencies/gurobi/linux)

set(SUPPORT_GUROBI ON)
#
set(Boost_NO_SYSTEM_PATHS TRUE)
# set(BOOST_ROOT ${Marabou_DIR}/tools/boost_1_81_0)
set(Boost_DIR ${Marabou_DIR}/tools/boost_1_81_0/installed/lib/cmake/Boost-1.81.0)
#set(Boost_DIR ${Marabou_DIR}/tools/boost-1.84.0/installed/lib/cmake/Boost-1.84.0)
#
set(Torch_DIR ${DEPENDENCIES_PATH}/libtorch/share/cmake/Torch) # Warning: May use a version installed globally! (local)
# set(Torch_DIR /data/common/opt/libtorch/v2.3.1+cpu/share/cmake/Torch) # On cluster.
#
set(pybind11_DIR ${DEPENDENCIES_PATH}/pybind11) # Warning: May use a version installed globally anyway!
set(PYTHON_ENV ${CMAKE_CURRENT_SOURCE_DIR}/adversarial_attack/python/env/.venv) # For PYBIND we additionally require the path to a proper python environment

set(ENABLE_DEFAULT_INSTALLATION OFF) # enable fall-back to local default installation of dependencies in ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
set(LOCAL_DEPENDENCIES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/)

set(CUDD_DIR ${DEPENDENCIES_PATH}/cudd)
# Compile time options #################################################################################################

include(${CMAKE_CURRENT_SOURCE_DIR}/COptionsConfig.cmake)

########################################################################################################################

# Allow to specify modules, dependencies paths and compile time options via external file.
# Simplifies developer usage in combination with git ...
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external_config.cmake)
    set(DEPENDENCIES_PATH_COPY ${DEPENDENCIES_PATH})
    include(${CMAKE_CURRENT_SOURCE_DIR}/external_config.cmake)
    if (${DEPENDENCIES_PATH_COPY} STREQUAL ${DEPENDENCIES_PATH})
    else ()
        message(WARNING "Updated DEPENDENCIES_PATH via external_config.cmake (\"${DEPENDENCIES_PATH_COPY}\" -> \"${DEPENDENCIES_PATH}\"). This may cause unintended behavior. DEPENDENCIES_PATH should be set as CMake option via commandline.")
    endif ()
endif ()

# Targets
set(PLAJA_LIB PlaJAHelper) # the project modulo main, usable e.g. for testing
set(PLAJA PlaJA) # the main executable

# Dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/DependenciesConfig.cmake)
# set preprocessor flags for source files:
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/compiler_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler_config.h) # compiler flags
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/factory_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/factory_config.h) # submodules
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/marabou_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/marabou_config.h) # compile time options (PA)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/pa_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/pa_config.h) # compile time options (PA)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/ct_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/ct_config.h) # compile time options (PA)

# Sources
include(${CMAKE_CURRENT_SOURCE_DIR}/PlaJAFiles.cmake)



# Debug Build
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")

    if (USE_PYBIND)
        SET(BUILD_STATIC_LIBRARY_FOR_DEBUG ON) # Apparently due to combination of pybind (importing torch) and Marabou.
    endif ()

    # Helper library
    if (BUILD_STATIC_LIBRARY_FOR_DEBUG)
        add_library(${PLAJA_LIB} STATIC ${PLAJA_SOURCES})
    else ()
        add_library(${PLAJA_LIB} SHARED ${PLAJA_SOURCES}
                )
    endif ()
    # link libs
    if (BUILD_STATIC_PLAJA)
        # link statically
        target_link_libraries(${PLAJA_LIB} ${LIBS} -static)
    else ()
        target_link_libraries(${PLAJA_LIB} ${LIBS})
    endif ()
    # includes
    target_include_directories(${PLAJA_LIB} PRIVATE ${INCLUDES})

    #EXECUTABLES############################################################################################################

    # PlaJA
    add_executable(${PLAJA} main.cpp)
    target_link_libraries(${PLAJA} ${PLAJA_LIB})
    target_include_directories(${PLAJA} PRIVATE ${INCLUDES})

    # TESTS
    if (${BUILD_TESTS})
        enable_testing() # must (apparently) be in the main directory to have an impact
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests/)
    endif (${BUILD_TESTS})

endif ()

# Release Build
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    # PlaJA
    add_executable(${PLAJA} ${PLAJA_SOURCES} main.cpp)
    # link libs
    if (BUILD_STATIC_PLAJA)
        # link statically
        target_link_libraries(${PLAJA} ${LIBS} -static)
    else ()
        target_link_libraries(${PLAJA} ${LIBS})
    endif ()
    target_include_directories(${PLAJA} PRIVATE ${INCLUDES})
endif ()


