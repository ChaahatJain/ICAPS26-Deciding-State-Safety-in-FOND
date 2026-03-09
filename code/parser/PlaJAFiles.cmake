set(PARSER_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/jani_words.cpp ${CMAKE_CURRENT_LIST_DIR}/jani_words.h
        )

# VISITOR (BASE)
include(${CMAKE_CURRENT_LIST_DIR}/visitor/PlaJAFiles.cmake)
list(APPEND PARSER_SOURCES ${VISITOR_BASE_SOURCES})

# AST
include(${CMAKE_CURRENT_LIST_DIR}/ast/PlaJAFiles.cmake)
list(APPEND PARSER_SOURCES ${AST_SOURCES})

# VISITOR (DERIVED)
list(APPEND PARSER_SOURCES ${VISITOR_SOURCES})

# PARSER
list(APPEND PARSER_SOURCES ${CMAKE_CURRENT_LIST_DIR}/parser_utils.cpp ${CMAKE_CURRENT_LIST_DIR}/parser_utils.h)
list(APPEND PARSER_SOURCES ${CMAKE_CURRENT_LIST_DIR}/parse_types.cpp ${CMAKE_CURRENT_LIST_DIR}/parse_expressions.cpp ${CMAKE_CURRENT_LIST_DIR}/parse_structures.cpp
                           ${CMAKE_CURRENT_LIST_DIR}/parser.cpp ${CMAKE_CURRENT_LIST_DIR}/parser.h)

include(${CMAKE_CURRENT_LIST_DIR}/nn_parser/PlaJAFiles.cmake)
list(APPEND PARSER_SOURCES ${NN_PARSER_SOURCES})
