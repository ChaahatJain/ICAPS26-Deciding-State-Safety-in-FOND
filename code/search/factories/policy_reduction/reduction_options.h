#ifndef PLAJA_REDUCTION_OPTIONS_H
#define PLAJA_REDUCTION_OPTIONS_H

#include <string>

namespace PLAJA_OPTION {

    extern const std::string frequency;
    extern const std::string compact;
    extern const std::string reduction_threshold;
    extern const std::string num_start_states;

}

namespace PLAJA_OPTION_DEFAULT {

    constexpr double frequency = 1.0;
    constexpr double reduction_threshold = 0;
    constexpr int num_start_states = 10000;

}

#endif//PLAJA_REDUCTION_OPTIONS_H
