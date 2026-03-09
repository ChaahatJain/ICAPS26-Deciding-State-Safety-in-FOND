
# Set compiler flags:

if (CMAKE_COMPILER_IS_GNUCXX)
    set(IS_GCC ON)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set(IS_CC ON)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL AppleClang)
    set(IS_APPLE_CC ON)
endif ()

########################################################################################################################

# Compiler flags adapted from a Fast Downward code base (http://gogs.cs.uni-saarland.de:3000/fai_group/FD-Stripped/src/master/src/cmake_modules/FastDownwardMacros.cmake [October 2018])

if (IS_GCC OR IS_CC OR IS_APPLE_CC)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-std=c++17" CXX17_FOUND)
    if (CXX17_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
	message("Here ${CMAKE_CXX_FLAGS}")
    else ()
        message(FATAL_ERROR "${CMAKE_CXX_COMPILER} does not support C++17, please use a different compiler")
    endif ()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror -Wno-sign-compare -Wno-deprecated -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-result -Wno-maybe-uninitialized")
    # unknown-pragmas: suppressed to allow ide diagnostics
    # set -Wno-error=unknown-pragmas to see warnings

    ## Configuration-specific flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fomit-frame-pointer -fno-stack-protector") # @plaja -fno-stack-protector; is subject to reconsider
    set(CMAKE_CXX_FLAGS_DEBUG "-O3 -DRUNTIME_CHECKS") # @plaja -DRUNTIME_CHECKS; subject to consider whether to have an additional "release" build for runtime checks
    #  --coverage to support coverage
else ()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER}")
endif ()

########################################################################################################################

# Set flags for dependencies:

if (NOT EXISTS ${Z3_DIR})
    if (${BUILD_PA} OR ${BUILD_BMC}) # fall back to default if dependency is actually required for some enabled module ...
        if (${ENABLE_DEFAULT_INSTALLATION})
            message("---------------------------------------------------------------------------")
            message(STATUS "Z3 path is not valid. Performing default installation ...")
            execute_process(COMMAND ${LOCAL_DEPENDENCIES_PATH}/install_z3.sh)
            set(Z3_DIR ${LOCAL_DEPENDENCIES_PATH}/z3)
            set(Z3_BUILD_DIR ${Z3_DIR}/build)
            if (EXISTS ${Z3_DIR})
                message(STATUS "Default installation of Z3 successful.")
            else ()
                message(WARNING "Default installation of Z3 failed.")
            endif ()
            message("---------------------------------------------------------------------------")
        endif ()
    endif ()
endif ()
#
if (EXISTS ${Z3_DIR})
    message ("Z3 directory is found")
    message(${Z3_DIR})
    set(USE_Z3 ON)
else ()
    message ("Z3 directory is not found")
    message(${Z3_DIR})
    set(USE_Z3 OFF)
endif ()
############################
if (NOT EXISTS ${CUDD_DIR})
    if (${BUILD_FAULT_ANALYSIS} AND ${BUILD_BDD_SUPPORT})
        message("---------------------------------------")
        message(STATUS "CUDD path is not valid. Performing default installation ...")
        execute_process(COMMAND ${LOCAL_DEPENDENCIES_PATH}/install_cudd.sh)
        set(CUDD_DIR ${LOCAL_DEPENDENCIES_PATH}/cudd)
        if (EXISTS ${CUDD_DIR})
            message(STATUS "Default installation of cudd is successful.")
        else ()
            message(WARNING "Default installation of cudd failed.")
        endif ()
        message("------------------------------------")
    endif ()
endif ()

if (EXISTS ${CUDD_DIR})
    set(USE_CUDD ON)
else ()
    set(USE_CUDD OFF)
endif ()
#############################
if (NOT EXISTS "${VERITAS_DIR}")
    message(${VERITAS_DIR})
    if ((${BUILD_PA} OR ${BUILD_FAULT_ANALYSIS}) AND ${ENABLE_DEFAULT_INSTALLATION})
        message("---------------------------------------")
        message(STATUS "Veritas path is not valid. Performing default installation ...")
        execute_process(COMMAND ${LOCAL_DEPENDENCIES_PATH}/install_veritas.sh)
        set(VERITAS_DIR ${LOCAL_DEPENDENCIES_PATH}/veritas)
        if (EXISTS ${VERITAS_DIR})
            message(STATUS "Default installation of veritas is successful.")
        else ()
            message(WARNING "Default installation of veritas failed.")
        endif ()
        message("------------------------------------")
    endif ()
