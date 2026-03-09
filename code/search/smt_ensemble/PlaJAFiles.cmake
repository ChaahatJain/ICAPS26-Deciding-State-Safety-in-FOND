set(SMT_ENSEMBLE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/predicates/Equation.cpp ${CMAKE_CURRENT_LIST_DIR}/predicates/Equation.h
        ${CMAKE_CURRENT_LIST_DIR}/predicates/Tightening.h
        ${CMAKE_CURRENT_LIST_DIR}/predicates/Disjunct.cpp ${CMAKE_CURRENT_LIST_DIR}/predicates/Disjunct.h
        ${CMAKE_CURRENT_LIST_DIR}/vars_in_veritas.cpp ${CMAKE_CURRENT_LIST_DIR}/vars_in_veritas.h
        ${CMAKE_CURRENT_LIST_DIR}/veritas_context.cpp ${CMAKE_CURRENT_LIST_DIR}/veritas_context.h
        ${CMAKE_CURRENT_LIST_DIR}/veritas_query.cpp ${CMAKE_CURRENT_LIST_DIR}/veritas_query.h
        ${CMAKE_CURRENT_LIST_DIR}/constraints_in_veritas.cpp ${CMAKE_CURRENT_LIST_DIR}/constraints_in_veritas.h
        )

# utils
# include(${CMAKE_CURRENT_LIST_DIR}/utils/PlaJAFiles.cmake)
# list(APPEND SMT_ENSEMBLE_SOURCES ${VERITAS_UTILS_SOURCES})

# model
include(${CMAKE_CURRENT_LIST_DIR}/model/PlaJAFiles.cmake)
list(APPEND SMT_ENSEMBLE_SOURCES ${MODEL_VERITAS_SOURCES})

# visitor
include(${CMAKE_CURRENT_LIST_DIR}/visitor/PlaJAFiles.cmake)
list(APPEND SMT_ENSEMBLE_SOURCES ${VERITAS_VISITOR_SOURCES})

# successor generation
include(${CMAKE_CURRENT_LIST_DIR}/successor_generation/PlaJAFiles.cmake)
list(APPEND SMT_ENSEMBLE_SOURCES ${SMT_ENSEMBLE_SUCCESSOR_GENERATION_SOURCES})

# solver
include(${CMAKE_CURRENT_LIST_DIR}/solver/PlaJAFiles.cmake)
list(APPEND SMT_ENSEMBLE_SOURCES ${SMT_SOLVER_VERITAS_SOURCES})

if (${USE_Z3})
    include(${CMAKE_CURRENT_LIST_DIR}/to_z3/PlaJAFiles.cmake)
    list(APPEND SMT_ENSEMBLE_SOURCES ${VERITAS_TO_Z3_SOURCES})
endif (${USE_Z3})

if (${USE_GUROBI})
    include(${CMAKE_CURRENT_LIST_DIR}/to_gurobi/PlaJAFiles.cmake)
    list(APPEND SMT_ENSEMBLE_SOURCES ${VERITAS_TO_GUROBI_SOURCES})
endif (${USE_GUROBI})