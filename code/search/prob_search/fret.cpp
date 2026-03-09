//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "fret.h"
#include "../../option_parser/option_parser.h"

// extern:
namespace PLAJA_OPTION {
    extern const std::string epsilon;
    extern const std::string delta;
    extern const std::string local_trap_elimination;
    extern const std::string target_value;
}

FRET_Base::FRET_Base(const PLAJA::Configuration& /*config*/):
    numIters(0),
    epsilon(PLAJA_GLOBAL::optionParser->get_option_value<QValue_type>(PLAJA_OPTION::epsilon, 0.000001)),
    localTrapElimination(PLAJA_GLOBAL::optionParser->get_option_value<bool>(PLAJA_OPTION::local_trap_elimination, true)),
    delta(PLAJA_GLOBAL::optionParser->get_option_value<QValue_type>(PLAJA_OPTION::delta, 0.000001)),
    // values set for each step:
    onlyUnitSCCs(true),
    leafDiff(0),
    extendedSearchSpace(false),
    currentIndex(0) {
}


FRET_Base::~FRET_Base() = default;

void FRET_Base::print_statistics() const {
    std::cout << "FRET iterations: " << numIters << std::endl;
    subEngine->print_statistics();
}

void FRET_Base::stats_to_csv(std::ofstream& file) const {
    file << numIters << PLAJA_UTILS::commaString;
    subEngine->stats_to_csv(file);
}

void FRET_Base::stat_names_to_csv(std::ofstream& file) const {
    file << "numIters,";
    subEngine->stat_names_to_csv(file);
}

void FRET_Base::set_sub_engine_base(std::unique_ptr<ProbSearchEngineBase>&& sub_engine) {
    subEngine = std::move(sub_engine);
    subEngine->set_stats_file(nullptr);
    subEngine->set_intermediate_stats_file(nullptr);
}