endif ()

if (EXISTS ${VERITAS_DIR})
    set(USE_VERITAS ON)
    message("USE VERITAS IS ON")
else ()
    set(USE_VERITAS OFF)
endif ()
#############################

if (NOT EXISTS ${Marabou_DIR})
    if (${BUILD_PA} OR ${BUILD_BMC}) # fall back to default if dependency is actually required for some enabled module ...
        if (${ENABLE_DEFAULT_INSTALLATION})
            message("---------------------------------------------------------------------------")
            message(STATUS "Marabou path is not valid. Performing default installation ...")
            execute_process(COMMAND ${LOCAL_DEPENDENCIES_PATH}/install_marabou.sh)
            set(Marabou_DIR ${LOCAL_DEPENDENCIES_PATH}/Marabou)
            set(Marabou_BUILD_DIR ${Marabou_DIR}/build)
            set(Openblas_DIR ${Marabou_DIR}/tools/OpenBLAS-0.3.19/installed/lib)
            set(Gurobi_DIR)
            if (EXISTS ${Marabou_DIR})
                message(STATUS "Default installation of Marabou successful.")
            else ()
                message(WARNING "Default installation of Marabou failed.")
            endif ()
            message("---------------------------------------------------------------------------")
        endif ()
    endif ()
endif ()
#
if (EXISTS ${Marabou_DIR})
    set(USE_MARABOU ON)
else ()
    set(USE_MARABOU OFF)
endif ()

#############################

if (NOT EXISTS ${Torch_DIR})
    if (${BUILD_POLICY_LEARNING}) # fall back to default if dependency is actually required for some enabled module ...
        if (${ENABLE_DEFAULT_INSTALLATION})
            message("---------------------------------------------------------------------------")
            message(STATUS "Torch path is not valid. Performing default installation ...")
            execute_process(COMMAND ${LOCAL_DEPENDENCIES_PATH}/install_torch.sh)
            set(Torch_DIR ${LOCAL_DEPENDENCIES_PATH}/libtorch/share/cmake/Torch)
            if (EXISTS ${Torch_DIR})
                message(STATUS "Default installation of Torch successful.")
            else ()
                message(WARNING "Default installation of Torch failed.")
            endif ()
            message("---------------------------------------------------------------------------")
        endif ()
    endif ()
endif ()
#
if (${BUILD_POLICY_LEARNING} AND EXISTS ${Torch_DIR})   # only use dependency if required by enabled module
    set(USE_TORCH ON)
else ()
    set(USE_TORCH OFF)
endif ()

#############################

if (NOT EXISTS ${pybind11_DIR})
    if (${BUILD_PA} AND ${USE_ADVERSARIAL_ATTACK}) # fall back to default if dependency is actually required for some enabled module ...
        if (${ENABLE_DEFAULT_INSTALLATION})
            message("---------------------------------------------------------------------------")
            message(STATUS "Pybind11 path is not valid. Performing default installation ...")
            execute_process(COMMAND ${LOCAL_DEPENDENCIES_PATH}/install_pybind11.sh)
            set(pybind11_DIR ${LOCAL_DEPENDENCIES_PATH}/pybind11)
            if (EXISTS ${pybind11_DIR})
                message(STATUS "Default installation of Pybind11 successful.")
            else ()
                message(WARNING "Default installation of Pybind11 failed.")
            endif ()
            message("---------------------------------------------------------------------------")
        endif ()
    endif ()
endif ()
#
if (${BUILD_PA} AND ${USE_ADVERSARIAL_ATTACK} AND EXISTS ${pybind11_DIR}) # only use dependency if required by enabled module
    set(USE_PYBIND ON)
    if (NOT EXISTS ${PYTHON_ENV})
        execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/adversarial_attack/python/env/install_py_env.sh)
    endif ()
else ()
    set(USE_PYBIND OFF)
endif ()

########################################################################################################################

# Dependency checks:

