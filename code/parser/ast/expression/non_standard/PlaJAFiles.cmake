#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2021) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

set(EXPRESSION_NON_STANDARD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/comment_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/comment_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/constant_array_access_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/constant_array_access_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/let_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/let_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/location_value_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/location_value_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/variable_value_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/variable_value_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/state_values_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/state_values_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/states_values_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/states_values_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/state_condition_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/state_condition_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/objective_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/objective_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/predicates_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/predicates_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction_expression.h
    ${CMAKE_CURRENT_LIST_DIR}/problem_instance_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/problem_instance_expression.h
)