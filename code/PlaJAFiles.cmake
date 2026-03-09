set(PLAJA_SOURCES)

set(PLAJA_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/globals.cpp ${CMAKE_CURRENT_LIST_DIR}/globals.h
)

# example usage, might be useful later:
# list(REMOVE_ITEM PLAJA_SOURCES ${CMAKE_CURRENT_LIST_DIR}/globals.cpp)

include(${CMAKE_CURRENT_LIST_DIR}/utils/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${UTILS_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/stats/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${STATS_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/exception/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${EXCEPTION_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/option_parser/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${OPTION_PARSER_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/parser/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${PARSER_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/search/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${SEARCH_SOURCES})

if (${BUILD_POLICY_LEARNING})
    include(${CMAKE_CURRENT_LIST_DIR}/policy_learning/PlaJAFiles.cmake)
    list(APPEND PLAJA_SOURCES ${POLICY_LEARNING_SOURCES})
endif (${BUILD_POLICY_LEARNING})

if (${BUILD_TO_NUXMV})
    include(${CMAKE_CURRENT_LIST_DIR}/to_nuxmv/PlaJAFiles.cmake)
    list(APPEND PLAJA_SOURCES ${TO_NUXMV_SOURCES})
endif (${BUILD_TO_NUXMV})

if (${USE_ADVERSARIAL_ATTACK})
    include(${CMAKE_CURRENT_LIST_DIR}/adversarial_attack/PlaJAFiles.cmake)
    list(APPEND PLAJA_SOURCES ${ADVERSARIAL_ATTACK_SOURCES})
endif (${USE_ADVERSARIAL_ATTACK})


if (${CMAKE_BUILD_TYPE} STREQUAL "Debug") # domain specifics only for debugging!!!
    include(${CMAKE_CURRENT_LIST_DIR}/domain_specific/PlaJAFiles.cmake)
    list(APPEND PLAJA_SOURCES ${DOMAIN_SPECIFIC_SOURCES})

    include(${CMAKE_CURRENT_LIST_DIR}/tests/PlaJAFiles.cmake)
    list(APPEND PLAJA_SOURCES ${TESTS_SOURCES})
endif ()

# Information taken from a Fast Downward code base (http://gogs.cs.uni-saarland.de:3000/fai_group/FD-Stripped/src/master/src/search/DownwardFiles.cmake [October 2018]) and freely adapted to PLAJA:
# The order in PLAJA_SOURCES influences the order in which object files are given to the linker.
# This can have a significant influence on the performance (cf. issue67 FastDownward).
# According to FastDownward, the general recommendation is to list files that define functions after files that use them.
# We approximate this recommendation by reversing PLAJA_SOURCES.
# Thereby, in particular, the "core" implementation files in SEARCH_SOURCES will be listed first, followed by the "supporting" files in, e.g., PARSER_SOURCES.
# Also according to FastDownward and in accordance with our observations, this approximation works well enough in practice.
list(REVERSE PLAJA_SOURCES)