if (NOT EXISTS ${NLOHMANN_PATH})
    if (${ENABLE_DEFAULT_INSTALLATION})
        message("---------------------------------------------------------------------------")
        message(STATUS "Nlohmann JSON library path is not valid. Performing default installation ...")
        execute_process(COMMAND ${LOCAL_DEPENDENCIES_PATH}/install_nlohmann.sh)
        set(NLOHMANN_PATH ${LOCAL_DEPENDENCIES_PATH}/json/single_include/nlohmann)
        if (EXISTS ${NLOHMANN_PATH})
            message(STATUS "Default installation of nlohmann JSON library successful.")
        else ()
            message(WARNING "Default installation of nlohmann JSON library failed.")
        endif ()
        message("---------------------------------------------------------------------------")
    endif ()
    if (NOT EXISTS ${NLOHMANN_PATH})
        message(FATAL_ERROR "Nlohmann JSON library is required.")
    endif ()
endif ()

if (${BUILD_PA})
    if (NOT ${USE_Z3} OR NOT ${USE_MARABOU})
        message(FATAL_ERROR "PREDICATE ABSTRACTION requires Z3 and MARABOU.")
    endif ()
else ()
    if (${USE_ADVERSARIAL_ATTACK})
        message(WARNING "ADVERSARIAL ATTACK flag will be ignored since PA module is disabled.")
    endif ()
    SET(USE_ADVERSARIAL_ATTACK OFF)
endif ()

if (${USE_ADVERSARIAL_ATTACK})
    if (NOT ${USE_PYBIND})
        message(FATAL_ERROR "ADVERSARIAL ATTACK requires PYBIND.")
    endif ()
endif ()

if (${BUILD_BMC})
    if (NOT ${USE_Z3})
        message(FATAL_ERROR "BMC requires Z3. Marabou is recommend.")
    endif ()
endif ()

if (${BUILD_INVARIANT_STRENGTHENING})
    if (NOT ${BUILD_PA})
        message(FATAL_ERROR "Invariant strengthening require Predicate Abstraction module.")
    endif ()
endif ()

if (${BUILD_POLICY_LEARNING})
    if (NOT ${USE_TORCH})
        message(FATAL_ERROR "POLICY LEARNING requires TORCH.")
    endif ()
endif ()

if (${USE_TORCH} AND ${USE_PYBIND})
    message(FATAL_ERROR "Builds with TORCH and Pybind are not supported.")
endif ()

if (${USE_MARABOU})
    if (NOT EXISTS ${CXXTEST_PATH})
        if (${ENABLE_DEFAULT_INSTALLATION})
            message("---------------------------------------------------------------------------")
            message(STATUS "CxxTest path is not valid. Performing default installation ...")
            execute_process(COMMAND ${LOCAL_DEPENDENCIES_PATH}/install_cxxtest.sh)
            set(CXXTEST_PATH ${LOCAL_DEPENDENCIES_PATH}/cxxtest)
            if (EXISTS ${CXXTEST_PATH})
                message(STATUS "Default installation of CxxTest successful.")
            else ()
                message(WARNING "Default installation of CxxTest failed.")
            endif ()
            message("---------------------------------------------------------------------------")
        endif ()
    endif ()
    if (NOT EXISTS ${CXXTEST_PATH})
        message(FATAL_ERROR "Marabou requires CXX_TEST.")
    endif ()
endif ()

########################################################################################################################

# Dependency configuration:
set(LIBS)
set(INCLUDES)

# Veritas
if (${USE_VERITAS})
    # Use find_library to locate the Veritas library
    find_library(VERITAS_LIBRARY NAMES veritas PATHS ${VERITAS_DIR}/manual_build)

    # Check if the library was found
    if(NOT VERITAS_LIBRARY)
        message(FATAL_ERROR "Could not find the Veritas library in ${VERITAS_DIR}/manual_build")
    endif()
    list(APPEND LIBS ${VERITAS_LIBRARY})
    list(APPEND INCLUDES ${VERITAS_DIR})
    list(APPEND INCLUDES ${VERITAS_DIR}/src/cpp)
endif()

# CUDD
if (${USE_CUDD})
    message(${CUDD_DIR})
    find_library(CUDD_LIBRARY
                NAMES cudd
                PATHS ${CUDD_DIR}/cudd/.libs /usr/lib /usr/local/lib)

    find_library(M_LIBRARY
                NAMES m
                PATHS /usr/lib /usr/local/lib)

    find_library(UTIL_LIBRARY
                NAMES util
                PATHS ${CUDD_DIR}/lib /usr/lib /usr/local/lib)


    if(NOT CUDD_LIBRARY)
        message(FATAL_ERROR "Could not find the CUDD library")
    endif()

    if(NOT M_LIBRARY)
        message(FATAL_ERROR "Could not find the math library")
    endif()

    if(NOT UTIL_LIBRARY)
        message(FATAL_ERROR "Could not find the util library")
    endif()
    # Include the CUDD header files
    list (APPEND INCLUDES ${CUDD_DIR}/cudd ${CUDD_DIR}/util ${CUDD_DIR}/include ${CUDD_DIR}/dddmp ${CUDD_DIR})

    # Link the CUDD libraries
    set(CUDD_LIBRARIES ${CUDD_LIBRARY} ${M_LIBRARY} ${UTIL_LIBRARY})

    list (APPEND LIBS ${CUDD_LIBRARIES})
