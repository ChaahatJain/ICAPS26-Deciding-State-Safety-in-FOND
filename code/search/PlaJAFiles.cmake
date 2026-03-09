set(SEARCH_SOURCES)

# information
include(${CMAKE_CURRENT_LIST_DIR}/information/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${INFORMATION_SOURCES})

# states
include(${CMAKE_CURRENT_LIST_DIR}/states/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${STATES_SOURCES})

# fd adaptions
include(${CMAKE_CURRENT_LIST_DIR}/fd_adaptions/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${FD_ADAPTIONS_SOURCES})

# further
list(APPEND SEARCH_SOURCES ${CMAKE_CURRENT_LIST_DIR}/prob_search/state_space.cpp ${CMAKE_CURRENT_LIST_DIR}/prob_search/state_space.h) # build independent of prob search engines

if (${USE_Z3})
    include(${CMAKE_CURRENT_LIST_DIR}/smt/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${SMT_SOURCES})
endif (${USE_Z3})

if (${USE_MARABOU})
    include(${CMAKE_CURRENT_LIST_DIR}/smt_nn/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${SMT_NN_SOURCES})
endif (${USE_MARABOU})

if (${USE_VERITAS})
    include(${CMAKE_CURRENT_LIST_DIR}/smt_ensemble/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${SMT_ENSEMBLE_SOURCES})
endif (${USE_VERITAS})

# successor generation
include(${CMAKE_CURRENT_LIST_DIR}/successor_generation/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${SUCCESSOR_GENERATION_SOURCES})

# non-probabilistic search
include(${CMAKE_CURRENT_LIST_DIR}/non_prob_search/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${NON_PROB_SEARCH_SOURCES})

# probabilistic search
if (${BUILD_PROB_SEARCH})
    include(${CMAKE_CURRENT_LIST_DIR}/prob_search/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${PROB_SEARCH_SOURCES})
endif (${BUILD_PROB_SEARCH})

# predicate abstraction
if (${BUILD_PA})
    include(${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${PREDICATE_ABSTRACTION_SOURCES})
endif (${BUILD_PA})

# BMC
if (${BUILD_BMC})
    include(${CMAKE_CURRENT_LIST_DIR}/bmc/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${BMC_SOURCES})
endif (${BUILD_BMC})

# Invariant strengthening
if (${BUILD_INVARIANT_STRENGTHENING})
    include(${CMAKE_CURRENT_LIST_DIR}/invariant_strengthening/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${INVARIANT_STRENGTHENING_SOURCES})
endif (${BUILD_INVARIANT_STRENGTHENING})

if (${BUILD_SAFE_START_GENERATOR})
    include(${CMAKE_CURRENT_LIST_DIR}/safe_start_generator/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${SAFE_START_GENERATOR_SOURCES})
endif (${BUILD_SAFE_START_GENERATOR})

# Fault Analysis
if (${BUILD_FAULT_ANALYSIS})
    include(${CMAKE_CURRENT_LIST_DIR}/fault_analysis/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${FAULT_ANALYSIS_SOURCES})
endif (${BUILD_FAULT_ANALYSIS})

# factories
include(${CMAKE_CURRENT_LIST_DIR}/factories/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${FACTORY_SOURCES})

