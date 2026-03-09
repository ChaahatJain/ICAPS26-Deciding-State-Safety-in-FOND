include(${CMAKE_CURRENT_LIST_DIR}/smt/PlaJAFiles.cmake)

# ensemble sat checker
include(${CMAKE_CURRENT_LIST_DIR}/ensemble_sat_checker/PlaJAFiles.cmake)

# ensemble sat in z3
include(${CMAKE_CURRENT_LIST_DIR}/ensemble_sat_in_z3/PlaJAFiles.cmake)


set(PA_ENSEMBLE_SOURCES
        ${SMT_PA_ENSEMBLE_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/search_statistics_ensemble_sat.cpp ${CMAKE_CURRENT_LIST_DIR}/search_statistics_ensemble_sat.h
        # ${CMAKE_CURRENT_LIST_DIR}/query_to_json.cpp ${CMAKE_CURRENT_LIST_DIR}/query_to_json.h
        ${PA_ENSEMBLE_SAT_CHECKER_SOURCES}
        ${PA_ENSEMBLE_SAT_IN_Z3_SOURCES}
        ${PA_ENSEMBLE_SAT_IN_GUROBI_SOURCES}
        )