endif()

# Gurobi
if (${SUPPORT_GUROBI})
  list(APPEND INCLUDES ${GUROBI_DIR})
  if (APPLE)
    list(APPEND INCLUDES ${GUROBI_DIR}/../../lib/)
  else ()
    list(APPEND INCLUDES ${GUROBI_DIR}/lib)
  endif ()
  message(STATUS "Using Gurobi for MILP ensemble encoding")
  if (NOT EXISTS ${GUROBI_DIR})
    message("Gurobi Path not found: " ${GUROBI_DIR})
  endif()
  set (USE_GUROBI ON)

  set(GUROBI_LIB1 "gurobi_c++")
  set(GUROBI_LIB2 "gurobi120")
  #set(GUROBI_LIB2 "gurobi951")

  add_library(${GUROBI_LIB1} SHARED IMPORTED)
  set_target_properties(${GUROBI_LIB1} PROPERTIES IMPORTED_LOCATION ${GUROBI_DIR}/lib/libgurobi_c++.a)
  list(APPEND LIBS ${GUROBI_LIB1})
  target_include_directories(${GUROBI_LIB1} INTERFACE ${GUROBI_DIR}/include/)

  add_library(${GUROBI_LIB2} SHARED IMPORTED)

  # MACOSx uses .dylib instead of .so for its Gurobi downloads.
  if (APPLE)
    set_target_properties(${GUROBI_LIB2} PROPERTIES IMPORTED_LOCATION ${GUROBI_DIR}/lib/libgurobi120.dylib)
  else()
    set_target_properties(${GUROBI_LIB2} PROPERTIES IMPORTED_LOCATION ${GUROBI_DIR}/lib/libgurobi120.so)
#    set_target_properties(${GUROBI_LIB2} PROPERTIES IMPORTED_LOCATION ${GUROBI_DIR}/lib/libgurobi95.so)
  endif ()

  list(APPEND LIBS ${GUROBI_LIB2})
  target_include_directories(${GUROBI_LIB2} INTERFACE ${GUROBI_DIR}/include/)
else()
  set (USE_GUROBI OFF)
endif()

# JSON parser
list(APPEND INCLUDES ${NLOHMANN_PATH})

# Z3
if (${USE_Z3})
    find_library(Z3_LIB z3 REQUIRED PATHS ${Z3_BUILD_DIR} NO_DEFAULT_PATH) # build expected to be in /build
    list(APPEND LIBS ${Z3_LIB})

    set(Z3_INCLUDE_DIRS ${Z3_DIR}/src/api ${Z3_DIR}/src/api/c++)
    list(APPEND INCLUDES ${Z3_INCLUDE_DIRS})
endif ()

# Marabou
if (${USE_MARABOU})

    if (NOT ${USE_Z3})
        message(FATAL_ERROR "Marabou infrastructure requires Z3.")
    endif ()

    set(Marabou_SRC_DIR ${Marabou_DIR}/src)
    set(Marabou_DEPS_DIR ${Marabou_DIR}/deps)
    find_library(MarabouHelper_LIB MarabouHelper REQUIRED PATHS ${Marabou_BUILD_DIR} NO_DEFAULT_PATH)
    list(APPEND LIBS ${MarabouHelper_LIB})

    if (NOT EXISTS ${Openblas_DIR}) # (old) fall-back/default build on MacOS does not support Openblas
        message(WARNING "Building without OpenBlas.")
    else ()
        find_library(Openblas_LIB openblas REQUIRED PATHS ${Openblas_DIR} NO_DEFAULT_PATH)
        list(APPEND LIBS ${Openblas_LIB})
    endif ()

    if (EXISTS ${GUROBI_DIR})
        find_library(Gurobi_LIB1 gurobi_c++ REQUIRED PATHS ${GUROBI_DIR}/lib NO_DEFAULT_PATH)
        list(APPEND LIBS ${Gurobi_LIB1})
        find_library(Gurobi_LIB2 gurobi120 REQUIRED PATHS ${GUROBI_DIR}/lib NO_DEFAULT_PATH)
