set(FAULT_ANALYSIS_SOURCES)

# bdd_computation
if (${USE_CUDD})
    include(${CMAKE_CURRENT_LIST_DIR}/bdd_computation/PlaJAFiles.cmake)
    list(APPEND FAULT_ANALYSIS_SOURCES ${BDD_COMPUTATION_SOURCES})
endif (${USE_CUDD})

# policy localization
include (${CMAKE_CURRENT_LIST_DIR}/search_based/PlaJAFiles.cmake)
list(APPEND FAULT_ANALYSIS_SOURCES ${FA_SEARCH_BASED_SOURCES})