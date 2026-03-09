//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "fret_factory.h"
#include "lrtdp_factory.h"
#include "../../../option_parser/option_parser.h"
#include "../../factories/prob_search/prob_search_options.h"
#include "../../information/property_information.h"
#include "../../prob_search/fret.h"
#include "../configuration.h"

FRET_Factory::FRET_Factory(SearchEngineFactory::SearchEngineType sub_engine_type):
    ProbSearchFactory(FretLrtdpType)
    , subEngineType(sub_engine_type) {
    PLAJA_ABORT_IF(subEngineType != SearchEngineFactory::LrtdpType) // Sub-engine argument should be properly used.
}

FRET_Factory::~FRET_Factory() = default;

std::unique_ptr<SearchEngine> FRET_Factory::construct(const PLAJA::Configuration& config) const {
    const bool upper_and_lower_bound = PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::upper_and_lower_bound);
    const auto* property_info = config.get_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO);
    PLAJA_ASSERT(property_info)
    switch (property_info->_property_type()) {
        case PropertyInformation::MINCOST: {

            if (upper_and_lower_bound) {

                std::unique_ptr<ProbSearchEngine<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualCMin, UpperAndLowerValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>> engine =
                    std::make_unique<FRET<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualCMin, UpperAndLowerValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(config);

                engine->set_subEngine(PLAJA_UTILS::cast_unique<ProbSearchEngine<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualCMin, UpperAndLowerValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(LRTDP_Factory::construct_LRTDP(config, engine.get())));

                return engine;

            } else {

                std::unique_ptr<ProbSearchEngine<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpaceCMin, ValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>> engine =
                    std::make_unique<FRET<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpaceCMin, ValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(config);

                engine->set_subEngine(PLAJA_UTILS::cast_unique<ProbSearchEngine<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpaceCMin, ValueInitializer<OptimizationCriterion::COST, InitializationCriterion::MIN>>>(LRTDP_Factory::construct_LRTDP(config, engine.get())));

                return engine;

            }

        }
        case PropertyInformation::PROBMIN: {

            if (upper_and_lower_bound ) {

                std::unique_ptr<ProbSearchEngine<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMin, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>> engine =
                    std::make_unique<FRET<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMin, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(config);

                engine->set_subEngine(PLAJA_UTILS::cast_unique<ProbSearchEngine<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMin, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(LRTDP_Factory::construct_LRTDP(config, engine.get())));

                return engine;

            } else {

                std::unique_ptr<ProbSearchEngine<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMin, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>> engine =
                    std::make_unique<FRET<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMin, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(config);

                engine->set_subEngine(PLAJA_UTILS::cast_unique<ProbSearchEngine<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMin, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MIN>>>(LRTDP_Factory::construct_LRTDP(config, engine.get())));

                return engine;

            }

        }
        case PropertyInformation::PROBMAX: {

            if (upper_and_lower_bound) {

                std::unique_ptr<ProbSearchEngine<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMax, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>> engine =
                    std::make_unique<FRET<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMax, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(config);

                engine->set_subEngine(PLAJA_UTILS::cast_unique<ProbSearchEngine<UpperAndLowerPNodeInfo, UpperAndLowerPNode, ProbSearchSpaceDualPMax, UpperAndLowerValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(LRTDP_Factory::construct_LRTDP(config, engine.get())));

                return engine;

            } else {

                std::unique_ptr<ProbSearchEngine<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMax, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>> engine =
                    std::make_unique<FRET<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMax, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(config);

                engine->set_subEngine(PLAJA_UTILS::cast_unique<ProbSearchEngine<ProbSearchNodeInfo, ProbSearchNode, ProbSearchSpacePMax, ValueInitializer<OptimizationCriterion::PROB, InitializationCriterion::MAX>>>(LRTDP_Factory::construct_LRTDP(config, engine.get())));

                return engine;

            }
        }

        default: PLAJA_ABORT

    }
}





