#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2024) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

if (${MARABOU_IGNORE_ARRAY_LEGACY})

    if (${BUILD_BMC})
        cxx_test(icaps22_bmc_racetrack_test ${CMAKE_CURRENT_LIST_DIR}/bmc_racetrack_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
    endif (${BUILD_BMC})

    if (${BUILD_PA})
        cxx_test(icaps22_ppa_racetrack_test ${CMAKE_CURRENT_LIST_DIR}/ppa_racetrack_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
    endif (${BUILD_PA})

endif (${MARABOU_IGNORE_ARRAY_LEGACY})

if (${BUILD_PA})
    cxx_test(icaps22_ppa_blocks_6_cost_test ${CMAKE_CURRENT_LIST_DIR}/ppa_blocks_6_cost_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
    cxx_test(icaps22_ppa_blocks_8_no_cost_test ${CMAKE_CURRENT_LIST_DIR}/ppa_blocks_8_no_cost_test.cpp "${LIBS_TESTS}" "${INCLUDES_TESTS}")
endif (${BUILD_PA})