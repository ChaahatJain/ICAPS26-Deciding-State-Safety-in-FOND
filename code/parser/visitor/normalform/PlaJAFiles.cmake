#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2024) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

set(VISITOR_TO_NORMALFORM_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/push_down_negation.cpp ${CMAKE_CURRENT_LIST_DIR}/push_down_negation.h
    ${CMAKE_CURRENT_LIST_DIR}/distribute_over.cpp ${CMAKE_CURRENT_LIST_DIR}/distribute_over.h
    ${CMAKE_CURRENT_LIST_DIR}/split_equality.cpp ${CMAKE_CURRENT_LIST_DIR}/split_equality.h
    ${CMAKE_CURRENT_LIST_DIR}/split_on_binary_op.cpp ${CMAKE_CURRENT_LIST_DIR}/split_on_binary_op.h
    ${CMAKE_CURRENT_LIST_DIR}/split_on_locs.cpp ${CMAKE_CURRENT_LIST_DIR}/split_on_locs.h
)