set(POLICY_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/policy.h
        ${CMAKE_CURRENT_LIST_DIR}/nn_policy.cpp ${CMAKE_CURRENT_LIST_DIR}/nn_policy.h
        ${CMAKE_CURRENT_LIST_DIR}/policy_restriction.cpp ${CMAKE_CURRENT_LIST_DIR}/policy_restriction.h
        )

if (${USE_VERITAS})
        list(APPEND POLICY_SOURCES ${CMAKE_CURRENT_LIST_DIR}/ensemble_policy.cpp ${CMAKE_CURRENT_LIST_DIR}/ensemble_policy.h)
endif (${USE_VERITAS})