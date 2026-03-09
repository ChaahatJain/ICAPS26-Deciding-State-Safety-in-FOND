#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2022) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

set(SMT_VISITOR_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/restrictions_smt_checker.cpp ${CMAKE_CURRENT_LIST_DIR}/restrictions_smt_checker.h
    ${CMAKE_CURRENT_LIST_DIR}/smt_based_restrictions_checker.cpp ${CMAKE_CURRENT_LIST_DIR}/smt_based_restrictions_checker.h
    ${CMAKE_CURRENT_LIST_DIR}/to_z3_visitor.cpp ${CMAKE_CURRENT_LIST_DIR}/to_z3_visitor.h
    ${CMAKE_CURRENT_LIST_DIR}/smt_vars_of.cpp ${CMAKE_CURRENT_LIST_DIR}/smt_vars_of.h
)