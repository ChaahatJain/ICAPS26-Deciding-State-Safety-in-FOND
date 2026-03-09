//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "lrtdp_factory.h"
#include "../../../option_parser/option_parser.h"
#include "../../factories/prob_search/prob_search_options.h"
#include "../../information/property_information.h"
#include "../../prob_search/lrtdp.h"
#include "../configuration.h"

LRTDP_Factory::LRTDP_Factory():
    ProbSearchFactory(SearchEngineType::LrtdpType) {}

LRTDP_Factory::~LRTDP_Factory() = default;

std::unique_ptr<SearchEngine> LRTDP_Factory::construct(const PLAJA::Configuration& config) const {
    const bool upper_and_lower_bound = PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::upper_and_lower_bound);
    const auto* property_info = config.get_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO);
    PLAJA_ASSERT(property_info)

    switch (property_info->_property_type()) {

        case PropertyInformation::MINCOST: {

            if (upper_and_lower_bound) {
                return (std::make_unique<LRTDP_DUAL<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualCMin, UpperAndLowerValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(config, true));
            } else {
                return std::make_unique<LRTDP<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpaceCMin, ValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(config, true);
            }

        }

        case PropertyInformation::PROBMIN: {

            if (upper_and_lower_bound) {
                return std::make_unique<LRTDP_DUAL<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMin, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(config, true);
            } else {
                return std::make_unique<LRTDP<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMin, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(config, true);
            }

        }

        case PropertyInformation::PROBMAX: {

            if (upper_and_lower_bound) {
                return std::make_unique<LRTDP_DUAL<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMax, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(config, false);
            } else {
                return std::make_unique<LRTDP<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMax, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(config, false);
            }

        }

        default: PLAJA_ABORT

    }
}

std::unique_ptr<SearchEngine> LRTDP_Factory::construct_LRTDP(const PLAJA::Configuration& configuration) { return LRTDP_Factory().construct(configuration); }

std::unique_ptr<SearchEngine> LRTDP_Factory::construct_LRTDP(const PLAJA::Configuration& config, SearchEngine* parent) {
    const bool upper_and_lower_bound = PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::upper_and_lower_bound);

    const auto* property_info = config.get_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO);
    PLAJA_ASSERT(property_info)

    switch (property_info->_property_type()) {

        case PropertyInformation::MINCOST: {

            if (upper_and_lower_bound) {

                auto* parent_cast = PLAJA_UTILS::cast_ptr<ProbSearchEngine<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualCMin, UpperAndLowerValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(parent);
                return (std::make_unique<LRTDP_DUAL<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualCMin, UpperAndLowerValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(config, true, parent_cast));

            } else {

                auto* parent_cast = PLAJA_UTILS::cast_ptr<ProbSearchEngine<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpaceCMin, ValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(parent);
                return std::make_unique<LRTDP<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpaceCMin, ValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(config, true, parent_cast);

            }

        }

        case PropertyInformation::PROBMIN: {

            if (upper_and_lower_bound) {

                auto* parent_cast = PLAJA_UTILS::cast_ptr<ProbSearchEngine<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMin, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(parent);
                return std::make_unique<LRTDP_DUAL<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMin, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(config, true, parent_cast);

            } else {

                auto* parent_cast = PLAJA_UTILS::cast_ptr<ProbSearchEngine<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMin, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(parent);
                return std::make_unique<LRTDP<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMin, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(config, true, parent_cast);

            }

        }

        case PropertyInformation::PROBMAX: {

            if (upper_and_lower_bound) {

                auto* parent_cast = PLAJA_UTILS::cast_ptr<ProbSearchEngine<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMax, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(parent);
                return std::make_unique<LRTDP_DUAL<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMax, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(config, false, parent_cast);

            } else {

                auto* parent_cast = PLAJA_UTILS::cast_ptr<ProbSearchEngine<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMax, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(parent);
                return std::make_unique<LRTDP<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMax, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(config, false, parent_cast);

            }

        }

        default: PLAJA_ABORT

    }

}

