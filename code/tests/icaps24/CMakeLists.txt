#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2024) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

if (${BUILD_PA})
    cxx_test(icaps24_cegar_blocks_4_cost_test ${CMAKE_CURRENT_LIST_DIR}/cegar_blocks_4_cost_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
endif (${BUILD_PA})
