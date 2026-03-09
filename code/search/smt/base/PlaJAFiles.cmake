#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2023) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

set(SMT_BASE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/smt_context.cpp ${CMAKE_CURRENT_LIST_DIR}/smt_context.h
    ${CMAKE_CURRENT_LIST_DIR}/smt_constraint.cpp ${CMAKE_CURRENT_LIST_DIR}/smt_constraint.h
    ${CMAKE_CURRENT_LIST_DIR}/smt_solver.cpp ${CMAKE_CURRENT_LIST_DIR}/smt_solver.h
    ${CMAKE_CURRENT_LIST_DIR}/model_smt.cpp ${CMAKE_CURRENT_LIST_DIR}/model_smt.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_checker.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_checker.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_check_wrapper.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_check_wrapper.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_checker_instance.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_checker_instance.h
)