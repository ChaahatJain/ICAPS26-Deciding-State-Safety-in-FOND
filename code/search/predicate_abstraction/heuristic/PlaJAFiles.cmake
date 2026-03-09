set (PA_HEURISTIC_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/statistics_heuristic.cpp ${CMAKE_CURRENT_LIST_DIR}/statistics_heuristic.h
        ${CMAKE_CURRENT_LIST_DIR}/abstract_heuristic.cpp ${CMAKE_CURRENT_LIST_DIR}/abstract_heuristic.h
        ${CMAKE_CURRENT_LIST_DIR}/constant_heuristic.cpp ${CMAKE_CURRENT_LIST_DIR}/constant_heuristic.h
        ${CMAKE_CURRENT_LIST_DIR}/hamming_distance.cpp ${CMAKE_CURRENT_LIST_DIR}/hamming_distance.h
        # ${CMAKE_CURRENT_LIST_DIR}/search_space_pa_heuristic.cpp ${CMAKE_CURRENT_LIST_DIR}/search_space_pa_heuristic.h
        ${CMAKE_CURRENT_LIST_DIR}/pa_heuristic.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_heuristic.h
        ${CMAKE_CURRENT_LIST_DIR}/state_queue.cpp ${CMAKE_CURRENT_LIST_DIR}/state_queue.h
        ${CMAKE_CURRENT_LIST_DIR}/heuristic_state_queue.cpp ${CMAKE_CURRENT_LIST_DIR}/heuristic_state_queue.h
        )