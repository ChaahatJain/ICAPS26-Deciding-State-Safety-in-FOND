set(PA_STATES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_base.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_base.h
    # ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_values.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_values.h
    # ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_composed.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_composed.h
    ${CMAKE_CURRENT_LIST_DIR}/pa_state_base.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_state_base.h
    ${CMAKE_CURRENT_LIST_DIR}/pa_state_values.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_state_values.h
    ${CMAKE_CURRENT_LIST_DIR}/abstract_state.cpp ${CMAKE_CURRENT_LIST_DIR}/abstract_state.h
    ${CMAKE_CURRENT_LIST_DIR}/abstract_path.cpp ${CMAKE_CURRENT_LIST_DIR}/abstract_path.h
    ${CMAKE_CURRENT_LIST_DIR}/predicate_state.cpp ${CMAKE_CURRENT_LIST_DIR}/predicate_state.h
    ${CMAKE_CURRENT_LIST_DIR}/state_registry_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/state_registry_pa.h
)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    list(APPEND PA_STATES_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_values.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_values.h
        ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_composed.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_entailment_composed.h
        )
endif ()
