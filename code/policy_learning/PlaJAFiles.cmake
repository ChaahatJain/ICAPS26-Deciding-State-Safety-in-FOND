set(POLICY_LEARNING_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/policy_teacher.cpp ${CMAKE_CURRENT_LIST_DIR}/policy_teacher.h
    ${CMAKE_CURRENT_LIST_DIR}/policy_model.cpp ${CMAKE_CURRENT_LIST_DIR}/policy_model.h
    ${CMAKE_CURRENT_LIST_DIR}/explorer_teacher.cpp ${CMAKE_CURRENT_LIST_DIR}/explorer_teacher.h
    ${CMAKE_CURRENT_LIST_DIR}/learning_statistics.cpp ${CMAKE_CURRENT_LIST_DIR}/learning_statistics.h
    ${CMAKE_CURRENT_LIST_DIR}/reduction_statistics.cpp ${CMAKE_CURRENT_LIST_DIR}/reduction_statistics.h
    ${CMAKE_CURRENT_LIST_DIR}/neural_networks/neural_network.cpp ${CMAKE_CURRENT_LIST_DIR}/neural_networks/neural_network.h
    ${CMAKE_CURRENT_LIST_DIR}/replay_buffer.cpp ${CMAKE_CURRENT_LIST_DIR}/replay_buffer.h
    ${CMAKE_CURRENT_LIST_DIR}/ql_agent_torch.cpp ${CMAKE_CURRENT_LIST_DIR}/ql_agent_torch.h
    ${CMAKE_CURRENT_LIST_DIR}/ql_agent.cpp ${CMAKE_CURRENT_LIST_DIR}/ql_agent.h
    ${CMAKE_CURRENT_LIST_DIR}/reduction_agent.cpp ${CMAKE_CURRENT_LIST_DIR}/reduction_agent.h
)