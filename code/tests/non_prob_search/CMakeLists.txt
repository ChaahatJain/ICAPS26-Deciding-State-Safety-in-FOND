cxx_test(explorer_test ${CMAKE_CURRENT_LIST_DIR}/explorer_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")

if (${BUILD_NON_PROB_SEARCH})
cxx_test(nn_explorer_test ${CMAKE_CURRENT_LIST_DIR}/nn_explorer_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
endif (${BUILD_NON_PROB_SEARCH})
