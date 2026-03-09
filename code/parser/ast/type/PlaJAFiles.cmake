set(TYPE_SOURCES
    # TYPES
    ${CMAKE_CURRENT_LIST_DIR}/declaration_type.cpp ${CMAKE_CURRENT_LIST_DIR}/declaration_type.h
    ${CMAKE_CURRENT_LIST_DIR}/array_type.cpp ${CMAKE_CURRENT_LIST_DIR}/array_type.h
    ${CMAKE_CURRENT_LIST_DIR}/bounded_type.cpp ${CMAKE_CURRENT_LIST_DIR}/bounded_type.h
    ${CMAKE_CURRENT_LIST_DIR}/continuous_type.cpp ${CMAKE_CURRENT_LIST_DIR}/continuous_type.h
    ${CMAKE_CURRENT_LIST_DIR}/clock_type.cpp ${CMAKE_CURRENT_LIST_DIR}/clock_type.h
    ${CMAKE_CURRENT_LIST_DIR}/basic_type.cpp ${CMAKE_CURRENT_LIST_DIR}/basic_type.h
    ${CMAKE_CURRENT_LIST_DIR}/bool_type.cpp ${CMAKE_CURRENT_LIST_DIR}/bool_type.h
    ${CMAKE_CURRENT_LIST_DIR}/int_type.cpp ${CMAKE_CURRENT_LIST_DIR}/int_type.h
    ${CMAKE_CURRENT_LIST_DIR}/real_type.cpp ${CMAKE_CURRENT_LIST_DIR}/real_type.h
    # non-standard
    ${CMAKE_CURRENT_LIST_DIR}/non_standard/location_type.cpp ${CMAKE_CURRENT_LIST_DIR}/non_standard/location_type.h
)