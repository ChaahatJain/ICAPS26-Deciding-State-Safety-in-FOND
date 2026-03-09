#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2024) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

set(SOLUTION_ENUMERATOR_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_z3.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_z3.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_bs.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_bs.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_td.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_td.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_sampler.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_sampler.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_sampler_optimizer.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_sampler_optimizer.h

)