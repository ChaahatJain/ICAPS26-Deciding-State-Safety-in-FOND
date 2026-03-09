//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pa_heuristic.h"
#include "../../../option_parser/plaja_options.h"
#include "../../../parser/ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../factories/predicate_abstraction/search_engine_config_pa.h"
#include "../../fd_adaptions/timer.h"
#include "../../information/property_information.h"
#include "../smt/model_z3_pa.h"
#include "../predicate_abstraction.h"
#include "../search_pa_path.h"

namespace PLAJA_OPTION { extern const std::string nn_sat; }

/**********************************************************************************************************************/

PAHeuristic::PAHeuristic(const PLAJA::Configuration& config):
    parentModel(*config.get_sharable<ModelZ3PA>(PLAJA::SharableKey::MODEL_Z3))
    , configEngine(new SearchEngineConfigPA(*config.share_option_parser()))
    , maxNumPreds(config.get_option_value(PLAJA_OPTION::max_num_preds_heu, std::numeric_limits<std::size_t>::max()))
    , maxTime(config.get_option_value(PLAJA_OPTION::max_time_heu, std::numeric_limits<double>::max()))
    , maxTimeRel(config.get_option_value(PLAJA_OPTION::max_time_heu_rel, 1.0))
    , timeForPA(0) {
    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))

    configEngine->load_sharable_from(config); // stats heuristic, succ-gen-concrete, search-space-pa
    configEngine->disable_value_option(PLAJA_OPTION::heuristic_search);
    configEngine->set_value_option(PLAJA_OPTION::heuristic_search, PLAJA_OPTION_DEFAULTS::heuristic_search);
    configEngine->disable_value_option(PLAJA_OPTION::nn_sat);
    configEngine->disable_value_option(PLAJA_OPTION::stats_file); // disable stats
    configEngine->set_flag(PLAJA_OPTION::save_intermediate_stats, false);
    configEngine->set_flag(PLAJA_OPTION::cacheWitnesses, false); // crucial when sharing state space

    // local copy of predicates
    predicates = std::make_unique<PredicatesExpression>();
    predicates->reserve(parentModel.get_number_predicates());
    for (auto pred_it = parentModel._predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) { predicates->add_predicate(pred_it->deepCopy_Exp()); }

    propInfo = std::make_unique<PropertyInformation>(*configEngine, PropertyInformation::PA_PROPERTY, nullptr, &parentModel.get_model());
    propInfo->set_start(parentModel.get_start());
    propInfo->set_reach(parentModel.get_reach());
    propInfo->set_predicates(predicates.get());
    configEngine->set_sharable_const(PLAJA::SharableKey::PROP_INFO, propInfo.get());

    // predAb = std::make_unique<PredicateAbstraction>(*configEngine);
    predAb = std::make_unique<SearchPAPath>(*configEngine);
    // predAb->search();
}

PAHeuristic::~PAHeuristic() = default;

void PAHeuristic::increment() {
    if (predicates->get_number_predicates() >= maxNumPreds or timeForPA >= maxTime or timeForPA / PLAJA_GLOBAL::timer->operator()() >= maxTimeRel) { return; }
    // copy predicates from parent:
    for (PredicateIndex_type pred_index = predicates->get_number_predicates(); pred_index < parentModel.get_number_predicates(); ++pred_index) {
        predicates->add_predicate(parentModel.get_predicate(pred_index)->deepCopy_Exp());
    }
    PLAJA_ASSERT(predicates->get_number_predicates() == parentModel.get_number_predicates())
#if 0
    propInfo = std::make_unique<PropertyInformation>(PropertyInformation::PA_PROPERTY, nullptr, &parentModel.get_model());
    propInfo->set_start(parentModel.get_start_condition());
    propInfo->set_reach(parentModel.get_reach_condition());
    propInfo->set_predicates(predicates.get());
    configEngine->set_problem(propInfo.get());
    predAb = std::make_unique<PredicateAbstraction>(*configEngine); // model Z3 should have been updated by caller
    predAb->search();
#else
    predAb->increment();
#endif
}

HeuristicValue_type PAHeuristic::_evaluate(const AbstractState& state) {
    auto offset = PLAJA_GLOBAL::timer->operator()();
    auto rlt = predAb->get_goal_distance(state);
    timeForPA += PLAJA_GLOBAL::timer->operator()() - offset;
    return rlt;
}

/** stats *************************************************************************************************************/

void PAHeuristic::print_statistics() const { predAb->print_statistics_frame(false, "PA heuristic"); }

void PAHeuristic::stats_to_csv(std::ofstream& file) const { predAb->stats_to_csv(file); }

void PAHeuristic::stat_names_to_csv(std::ofstream& file) const { predAb->stat_names_to_csv(file); }