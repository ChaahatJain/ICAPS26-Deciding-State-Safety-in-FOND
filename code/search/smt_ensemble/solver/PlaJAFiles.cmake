set (SMT_SOLVER_VERITAS_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/statistics_smt_solver_veritas.cpp ${CMAKE_CURRENT_LIST_DIR}/statistics_smt_solver_veritas.h
        ${CMAKE_CURRENT_LIST_DIR}/solution_check_wrapper_veritas.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_check_wrapper_veritas.h
        ${CMAKE_CURRENT_LIST_DIR}/solution_veritas.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_veritas.h
        ${CMAKE_CURRENT_LIST_DIR}/solver.cpp ${CMAKE_CURRENT_LIST_DIR}/solver.h
        ${CMAKE_CURRENT_LIST_DIR}/solver_veritas.cpp ${CMAKE_CURRENT_LIST_DIR}/solver_veritas.h
        ${CMAKE_CURRENT_LIST_DIR}/solver_z3.cpp ${CMAKE_CURRENT_LIST_DIR}/solver_z3.h
        )

if (${USE_GUROBI})
  set (SMT_SOLVER_GUROBI ${CMAKE_CURRENT_LIST_DIR}/solver_gurobi.cpp ${CMAKE_CURRENT_LIST_DIR}/solver_gurobi.h)
  list(APPEND SMT_SOLVER_VERITAS_SOURCES ${SMT_SOLVER_GUROBI})
endif (${USE_GUROBI})
