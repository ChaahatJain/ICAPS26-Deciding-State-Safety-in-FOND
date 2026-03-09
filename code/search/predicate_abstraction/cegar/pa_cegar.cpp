//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pa_cegar.h"
#include "../../../exception/runtime_exception.h"
#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/plaja_options.h"
#include "../../../parser/ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../../parser/ast/iterators/model_iterator.h"
#include "../../../parser/ast/non_standard/properties.h"
#include "../../../parser/ast/automaton.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/ast/property.h"
#include "../../../utils/structs_utils/update_op_ref.h"
#include "../../factories/predicate_abstraction/search_engine_config_pa.h"
#include "../../fd_adaptions/timer.h"
#include "../../information/property_information.h"
#include "../../smt/visitor/extern/smt_based_restrictions_check.h"
#include "../pa_states/abstract_path.h"
#include "../search_pa_path.h"
#include "predicates_structure.h"
#include "search_statistics_cegar.h"
#include "spuriousness_checker.h"
#include "spuriousness_result.h"
#include "../../states/state_base.h"

namespace MODEL_Z3_PA { extern void make_sharable(const PLAJA::Configuration& config); }
namespace MODEL_MARABOU_PA { extern void make_sharable(const PLAJA::Configuration& config); }
namespace MODEL_VERITAS_PA { extern void make_sharable(const PLAJA::Configuration& config); }
/**********************************************************************************************************************/

/**********************************************************************************************************************/

PAExpression* PACegar::get_pa_expression() {
    return PLAJA_UTILS::cast_ptr<PAExpression>(paProperty->get_propertyExpression());
}

