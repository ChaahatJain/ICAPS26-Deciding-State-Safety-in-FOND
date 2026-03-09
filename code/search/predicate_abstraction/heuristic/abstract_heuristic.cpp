//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "abstract_heuristic.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/option_parser.h"
#include "../../factories/configuration.h"
#include "../../fd_adaptions/timer.h"
#include "../smt/model_z3_pa.h"
#include "heuristic_state_queue.h"
#include "constant_heuristic.h"
#include "hamming_distance.h"
#include "pa_heuristic.h"

namespace PLAJA_OPTION_DEFAULTS {

    const std::string heuristic_search("hamming"); // NOLINT(cert-err58-cpp)

}

namespace PLAJA_OPTION {

    const std::string heuristic_search("heuristic-search"); // NOLINT(cert-err58-cpp)
    const std::string randomize_tie_breaking("randomize-tie-breaking"); // NOLINT(cert-err58-cpp)
    const std::string max_num_preds_heu("max-num-preds-heu"); // NOLINT(cert-err58-cpp)
    const std::string max_time_heu("max-time-heu"); // NOLINT(cert-err58-cpp)
    const std::string max_time_heu_rel("max-time-heu-rel"); // NOLINT(cert-err58-cpp)

    namespace ABSTRACT_HEURISTIC {

        const std::unordered_map<std::string, AbstractHeuristic::HeuristicType> stringToHeuristic { // NOLINT(cert-err58-cpp)
            { "none",     AbstractHeuristic::HeuristicType::NONE },
            { "constant", AbstractHeuristic::HeuristicType::CONSTANT },
            { "hamming",  AbstractHeuristic::HeuristicType::HAMMING },
            { "PA",       AbstractHeuristic::HeuristicType::PA }
        };

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::heuristic_search, PLAJA_OPTION_DEFAULTS::heuristic_search);
            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::randomize_tie_breaking);
            OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::max_num_preds_heu);
            OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::max_time_heu);
            OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::max_time_heu_rel);
        }

        void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::heuristic_search, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULTS::heuristic_search, "Perform PA search greedy with respect to a heuristic.", true);
            OPTION_PARSER::print_additional_specification(PLAJA::OptionParser::print_supported_options(stringToHeuristic));
            OPTION_PARSER::print_flag(PLAJA_OPTION::randomize_tie_breaking, "Randomize tie-breaking for abstract states with same heuristic value.");
            OPTION_PARSER::print_option(PLAJA_OPTION::max_num_preds_heu, "Maximal number of predicates used for the PA heuristic.", OPTION_PARSER::valueStr);
            OPTION_PARSER::print_option(PLAJA_OPTION::max_time_heu, "Maximal time spend computing the PA heuristic.", OPTION_PARSER::valueStr);
            OPTION_PARSER::print_option(PLAJA_OPTION::max_time_heu_rel, "Maximal time spend computing the PA heuristic relative to total search time.", OPTION_PARSER::valueStr);
        }

    }
}

/**********************************************************************************************************************/

AbstractHeuristic::AbstractHeuristic():
    statsHeuristic(nullptr) {}

AbstractHeuristic::~AbstractHeuristic() = default;

//

HeuristicValue_type AbstractHeuristic::_evaluate(const AbstractState& /*state*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

HeuristicValue_type AbstractHeuristic::evaluate(const AbstractState& state) {
    if (statsHeuristic) { statsHeuristic->inc_attr_unsigned(PLAJA::StatsUnsigned::HEURISTIC_CALLS); }
    PUSH_LAP_IF(statsHeuristic, PLAJA::StatsDouble::HEURISTIC_COMPUTATION_TIME)
    auto rlt = _evaluate(state);
    POP_LAP_IF(statsHeuristic, PLAJA::StatsDouble::HEURISTIC_COMPUTATION_TIME)
    return rlt;
}

HeuristicValue_type AbstractHeuristic::evaluate(const AbstractState& state) const {
    if (statsHeuristic) { statsHeuristic->inc_attr_unsigned(PLAJA::StatsUnsigned::HEURISTIC_CALLS); }
    PUSH_LAP_IF(statsHeuristic, PLAJA::StatsDouble::HEURISTIC_COMPUTATION_TIME)
    auto rlt = _evaluate(state);
    POP_LAP_IF(statsHeuristic, PLAJA::StatsDouble::HEURISTIC_COMPUTATION_TIME)
    return rlt;
}

std::unique_ptr<ConstHeuristicStateQueue> AbstractHeuristic::make_const_heuristic_queue(const PLAJA::Configuration& config) { return std::make_unique<ConstHeuristicStateQueue>(config, this); }

std::unique_ptr<StateQueue> AbstractHeuristic::make_state_queue(const PLAJA::Configuration& config) {
    std::unique_ptr<AbstractHeuristic> heuristic;
    auto heuristic_type = config.get_option(PLAJA_OPTION::ABSTRACT_HEURISTIC::stringToHeuristic, PLAJA_OPTION::heuristic_search);
    switch (heuristic_type) {
        case HeuristicType::NONE: { return std::make_unique<FIFOStateQueue>(); }
        case HeuristicType::CONSTANT: {
            heuristic = std::make_unique<ConstantHeuristic>();
            break;
        }
        case HeuristicType::HAMMING: {
            PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
            heuristic = std::make_unique<HammingDistance>(config, *config.get_sharable_cast<ModelZ3, ModelZ3PA>(PLAJA::SharableKey::MODEL_Z3));
            break;
        }
        case HeuristicType::PA: {
            heuristic = std::make_unique<PAHeuristic>(config);
            break;
        }
        default: PLAJA_ABORT
    }
    //
    heuristic->set_stats_heuristic(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS_HEURISTIC)); // set stats
    //
    return std::make_unique<HeuristicStateQueue>(config, std::move(heuristic));
}
