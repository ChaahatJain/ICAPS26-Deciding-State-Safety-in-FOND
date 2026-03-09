set(PREDICATE_ABSTRACTION_SOURCES)

# Statistics:
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/search_statistics_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/search_statistics_pa.h)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/predicate_dependency_graph.cpp ${CMAKE_CURRENT_LIST_DIR}/predicate_dependency_graph.h)

# Abstract states:
include(${CMAKE_CURRENT_LIST_DIR}/pa_states/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${PA_STATES_SOURCES})

# Transition structure:
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/pa_transition_structure.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_transition_structure.h)

# Solution checker:
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/solution_checker_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_checker_pa.h)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/solution_checker_instance_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_checker_instance_pa.h)

# Smt for pa:
include(${CMAKE_CURRENT_LIST_DIR}/smt/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${SMT_PA_SOURCES})

# Optimization:
include(${CMAKE_CURRENT_LIST_DIR}/optimization/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${OPTIMIZATION_PA_SOURCES})

# Successor generation:
include(${CMAKE_CURRENT_LIST_DIR}/successor_generation/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${SUCCESSOR_GENERATION_PA_SOURCES})

# Neural networks:
include(${CMAKE_CURRENT_LIST_DIR}/nn/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${PA_NN_SOURCES})

# sat checkers
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/sat_checker.cpp ${CMAKE_CURRENT_LIST_DIR}/sat_checker.h)

if (${USE_VERITAS})
# Ensembles:
include(${CMAKE_CURRENT_LIST_DIR}/ensemble/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${PA_ENSEMBLE_SOURCES})
endif (${USE_VERITAS})
# Heuristic:
include(${CMAKE_CURRENT_LIST_DIR}/heuristic/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${PA_HEURISTIC_SOURCES})

# match tree
include(${CMAKE_CURRENT_LIST_DIR}/match_tree/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${PA_MATCH_TREE_SOURCES})

# Inc state space:
include(${CMAKE_CURRENT_LIST_DIR}/inc_state_space/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${PA_INC_STATE_SPACE_SOURCES})

# Search space:
include(${CMAKE_CURRENT_LIST_DIR}/search_space/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${PA_SEARCH_SPACE_SOURCES})

list(APPEND PREDICATE_ABSTRACTION_SOURCES
    # main classes
    ${CMAKE_CURRENT_LIST_DIR}/base/expansion_pa_base.cpp ${CMAKE_CURRENT_LIST_DIR}/base/expansion_pa_base.h
    ${CMAKE_CURRENT_LIST_DIR}/base/search_pa_base.cpp ${CMAKE_CURRENT_LIST_DIR}/base/search_pa_base.h
    ${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction.cpp ${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction.h
    ${CMAKE_CURRENT_LIST_DIR}/search_pa_path.cpp ${CMAKE_CURRENT_LIST_DIR}/search_pa_path.h
    ${CMAKE_CURRENT_LIST_DIR}/problem_instance_checker_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/problem_instance_checker_pa.h
)

# cegar
include(${CMAKE_CURRENT_LIST_DIR}/cegar/PlaJAFiles.cmake)
list(APPEND PREDICATE_ABSTRACTION_SOURCES ${CEGAR_SOURCES})