#        find_library(Gurobi_LIB2 gurobi95 REQUIRED PATHS ${GUROBI_DIR}/lib NO_DEFAULT_PATH)
        list(APPEND LIBS ${Gurobi_LIB2})
        set(SUPPORT_GUROBI ON)
        list(APPEND INCLUDES ${GUROBI_DIR}/include/)
    endif ()

    find_package(Boost REQUIRED COMPONENTS program_options timer chrono thread) # subsumes: SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    list(APPEND LIBS ${Boost_LIBRARIES})
    # list(APPEND INCLUDES ${Boost_INCLUDE_DIRS}) # package: no need to add explicitly

    if (NOT EXISTS ${Marabou_DIR}/c++/api)
        message(FATAL_ERROR "Build script assumes Marabou API generated by script in ${Marabou_DIR}/c++/. Alternatively, set Marabou includes manually in DependenciesConfig.cmake.")
    endif ()

    set(Marabou_INCLUDE_DIRS
        ${Marabou_DIR}/c++/api/
        # ${Marabou_SRC_DIR}/basis_factorization/
        # ${Marabou_SRC_DIR}/common
        # ${Marabou_SRC_DIR}/configuration
        # ${Marabou_SRC_DIR}/engine
        # ${Marabou_SRC_DIR}/input_parsers
        # ${Marabou_SRC_DIR}/nlr
        ${Marabou_DEPS_DIR}/CVC4
        ${Marabou_DEPS_DIR}/CVC4/include
        # also requires to include a cxx test implementation (as included Marabou files (indirectly) include cxx test files)
        # so far the one (formally) used for tests (see below) suffices -- even tough it differs from the one used by Marabou itself (${Marabou_DIR}/tools/cxxtest)
    )
    list(APPEND INCLUDES ${Marabou_INCLUDE_DIRS})

endif ()

if (${USE_MARABOU})   # only use dependency if required by enabled module
    if (EXISTS ${CXXTEST_PATH})
        list(APPEND INCLUDES ${CXXTEST_PATH})  # required for Marabou includes (see above)
    endif ()
endif ()

# Torch

if (${USE_TORCH})
    find_package(Torch REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
    list(APPEND LIBS ${TORCH_LIBRARIES})
    # list(APPEND INCLUDES ${TORCH_INCLUDE_DIRS}) # package: no need to add explicitly
endif (${USE_TORCH})

# Pybind

if (${USE_PYBIND})
    set(PYTHON_ENV_BIN ${PYTHON_ENV}/bin)
    # set(PYTHON_EXECUTABLE ${PYTHON_ENV_BIN}/python) # CACHE FILEPATH "python environment executable")
    if (UNIX AND NOT APPLE)
        set(PYTHON_ENV_LIB "${PYTHON_ENV}/lib64")
    else ()
        set(PYTHON_ENV_LIB "${PYTHON_ENV}/lib")
    endif ()
    set(PYTHON_ENV_PACKAGES "${PYTHON_ENV_LIB}/python3.9/site-packages")
    # set(ENV{PYTHONPATH} "${PYTHON_ENV_PACKAGES}:$ENV{PYTHONPATH}")
    # set(ENV{PATH} "${PYTHON_ENV_BIN}:${PYTHON_ENV_LIB}:$ENV{PATH}")
    set(CUSTOM_SYS_PATH "\"${PYTHON_ENV_PACKAGES}\"")
    set(ATTACK_MODULE "\"${CMAKE_CURRENT_SOURCE_DIR}/adversarial_attack/python/adversarial_attacks\"")
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/pybind_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/pybind_config.h)
    add_subdirectory(${pybind11_DIR} ${CMAKE_CURRENT_BINARY_DIR}/pybind)

    # Alternative ?
    # set(pybind11_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/pybind11/install/share/cmake/pybind11) # Warning: May use a version installed globally anyway!
    # find_package(pybind11 REQUIRED)
    # # list(APPEND INCLUDES ${PYBIND_INCLUDE_DIRS}) # package: no need to add explicitly

    list(APPEND LIBS pybind11::embed)
endif (${USE_PYBIND})
