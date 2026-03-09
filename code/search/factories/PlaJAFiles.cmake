set(FACTORY_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/sharable_structure.cpp ${CMAKE_CURRENT_LIST_DIR}/sharable_structure.h
        ${CMAKE_CURRENT_LIST_DIR}/configuration.cpp ${CMAKE_CURRENT_LIST_DIR}/configuration.h
        ${CMAKE_CURRENT_LIST_DIR}/search_engine_factory.cpp ${CMAKE_CURRENT_LIST_DIR}/search_engine_factory.h
        )

include(${CMAKE_CURRENT_LIST_DIR}/non_prob_search/PlaJAFiles.cmake)
list(APPEND FACTORY_SOURCES ${NON_PROB_SEARCH_FACTORY_SOURCES})

if (${BUILD_PROB_SEARCH})
    include(${CMAKE_CURRENT_LIST_DIR}/prob_search/PlaJAFiles.cmake)
    list(APPEND FACTORY_SOURCES ${PROB_SEARCH_FACTORY_SOURCES})
endif (${BUILD_PROB_SEARCH})

if (${USE_Z3} OR ${USE_MARABOU})
include(${CMAKE_CURRENT_LIST_DIR}/smt_base/PlaJAFiles.cmake)
list(APPEND FACTORY_SOURCES ${SMT_FACTORY_SOURCES})
endif ()

if (${BUILD_PA})
    include(${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction/PlaJAFiles.cmake)
    list(APPEND FACTORY_SOURCES ${PA_FACTORY_SOURCES})
endif (${BUILD_PA})

if (${BUILD_BMC})
    include(${CMAKE_CURRENT_LIST_DIR}/bmc/PlaJAFiles.cmake)
    list(APPEND FACTORY_SOURCES ${BMC_FACTORY_SOURCES})
endif (${BUILD_BMC})

if (${BUILD_INVARIANT_STRENGTHENING})
    include(${CMAKE_CURRENT_LIST_DIR}/invariant_strengthening/PlaJAFiles.cmake)
    list(APPEND FACTORY_SOURCES ${INV_STR_FACTORY_SOURCES})
endif (${BUILD_INVARIANT_STRENGTHENING})

if (${BUILD_FAULT_ANALYSIS})
    include(${CMAKE_CURRENT_LIST_DIR}/fault_analysis/PlaJAFiles.cmake)
    list(APPEND FACTORY_SOURCES ${FAULT_ANALYSIS_FACTORY_SOURCES})
endif (${BUILD_FAULT_ANALYSIS})

if (${BUILD_POLICY_LEARNING})
    include(${CMAKE_CURRENT_LIST_DIR}/policy_learning/PlaJAFiles.cmake)
    list(APPEND FACTORY_SOURCES ${POLICY_LEARNING_FACTORY_SOURCES})
    include(${CMAKE_CURRENT_LIST_DIR}/policy_reduction/PlaJAFiles.cmake)
    list(APPEND FACTORY_SOURCES ${POLICY_REDUCTION_FACTORY_SOURCES})
endif (${BUILD_POLICY_LEARNING})

if (${BUILD_SAFE_START_GENERATOR})
    include(${CMAKE_CURRENT_LIST_DIR}/safe_start_generator/PlaJAFiles.cmake)
    list(APPEND FACTORY_SOURCES ${SAFE_START_GENERATOR_FACTORY_SOURCES})
endif (${BUILD_SAFE_START_GENERATOR})
