#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2020) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

# cmake_minimum_required(VERSION 3.10) # no longer needed as tests are now CMake subdirectory

########################################################################################################################
# adaption of the CxxTest cmake templates (https://github.com/CxxTest/cxxtest/tree/master/build_tools/cmake [September 2021])

set(CXXTESTGEN ${CXXTEST_PATH}/bin/cxxtestgen) # main script

function(cxx_test target test_file lib_includes includes)
    # disabling all this CxxTest stuff ...
    # set(CPP_FULL_NAME ${target}.cpp)
    # add_custom_command(
    #        OUTPUT ${CPP_FULL_NAME}
    #        COMMAND python3 ${CXXTESTGEN} --runner=ErrorPrinter --output ${CPP_FULL_NAME} ${test_file} # enforcing (arbitrary) python version for systems without "python" link to python2/3
    #        DEPENDS ${test_file}
    # )
    # add_executable(${target} ${CPP_FULL_NAME})
    add_executable(${target} ${test_file})
    # set_target_properties(${target} PROPERTIES COMPILE_FLAGS "-Wno-effc++ -Wno-ignored-qualifiers") # former is recommended by template, latter is due warning-as-error issues (alternatively use -Wno-error=ignored-qualifiers to only suppress the error)
    target_link_libraries(${target} ${lib_includes})  # libs/dependencies
    target_include_directories(${target} PRIVATE ${includes}) # headers
    add_test(${target} ${RUNTIME_OUTPUT_DIRECTORY}/${target})
endfunction(cxx_test)

########################################################################################################################

# include dependencies
find_library(PLAJA_LIB PlaJAHelper PATH ${CMAKE_CURRENT_BINARY_DIR}/../)

set(INCLUDES_TESTS ${INCLUDES})

set(LIBS_TESTS
    ${PLAJA_LIB}
    ${LIBS}
)

# auxiliary structures
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/auxiliary_structures/)

# non prob search
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/non_prob_search/)

# successor generation
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/successor_generation/)

# predicate abstraction
if (${BUILD_PA})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/predicate_abstraction/)
endif (${BUILD_PA})

# Extended Tests.

if (${BUILD_EXTENDED_TESTS})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/icaps22)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/icaps24)
endif (${BUILD_EXTENDED_TESTS})