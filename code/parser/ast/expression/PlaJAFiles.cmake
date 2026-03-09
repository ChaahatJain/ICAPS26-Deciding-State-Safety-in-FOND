list(APPEND EXPRESSION_SOURCES
            # EXPRESSIONS
            ${CMAKE_CURRENT_LIST_DIR}/property_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/property_expression.h # PROPERTY EXPRESSION (BASE)
            ${CMAKE_CURRENT_LIST_DIR}/expression.cpp ${CMAKE_CURRENT_LIST_DIR}/expression.h # EXPRESSION (BASE)

            ${CMAKE_CURRENT_LIST_DIR}/array_access_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/array_access_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/array_constructor_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/array_constructor_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/array_value_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/array_value_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/constant_value_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/constant_value_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/bool_value_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/bool_value_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/real_value_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/real_value_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/binary_op_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/binary_op_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/constant_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/constant_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/derivative_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/derivative_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/distribution_sampling_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/distribution_sampling_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/free_variable_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/free_variable_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/integer_value_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/integer_value_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/ite_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/ite_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/lvalue_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/lvalue_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/unary_op_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/unary_op_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/variable_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/variable_expression.h

            # PROPERTY EXPRESSIONS (DERIVED)
            ${CMAKE_CURRENT_LIST_DIR}/expectation_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/expectation_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/filter_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/filter_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/path_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/path_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/qfied_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/qfied_expression.h
            ${CMAKE_CURRENT_LIST_DIR}/state_predicate_expression.cpp ${CMAKE_CURRENT_LIST_DIR}/state_predicate_expression.h
            )

# NON-STANDARD
include(${CMAKE_CURRENT_LIST_DIR}/non_standard/PlaJAFiles.cmake)
list(APPEND AST_SOURCES ${EXPRESSION_NON_STANDARD_SOURCES})

# SPECIAL CASES
include(${CMAKE_CURRENT_LIST_DIR}/special_cases/PlaJAFiles.cmake)
list(APPEND AST_SOURCES ${EXPRESSION_SPECIAL_CASE_SOURCES})