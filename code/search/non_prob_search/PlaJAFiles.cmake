set(NON_PROB_SEARCH_SOURCES)

# base
include(${CMAKE_CURRENT_LIST_DIR}/base/PlaJAFiles.cmake)
list(APPEND NON_PROB_SEARCH_SOURCES ${SEARCH_BASE_SOURCES})

# policy
include(${CMAKE_CURRENT_LIST_DIR}/policy/PlaJAFiles.cmake)
list(APPEND NON_PROB_SEARCH_SOURCES ${POLICY_SOURCES})

list(APPEND NON_PROB_SEARCH_SOURCES
    # Added here on purpose:
    ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/solution_enumerator_base.cpp ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/solution_enumerator_base.h
    ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/solution_enumerator_naive.cpp ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/solution_enumerator_naive.h
    ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/state_values_enumerator.cpp ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/state_values_enumerator.h
    #
    ${CMAKE_CURRENT_LIST_DIR}/restrictions_checker_explicit.cpp ${CMAKE_CURRENT_LIST_DIR}/restrictions_checker_explicit.h
    ${CMAKE_CURRENT_LIST_DIR}/initial_states_enumerator.cpp ${CMAKE_CURRENT_LIST_DIR}/initial_states_enumerator.h
    ${CMAKE_CURRENT_LIST_DIR}/space_explorer.cpp ${CMAKE_CURRENT_LIST_DIR}/space_explorer.h
)

if (${BUILD_NON_PROB_SEARCH})
    list(APPEND NON_PROB_SEARCH_SOURCES ${CMAKE_CURRENT_LIST_DIR}/breadth_first_search.cpp ${CMAKE_CURRENT_LIST_DIR}/breadth_first_search.h)
    list(APPEND NON_PROB_SEARCH_SOURCES ${CMAKE_CURRENT_LIST_DIR}/nn_explorer.cpp ${CMAKE_CURRENT_LIST_DIR}/nn_explorer.h)
endif (${BUILD_NON_PROB_SEARCH})
