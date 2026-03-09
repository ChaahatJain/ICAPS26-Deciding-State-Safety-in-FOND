cxx_test(ppa_simple_test ${CMAKE_CURRENT_LIST_DIR}/ppa_simple_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(to_marabou_bound_predicate_test ${CMAKE_CURRENT_LIST_DIR}/to_marabou_bound_predicate_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(to_marabou_constraint_test ${CMAKE_CURRENT_LIST_DIR}/to_marabou_constraint_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(to_marabou_equation_predicate_test ${CMAKE_CURRENT_LIST_DIR}/to_marabou_equation_predicate_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(to_marabou_conjunction_predicate_test ${CMAKE_CURRENT_LIST_DIR}/to_marabou_conjunction_predicate_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(to_marabou_large_test ${CMAKE_CURRENT_LIST_DIR}/to_marabou_large_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(variable_selection_test ${CMAKE_CURRENT_LIST_DIR}/variable_selection_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")

if (${SUPPORT_RELAXED_NN_SAT})
    cxx_test(relaxed_nn_sat_checker_test ${CMAKE_CURRENT_LIST_DIR}/relaxed_nn_sat_checker_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
    cxx_test(relaxed_nn_sat_checker_with_unused_vars_test ${CMAKE_CURRENT_LIST_DIR}/relaxed_nn_sat_checker_with_unused_vars_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
endif (${SUPPORT_RELAXED_NN_SAT})

if (${MARABOU_IGNORE_ARRAY_LEGACY})
    cxx_test(ppa_racetrack_test ${CMAKE_CURRENT_LIST_DIR}/ppa_racetrack_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
    cxx_test(relaxed_ppa_racetrack_test ${CMAKE_CURRENT_LIST_DIR}/relaxed_ppa_racetrack_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
endif (${MARABOU_IGNORE_ARRAY_LEGACY})

# cxx_test(debugging_test.cpp ${CMAKE_CURRENT_LIST_DIR}/debugging_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
