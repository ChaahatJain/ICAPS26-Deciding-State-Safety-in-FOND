#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2020) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

set(VISITOR_BASE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/ast_visitor.cpp ${CMAKE_CURRENT_LIST_DIR}/ast_visitor.h
    ${CMAKE_CURRENT_LIST_DIR}/ast_visitor_const.cpp ${CMAKE_CURRENT_LIST_DIR}/ast_visitor_const.h
    ${CMAKE_CURRENT_LIST_DIR}/ast_element_visitor.cpp ${CMAKE_CURRENT_LIST_DIR}/ast_element_visitor.h
    ${CMAKE_CURRENT_LIST_DIR}/ast_element_visitor_const.cpp ${CMAKE_CURRENT_LIST_DIR}/ast_element_visitor_const.h
)

set(VISITOR_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/ast_specialization.cpp ${CMAKE_CURRENT_LIST_DIR}/ast_specialization.h
    ${CMAKE_CURRENT_LIST_DIR}/ast_standardization.cpp ${CMAKE_CURRENT_LIST_DIR}/ast_standardization.h
    ${CMAKE_CURRENT_LIST_DIR}/ast_optimization.cpp ${CMAKE_CURRENT_LIST_DIR}/ast_optimization.h
    ${CMAKE_CURRENT_LIST_DIR}/is_constant.cpp ${CMAKE_CURRENT_LIST_DIR}/is_constant.h
    ${CMAKE_CURRENT_LIST_DIR}/linear_constraints_checker.cpp ${CMAKE_CURRENT_LIST_DIR}/linear_constraints_checker.h
    ${CMAKE_CURRENT_LIST_DIR}/restrictions_checker.cpp ${CMAKE_CURRENT_LIST_DIR}/restrictions_checker.h
    ${CMAKE_CURRENT_LIST_DIR}/retrieve_variable_bound.cpp ${CMAKE_CURRENT_LIST_DIR}/retrieve_variable_bound.h
    ${CMAKE_CURRENT_LIST_DIR}/semantics_checker.cpp ${CMAKE_CURRENT_LIST_DIR}/semantics_checker.h
    ${CMAKE_CURRENT_LIST_DIR}/set_constants.cpp ${CMAKE_CURRENT_LIST_DIR}/set_constants.h
    ${CMAKE_CURRENT_LIST_DIR}/set_variable_index.cpp ${CMAKE_CURRENT_LIST_DIR}/set_variable_index.h
    ${CMAKE_CURRENT_LIST_DIR}/simple_checks_visitor.cpp ${CMAKE_CURRENT_LIST_DIR}/simple_checks_visitor.h
    ${CMAKE_CURRENT_LIST_DIR}/to_normalform.cpp ${CMAKE_CURRENT_LIST_DIR}/to_normalform.h
    ${CMAKE_CURRENT_LIST_DIR}/variable_substitution.cpp ${CMAKE_CURRENT_LIST_DIR}/variable_substitution.h
    ${CMAKE_CURRENT_LIST_DIR}/variables_of.cpp ${CMAKE_CURRENT_LIST_DIR}/variables_of.h
    # special case
    ${CMAKE_CURRENT_LIST_DIR}/special_cases/to_linear_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/special_cases/to_linear_expression.h
)

# normalform:
include(${CMAKE_CURRENT_LIST_DIR}/normalform/PlaJAFiles.cmake)
list(APPEND VISITOR_SOURCES ${VISITOR_TO_NORMALFORM_SOURCES})

# to_str:
include(${CMAKE_CURRENT_LIST_DIR}/to_str/PlaJAFiles.cmake)
list(APPEND VISITOR_SOURCES ${VISITOR_TO_SRC_SOURCES})