PACegar::PACegar(const SearchEngineConfigPA& config):
    SearchEngine(config)
    , cachedProperties(new Properties())
    , paProperty(new Property())
    , maxNumPreds(config.get_int_option(PLAJA_OPTION::max_num_preds)) {
    PA_CEGAR::add_stats(*searchStatistics);

    // set PA property
    paProperty->set_name(propertyInfo->get_property_name());
    paProperty->set_propertyExpression(std::make_unique<PAExpression>());
    get_pa_expression()->set_predicates(std::make_unique<PredicatesExpression>());

    // start
    if (propertyInfo->get_start()) { get_pa_expression()->set_start(propertyInfo->get_start()->deepCopy_Exp()); }

    // reach
    PLAJA_ASSERT(propertyInfo->get_reach()) // there is really no point in cegar without some kind of goal expression; this is enforced by the factory infrastructure
    // interface file
    if (propertyInfo->get_interface()) { get_pa_expression()->set_nnFile(*propertyInfo->get_policy_file()); }
    if (propertyInfo->get_reach()) { get_pa_expression()->set_reach(propertyInfo->get_reach()->deepCopy_Exp()); }

    // after setting PA expression, set property info for sub-engines ...
    propInfoSub = std::make_unique<PropertyInformation>(config, PropertyInformation::PA_PROPERTY, paProperty.get(), model);
    propInfoSub->set_start(get_pa_expression()->get_start());
    propInfoSub->set_reach(get_pa_expression()->get_reach());
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { propInfoSub->set_terminal(propertyInfo->get_terminal()); }
    propInfoSub->set_predicates(get_pa_expression()->get_predicates());

    PLAJA_ASSERT_EXPECT(config.size_sharable() == 1)
    PLAJA_ASSERT_EXPECT(config.size_sharable_ptr() == 1) // stats
    PLAJA_ASSERT_EXPECT(config.size_sharable_const() == 1) // prop info
    PLAJA::Configuration config_main(*config.share_option_parser());
    config_main.set_sharable_const(PLAJA::SharableKey::MODEL, model);
    config_main.set_sharable_const(PLAJA::SharableKey::PROP_INFO, propInfoSub.get());
    config_main.set_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS, searchStatistics.get());
    MODEL_Z3_PA::make_sharable(config_main);
    if (propertyInfo->has_nn_interface()) { MODEL_MARABOU_PA::make_sharable(config_main); }
    #ifdef USE_VERITAS
    if (propertyInfo->has_ensemble_interface()) { MODEL_VERITAS_PA::make_sharable(config_main); }
    #endif
    // TODO: For now just perform SMT-based checks here. After all, only CEGAR is dependent on the implemented assumptions.
    SMT_BASED_RESTRICTIONS_CHECKER::check(*model, config_main);

    predicates = std::make_unique<PredicatesStructure>(config_main, *get_pa_expression()->get_predicates());

    // construct spuriousness checker
    spuriousnessChecker = std::make_unique<SpuriousnessChecker>(config_main, *predicates);

    // add predicates ...

    auto reach_constraint = StateConditionExpression::transform(get_pa_expression()->get_reach()->deepCopy_Exp())->set_constraint();
    PLAJA_LOG_DEBUG_IF(reach_constraint, "CEGAR always adds reach condition to predicates.")
    if (reach_constraint) { predicates->add_predicate(std::move(reach_constraint)); }

    // predicates
    if (propertyInfo->get_predicates()) {
        predicates->reserve(propertyInfo->get_predicates()->get_number_predicates());
        for (auto it_pred = propertyInfo->get_predicates()->predicatesIterator(); !it_pred.end(); ++it_pred) { predicates->add_predicate(it_pred->deepCopy_Exp()); }
    }

    /* Add objective as predicates? */
    if (PLAJA_GLOBAL::enableTerminalStateSupport and config.get_bool_option(PLAJA_OPTION::terminal_as_preds) and propertyInfo->get_terminal()) {
        const auto* terminal = propertyInfo->get_terminal();
        PLAJA_ASSERT(terminal)
        auto terminal_constraint = StateConditionExpression::transform(terminal->deepCopy_Exp())->set_constraint();
        PLAJA_LOG_IF(terminal_constraint, "Cegar is adding termination condition to predicates.")
        if (terminal_constraint) { predicates->add_predicate(std::move(terminal_constraint)); }
    }

    /* Add all guards as predicates? */
    if (config.get_bool_option(PLAJA_OPTION::guards_as_preds)) {
        PLAJA_LOG("Cegar is adding transition guards to predicates.")
        for (auto it_ai = ModelIterator::automatonInstanceIterator(*model); !it_ai.end(); ++it_ai) {
            for (auto it_edge = it_ai->edgeIterator(); !it_edge.end(); ++it_edge) {
                predicates->add_predicate(it_edge->get_guardExpression()->deepCopy_Exp());
            }
        }
    }

    // construct sub-engine
    auto time_offset = (*PLAJA_GLOBAL::timer)();
    auto sub_config(config);
    sub_config.delete_sharable(PLAJA::SharableKey::STATS);
    sub_config.delete_sharable_ptr(PLAJA::SharableKey::STATS);
    sub_config.set_sharable_const(PLAJA::SharableKey::PROP_INFO, propInfoSub.get());
    sub_config.disable_value_option(PLAJA_OPTION::stats_file);
    sub_config.set_flag(PLAJA_OPTION::save_intermediate_stats, false);
    sub_config.set_flag(PLAJA_OPTION::cacheWitnesses, true); // Enable witness caching.
    sub_config.load_sharable(PLAJA::SharableKey::MODEL_Z3, config_main);
    if (propertyInfo->has_nn_interface()) { sub_config.load_sharable(PLAJA::SharableKey::MODEL_MARABOU, config_main); }
    #ifdef USE_VERITAS
    if (propertyInfo->has_ensemble_interface()) {sub_config.load_sharable(PLAJA::SharableKey::MODEL_VERITAS, config_main); }
    #endif
    sub_config.load_sharable(PLAJA::SharableKey::SUCC_GEN, config_main);
    sub_config.load_sharable(PLAJA::SharableKey::PREDICATE_RELATIONS, config_main);
    if constexpr (PLAJA_GLOBAL::lazyPA) { sub_config.load_sharable(PLAJA::SharableKey::PA_MT, config_main); }
    // #ifdef USE_VERITAS
    cegarSearch = std::make_unique<SearchPAPath>(
        sub_config
#ifdef USE_VERITAS
        ,spuriousnessChecker->needs_interval()
#endif
        );
    cegarSearch->set_engine_for_stats(this);
    cegarSearch->set_construction_time((*PLAJA_GLOBAL::timer)() - time_offset);

    // cegar stats:
    searchStatistics->add_int_stats(PLAJA::StatsInt::SPURIOUS_PREFIX_LENGTH, "SpuriousPrefixLength", -1);

}

PACegar::~PACegar() = default;

inline void PACegar::cache_pa_property() {
    if (cachedProperties->get_number_properties() == 0 || PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::save_intermediate_predicates)) {
        cachedProperties->add_property(*paProperty);
    } else { cachedProperties->set_property(paProperty->deepCopy(), 0); }
    predicates_to_file(); // save intermediate version
}

void PACegar::predicates_to_file() const {
    if (not PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::predicates_file)) { return; }
    if (cachedProperties->get_number_properties() == 0) { PLAJA_LOG("Warning: no predicate abstraction generated ..."); }
    //
    try {
        cachedProperties->write_to_file(PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::predicates_file));
        if (PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::local_backup)) { // local backup
            cachedProperties->write_to_file(PLAJA_UTILS::extract_filename(PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::predicates_file), true));
        }
    } catch (PlajaException& e) { PLAJA_LOG(e.what()); }
}

SearchEngine::SearchStatus PACegar::initialize() {
    PLAJA_LOG("Initializing PA CEGAR for predicate abstraction ... ")
    this->share_timers(cegarSearch.get()); // share timers (only during initialization since during construction not set)
    return SearchEngine::IN_PROGRESS;
}

