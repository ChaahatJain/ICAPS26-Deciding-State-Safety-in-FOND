//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "lrtdp.h"
#include "../../option_parser/option_parser.h"

// extern:

namespace PLAJA_OPTION {
    extern const std::string epsilon;
    extern const std::string delta;
    extern const std::string target_value;
}

LRTDP_Base::LRTDP_Base(bool v_current_is_lower):
    numTrials(0),
    vCurrentIsLower(v_current_is_lower),
    epsilon(PLAJA_GLOBAL::optionParser->get_option_value<QValue_type>(PLAJA_OPTION::epsilon, 0.000001)),
    delta(PLAJA_GLOBAL::optionParser->get_option_value<QValue_type>(PLAJA_OPTION::delta, 0.000001)),
    tvFlag(PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::target_value)),
    targetValue(PLAJA_GLOBAL::optionParser->get_option_value<QValue_type>(PLAJA_OPTION::target_value, -1)) {

}

LRTDP_Base::~LRTDP_Base() = default;

void LRTDP_Base::print_statistics() const { std::cout << "LRTDP-trials: " << numTrials << std::endl; }

void LRTDP_Base::stats_to_csv(std::ofstream& file) const { file << numTrials << PLAJA_UTILS::commaString; }

void LRTDP_Base::stat_names_to_csv(std::ofstream& file) { file << "numTrials,"; }
