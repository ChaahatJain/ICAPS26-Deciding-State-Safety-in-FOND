cxx_test(model_z3_test ${CMAKE_CURRENT_LIST_DIR}/model_z3_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(predicate_dependency_graph_test ${CMAKE_CURRENT_LIST_DIR}/predicate_dependency_graph_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(pa_cegar_test ${CMAKE_CURRENT_LIST_DIR}/pa_cegar_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(predicate_abstraction_test ${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(predicate_abstraction_test2 ${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction_test2.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
cxx_test(solution_enumerator_test ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")

# nn
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nn/)