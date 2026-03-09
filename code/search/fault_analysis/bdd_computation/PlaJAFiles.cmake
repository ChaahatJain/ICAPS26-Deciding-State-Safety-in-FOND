set(BDD_COMPUTATION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/bdd_context.cpp ${CMAKE_CURRENT_LIST_DIR}/bdd_context.h 
        ${CMAKE_CURRENT_LIST_DIR}/predicates/tightening.cpp ${CMAKE_CURRENT_LIST_DIR}/predicates/tightening.h
        ${CMAKE_CURRENT_LIST_DIR}/predicates/equation.cpp ${CMAKE_CURRENT_LIST_DIR}/predicates/equation.h
        ${CMAKE_CURRENT_LIST_DIR}/bdd_query.cpp ${CMAKE_CURRENT_LIST_DIR}/bdd_query.h 
        ${CMAKE_CURRENT_LIST_DIR}/unsafe_bdd.cpp ${CMAKE_CURRENT_LIST_DIR}/unsafe_bdd.h
        ${CMAKE_CURRENT_LIST_DIR}/reachable_bdd.cpp ${CMAKE_CURRENT_LIST_DIR}/reachable_bdd.h
)