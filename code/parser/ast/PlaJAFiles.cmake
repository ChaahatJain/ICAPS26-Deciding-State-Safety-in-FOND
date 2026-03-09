set(AST_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/ast_element.cpp ${CMAKE_CURRENT_LIST_DIR}/ast_element.h # AST ELEMENT (BASE)
        ${CMAKE_CURRENT_LIST_DIR}/commentable.cpp ${CMAKE_CURRENT_LIST_DIR}/commentable.h
        )

# TYPES
include(${CMAKE_CURRENT_LIST_DIR}/type/PlaJAFiles.cmake)
list(APPEND AST_SOURCES ${TYPE_SOURCES})

# EXPRESSIONS
include(${CMAKE_CURRENT_LIST_DIR}/expression/PlaJAFiles.cmake)
list(APPEND AST_SOURCES ${EXPRESSION_SOURCES})

list(APPEND AST_SOURCES
        # PROPERTY EXPRESSION SUBSTRUCTURES (moving after EXPRESSIONS and before PROPERTY_EXPRESSIONS may enhance linking performance)
        ${CMAKE_CURRENT_LIST_DIR}/property_interval.cpp ${CMAKE_CURRENT_LIST_DIR}/property_interval.h
        ${CMAKE_CURRENT_LIST_DIR}/reward_bound.cpp ${CMAKE_CURRENT_LIST_DIR}/reward_bound.h
        ${CMAKE_CURRENT_LIST_DIR}/reward_instant.cpp ${CMAKE_CURRENT_LIST_DIR}/reward_instant.h
        # AST
        ${CMAKE_CURRENT_LIST_DIR}/action.cpp ${CMAKE_CURRENT_LIST_DIR}/action.h
        ${CMAKE_CURRENT_LIST_DIR}/assignment.cpp ${CMAKE_CURRENT_LIST_DIR}/assignment.h
        ${CMAKE_CURRENT_LIST_DIR}/constant_declaration.cpp ${CMAKE_CURRENT_LIST_DIR}/constant_declaration.h
        ${CMAKE_CURRENT_LIST_DIR}/destination.cpp ${CMAKE_CURRENT_LIST_DIR}/destination.h
        ${CMAKE_CURRENT_LIST_DIR}/edge.cpp ${CMAKE_CURRENT_LIST_DIR}/edge.h
        ${CMAKE_CURRENT_LIST_DIR}/location.cpp ${CMAKE_CURRENT_LIST_DIR}/location.h
        ${CMAKE_CURRENT_LIST_DIR}/metadata.cpp ${CMAKE_CURRENT_LIST_DIR}/metadata.h
        ${CMAKE_CURRENT_LIST_DIR}/transient_value.cpp ${CMAKE_CURRENT_LIST_DIR}/transient_value.h
        ${CMAKE_CURRENT_LIST_DIR}/variable_declaration.cpp ${CMAKE_CURRENT_LIST_DIR}/variable_declaration.h
        ${CMAKE_CURRENT_LIST_DIR}/automaton.cpp ${CMAKE_CURRENT_LIST_DIR}/automaton.h
        ${CMAKE_CURRENT_LIST_DIR}/property.cpp ${CMAKE_CURRENT_LIST_DIR}/property.h
        ${CMAKE_CURRENT_LIST_DIR}/synchronisation.cpp ${CMAKE_CURRENT_LIST_DIR}/synchronisation.h
        ${CMAKE_CURRENT_LIST_DIR}/composition_element.cpp ${CMAKE_CURRENT_LIST_DIR}/composition_element.h
        ${CMAKE_CURRENT_LIST_DIR}/composition.cpp ${CMAKE_CURRENT_LIST_DIR}/composition.h
        ${CMAKE_CURRENT_LIST_DIR}/model.cpp ${CMAKE_CURRENT_LIST_DIR}/model.h
        )

# NON-STANDARD
include(${CMAKE_CURRENT_LIST_DIR}/non_standard/PlaJAFiles.cmake)
list(APPEND AST_SOURCES ${AST_NON_STANDARD_SOURCES})

# AST ITERATORS
include(${CMAKE_CURRENT_LIST_DIR}/iterators/PlaJAFiles.cmake)
list(APPEND AST_SOURCES ${AST_ITERATOR_SOURCES})