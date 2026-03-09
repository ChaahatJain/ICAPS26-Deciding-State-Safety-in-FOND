//
// Created by Chaahat Jain on 21/01/25.
//

#ifndef PLAJA_STATS_STRING_H
#define PLAJA_STATS_STRING_H


#include "../include/ct_config_const.h"

namespace PLAJA {

    enum class StatsString {

#ifdef BUILD_FAULT_ANALYSIS
        START_STATE_CHECKED,
        FAULT_SOLVING_TIMES,
        STATES_FOR_ORACLE,
#endif
    };

}

#endif //PLAJA_STATS_STRING_H