SearchEngine::SearchStatus PACegar::step() {
    PLAJA_LOG("\nNew PA CEGAR iteration ...");
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ITERATIONS);

    if (searchStatistics->get_attr_unsigned(PLAJA::StatsUnsigned::ITERATIONS) > 1) {
        PUSH_LAP(searchStatistics.get(), PLAJA::StatsDouble::INCREMENTATION_TIME)
        cegarSearch->increment();
        POP_LAP(searchStatistics.get(), PLAJA::StatsDouble::INCREMENTATION_TIME)
    }

    paProperty->set_name(propertyInfo->get_property_name() + PLAJA_UTILS::underscoreString + std::to_string(searchStatistics->get_attr_unsigned(PLAJA::StatsUnsigned::ITERATIONS)));
    cache_pa_property(); // cache predicate abstraction property, i.e., the predicate set, for output

    bool cegar_finished = false;

    if (predicates->size() > maxNumPreds) {
        PLAJA_LOG("Maximal number of predicates exceeded")
        return SearchStatus::SOLVED;
    }

    // search


    searchStatistics->set_attr_int(PLAJA::StatsInt::SPURIOUS_PREFIX_LENGTH, -1);

    PUSH_LAP(searchStatistics.get(), PLAJA::StatsDouble::TIME_FOR_PA)
    cegarSearch->search();
    POP_LAP(searchStatistics.get(), PLAJA::StatsDouble::TIME_FOR_PA)

    if (cegarSearch->get_status() == SearchStatus::FINISHED) {

        // Extract path
        if (cegarSearch->has_goal_path()) {

            auto path = cegarSearch->extract_goal_path();
            PLAJA_LOG(PLAJA_UTILS::string_f("Abstract path length: %i", path.get_size()));
            auto spuriousness_result = spuriousnessChecker->check_path(path);

            if (spuriousness_result->is_spurious()) {

                PLAJA_LOG(PLAJA_UTILS::string_f("Spurious prefix length: %i", spuriousness_result->get_spurious_prefix_length()));
                const bool progress = spuriousness_result->refine(*predicates);

                if (not progress) { throw RuntimeException("Spurious path, but no new predicates."); }
                else { cegar_finished = false; }

            } else {

                searchStatistics->set_attr_int(PLAJA::StatsInt::HAS_GOAL_PATH, 1);
                cegar_finished = true;
                is_safe = false;
                PLAJA_LOG("Unsafe policy");
                if (PLAJA_GLOBAL::has_value_option(PLAJA_OPTION::savePath)) { spuriousness_result->dump_concretization(PLAJA_GLOBAL::get_value_option_string(PLAJA_OPTION::savePath)); }
                unsafe_path = spuriousness_result->get_path_instance().get_concrete_unsafe_path();
            }

            spuriousness_result->update_stats(*searchStatistics);
            current_spuriousness_result = std::move(spuriousness_result);

        } else { // solved condition to properly handle (internal) timeout
            searchStatistics->set_attr_int(PLAJA::StatsInt::HAS_GOAL_PATH, 0);
            cegar_finished = true;
            is_safe = true;
            PLAJA_LOG("Safe policy");
        }

        if (not cegar_finished) {
            // intermediate stats
            trigger_intermediate_stats();

            /* PUSH_LAP(searchStatistics.get(), PLAJA::StatsDouble::INCREMENTATION_TIME)
            cegarSearch->increment();
            POP_LAP(searchStatistics.get(), PLAJA::StatsDouble::INCREMENTATION_TIME) */
        }
    }

    if (cegar_finished) {
        PLAJA_LOG("\n... PA CEGAR finished.");
        return SearchStatus::SOLVED;
    }

    PLAJA_LOG("... PA CEGAR iteration finished.");

    return SearchStatus::IN_PROGRESS;
}

std::unordered_set<std::unique_ptr<StateBase>> PACegar::extract_concrete_unsafe_path() {
    auto unsafe_path = current_spuriousness_result->get_path_instance().get_states_along_path();
    current_spuriousness_result = nullptr;
    return unsafe_path;
}

std::vector<StateValues> PACegar::extract_ordered_concrete_unsafe_path() {
    auto unsafe_path = current_spuriousness_result->get_path_instance().get_concrete_unsafe_path();
    current_spuriousness_result = nullptr;
    return unsafe_path;
}

// statistics:

void PACegar::update_statistics() const { cegarSearch->update_statistics(); }

void PACegar::print_statistics() const {
    searchStatistics->print_statistics();
    cegarSearch->print_statistics_frame(false, "PA SEARCH");
    // hack to force output in case of timeout
    if (this->get_status() == SearchStatus::TIMEOUT) { predicates_to_file(); }
}

void PACegar::stats_to_csv(std::ofstream& file) const {
    searchStatistics->stats_to_csv(file);
    cegarSearch->stats_to_csv(file);
}

void PACegar::stat_names_to_csv(std::ofstream& file) const {
    searchStatistics->stat_names_to_csv(file);
    cegarSearch->stat_names_to_csv(file);
}
