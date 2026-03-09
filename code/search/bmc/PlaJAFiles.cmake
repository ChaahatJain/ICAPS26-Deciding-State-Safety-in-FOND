#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2021) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

set(BMC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/bmc.cpp ${CMAKE_CURRENT_LIST_DIR}/bmc.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_checker_bmc.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_checker_bmc.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_checker_instance_bmc.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_checker_instance_bmc.h
)

if (${USE_Z3})
    list(APPEND BMC_SOURCES ${CMAKE_CURRENT_LIST_DIR}/bmc_z3.cpp ${CMAKE_CURRENT_LIST_DIR}/bmc_z3.h)
endif (${USE_Z3})

if (${USE_MARABOU})
    list(APPEND BMC_SOURCES ${CMAKE_CURRENT_LIST_DIR}/bmc_marabou.cpp ${CMAKE_CURRENT_LIST_DIR}/bmc_marabou.h)
endif (${USE_MARABOU})