set(EXCEPTION_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/exception_strings.cpp ${CMAKE_CURRENT_LIST_DIR}/exception_strings.h
    ${CMAKE_CURRENT_LIST_DIR}/plaja_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/plaja_exception.h
    ${CMAKE_CURRENT_LIST_DIR}/constructor_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/constructor_exception.h
    ${CMAKE_CURRENT_LIST_DIR}/not_implemented_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/not_implemented_exception.h
    ${CMAKE_CURRENT_LIST_DIR}/not_supported_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/not_supported_exception.h
    ${CMAKE_CURRENT_LIST_DIR}/option_parser_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/option_parser_exception.h
    ${CMAKE_CURRENT_LIST_DIR}/parser_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/parser_exception.h
    ${CMAKE_CURRENT_LIST_DIR}/property_analysis_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/property_analysis_exception.h
    ${CMAKE_CURRENT_LIST_DIR}/runtime_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/runtime_exception.h
    ${CMAKE_CURRENT_LIST_DIR}/smt_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/smt_exception.h
    ${CMAKE_CURRENT_LIST_DIR}/semantics_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/semantics_exception.h
)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    list(APPEND EXCEPTION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/assertion_exception.cpp ${CMAKE_CURRENT_LIST_DIR}/assertion_exception.h)
endif ()