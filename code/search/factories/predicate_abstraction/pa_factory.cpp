//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pa_factory.h"
#include "../../../include/ct_config_const.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/plaja_options.h"
#include "../../information/property_information.h"
#include "../../predicate_abstraction/cegar/pa_cegar.h"
#include "../../predicate_abstraction/cegar/restrictions_checker_cegar.h"
#include "../../predicate_abstraction/predicate_abstraction.h"
#include "../../predicate_abstraction/problem_instance_checker_pa.h"
#include "../../predicate_abstraction/search_pa_path.h"
#include "../configuration.h"
#include "pa_options.h"
#include "search_engine_config_pa.h"

namespace PLAJA_OPTION {

    namespace OP_APPLICABILITY {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

    namespace ABSTRACT_HEURISTIC {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

    namespace NN_SAT_CHECKER_PA {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

    namespace ENSEMBLE_SAT_CHECKER_PA {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }
    namespace SELECTION_REFINEMENT {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

    extern std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_pa_state_reachability_enum();

}

/**********************************************************************************************************************/

PABaseFactory::PABaseFactory(SearchEngineFactory::SearchEngineType engine_type):
    SMTBaseFactory(engine_type) {}

PABaseFactory::~PABaseFactory() = default;

void PABaseFactory::add_options(PLAJA::OptionParser& option_parser) const {
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::savePath);
    //
    SMTBaseFactory::add_options(option_parser);
    // PA general
    // if (not PLAJA_GLOBAL::paOnlyReachability) { OPTION_PARSER::add_enum_option(option_parser, PLAJA_OPTION::pa_state_reachability, PLAJA_OPTION::construct_pa_state_reachability_enum()); }
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::explicit_initial_state_enum);
    PLAJA_OPTION::add_initial_state_enum(option_parser);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::search_per_start, PLAJA_OPTION_DEFAULT::search_per_start);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::inc_op_app_test, PLAJA_OPTION_DEFAULT::inc_op_app_test);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::predicate_relations, PLAJA_OPTION_DEFAULT::predicate_relations);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::predicate_entailments, PLAJA_OPTION_DEFAULT::predicate_entailments);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::incremental_search, PLAJA_OPTION_DEFAULT::incremental_search);
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::check_for_pa_terminal_states, PLAJA_OPTION_DEFAULT::check_for_pa_terminal_states); }
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::lazyNonTerminalPush, PLAJA_OPTION_DEFAULT::lazyNonTerminalPush); }
    if constexpr (PLAJA_GLOBAL::enableApplicabilityFiltering) { OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::lazyPolicyInterfacePush, PLAJA_OPTION_DEFAULT::lazyPolicyInterfacePush); }
    if constexpr (PLAJA_GLOBAL::enableApplicabilityFiltering) { PLAJA_OPTION::OP_APPLICABILITY::add_options(option_parser); }
    if constexpr (PLAJA_GLOBAL::enableHeuristicSearchPA) { PLAJA_OPTION::ABSTRACT_HEURISTIC::add_options(option_parser); }
    // NN-SAT
    PLAJA_OPTION::NN_SAT_CHECKER_PA::add_options(option_parser);
    #ifdef USE_VERITAS
    PLAJA_OPTION::ENSEMBLE_SAT_CHECKER_PA::add_options(option_parser);
    #endif

}

void PABaseFactory::print_options() const {
    // Predicate Abstraction
    OPTION_PARSER::print_options_headline("Predicate Abstraction");
    // if (not PLAJA_GLOBAL::paOnlyReachability) { OPTION_PARSER::print_enum_option(PLAJA_OPTION::pa_state_reachability, *PLAJA_OPTION::construct_pa_state_reachability_enum(), "Check for reachability only globally or only per source state/label/op."); }
    OPTION_PARSER::print_flag(PLAJA_OPTION::explicit_initial_state_enum, "Use explicit state enumeration to compute abstract initial states (if compactly represented).");
    PLAJA_OPTION::print_search_per_start();
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::inc_op_app_test, PLAJA_OPTION_DEFAULT::inc_op_app_test, "Apply operator applicability tests incrementally, i.e, per-operator expansion.", true);
    OPTION_PARSER::print_additional_specification("For selection tests or applicability filtering, it may improve performance to check applicability for all operators per-abstract-source-state expansion at once.");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::predicate_relations, PLAJA_OPTION_DEFAULT::predicate_relations, "PA state expansion (and PA start state enumeration) reflect binary predicate relations.");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::predicate_entailments, PLAJA_OPTION_DEFAULT::predicate_entailments, "PA state expansion computes predicate truth values entailed by the action operator and the abstract source state.");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::incremental_search, PLAJA_OPTION_DEFAULT::incremental_search, "Run search in CEGAR loop incrementally.");
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { OPTION_PARSER::print_bool_option(PLAJA_OPTION::check_for_pa_terminal_states, PLAJA_OPTION_DEFAULT::check_for_pa_terminal_states, "Check for pa states that fulfill terminal state condition."); }
    // TODO might add information on lazyNonTerminalPush and PLAJA_OPTION::lazyPolicyInterfacePush.
    if constexpr (PLAJA_GLOBAL::enableApplicabilityFiltering) { PLAJA_OPTION::OP_APPLICABILITY::print_options(); }
    if constexpr (PLAJA_GLOBAL::enableHeuristicSearchPA) { PLAJA_OPTION::ABSTRACT_HEURISTIC::print_options(); }
    OPTION_PARSER::print_options_footline();

    #ifdef USE_VERITAS
    // ENSEMBLE-SAT
    OPTION_PARSER::print_options_headline("ENSEMBLE-SAT (PA)");
    PLAJA_OPTION::ENSEMBLE_SAT_CHECKER_PA::print_options();
    #endif

    // NN-SAT
    OPTION_PARSER::print_options_headline("NN-SAT (PA)");
    PLAJA_OPTION::NN_SAT_CHECKER_PA::print_options();
    SMTBaseFactory::print_options_base();
    OPTION_PARSER::print_options_footline();
}

bool PABaseFactory::supports_property_sub(const PropertyInformation& prop_info) const { return prop_info.get_predicates(); }

std::unique_ptr<PLAJA::Configuration> PABaseFactory::init_configuration(const PLAJA::OptionParser& option_parser, const PropertyInformation* property_info) const {
    auto config = std::make_unique<SearchEngineConfigPA>(option_parser);
    if (property_info) { config->set_sharable_const(PLAJA::SharableKey::PROP_INFO, property_info); }
    return config;
}

/**********************************************************************************************************************/

PAFactory::PAFactory():
    PABaseFactory(SearchEngineFactory::PaType) {}

PAFactory::~PAFactory() = default;

std::unique_ptr<SearchEngine> PAFactory::construct(const PLAJA::Configuration& config) const {
    return std::make_unique<PredicateAbstraction>(PLAJA_UTILS::cast_ref<SearchEngineConfigPA>(config));
}

/**********************************************************************************************************************/

SearchPAPathFactory::SearchPAPathFactory():
    PABaseFactory(SearchEngineFactory::PaPathType) {}

SearchPAPathFactory::~SearchPAPathFactory() = default;

std::unique_ptr<SearchEngine> SearchPAPathFactory::construct(const PLAJA::Configuration& config) const {
    return std::make_unique<SearchPAPath>(PLAJA_UTILS::cast_ref<SearchEngineConfigPA>(config));
}

bool SearchPAPathFactory::supports_property_sub(const PropertyInformation& prop_info) const { return PABaseFactory::supports_property_sub(prop_info) and prop_info.get_reach(); }

/**********************************************************************************************************************/

PACegarFactory::PACegarFactory():
    PABaseFactory(SearchEngineFactory::PaCegarType) {}

PACegarFactory::~PACegarFactory() = default;

void PACegarFactory::supports_model(const Model& model) const { RestrictionsCheckerCegar::check_restrictions(&model, false); }

void PACegarFactory::add_options(PLAJA::OptionParser& option_parser) const {
    PABaseFactory::add_options(option_parser);
    //
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::max_num_preds, PLAJA_OPTION_DEFAULT::max_num_preds);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::guards_as_preds, PLAJA_OPTION_DEFAULT::guards_as_preds);
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::terminal_as_preds, PLAJA_OPTION_DEFAULT::terminal_as_preds); }
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::pa_state_aware, PLAJA_OPTION_DEFAULT::pa_state_aware);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::check_policy_spuriousness, PLAJA_OPTION_DEFAULT::check_policy_spuriousness);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::compute_wp, PLAJA_OPTION_DEFAULT::compute_wp);
    STMT_IF_LAZY_PA(OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::lazy_pa_state, PLAJA_OPTION_DEFAULT::lazy_pa_state);)
    STMT_IF_LAZY_PA(OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::lazy_pa_app, PLAJA_OPTION_DEFAULT::lazy_pa_app);)
    STMT_IF_LAZY_PA(OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::lazy_pa_sel, PLAJA_OPTION_DEFAULT::lazy_pa_sel);)
    STMT_IF_LAZY_PA(OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::lazy_pa_target, PLAJA_OPTION_DEFAULT::lazy_pa_target);)
    PLAJA_OPTION::SELECTION_REFINEMENT::add_options(option_parser);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::add_guards);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::add_predicates);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::exclude_entailments, PLAJA_OPTION_DEFAULT::exclude_entailments);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::re_sub);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::predicates_file);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::save_intermediate_predicates);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::local_backup);
}

void PACegarFactory::print_options() const {
    PABaseFactory::print_options();
    // PACegar
    OPTION_PARSER::print_options_headline("PA CEGAR");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::max_num_preds, PLAJA_OPTION_DEFAULT::max_num_preds, "Maximal number of predicates.");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::guards_as_preds, PLAJA_OPTION_DEFAULT::guards_as_preds, "Add transition guards to the initial predicate set.");
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { OPTION_PARSER::print_bool_option(PLAJA_OPTION::terminal_as_preds, PLAJA_OPTION_DEFAULT::terminal_as_preds, "Add terminal state condition to the initial predicate set."); }
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::pa_state_aware, PLAJA_OPTION_DEFAULT::pa_state_aware, "Spuriousness check is aware of abstract state path.");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::check_policy_spuriousness, PLAJA_OPTION_DEFAULT::check_policy_spuriousness, "Check policy-spuriousness exactly (experimental).");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::compute_wp, PLAJA_OPTION_DEFAULT::compute_wp, "Apply weakest precondition for refinement. Required for completeness if pa-state-aware is not set.");
    STMT_IF_LAZY_PA(OPTION_PARSER::print_bool_option(PLAJA_OPTION::lazy_pa_state, PLAJA_OPTION_DEFAULT::lazy_pa_state, "Perform abstract state related refinement in CEGAR loop lazily.");)
    STMT_IF_LAZY_PA(OPTION_PARSER::print_bool_option(PLAJA_OPTION::lazy_pa_app, PLAJA_OPTION_DEFAULT::lazy_pa_app, "Perform operator applicability related refinement in CEGAR loop lazily.");)
    STMT_IF_LAZY_PA(OPTION_PARSER::print_bool_option(PLAJA_OPTION::lazy_pa_sel, PLAJA_OPTION_DEFAULT::lazy_pa_sel, "Perform (policy) selection related refinement in CEGAR loop lazily.");)
    STMT_IF_LAZY_PA(OPTION_PARSER::print_bool_option(PLAJA_OPTION::lazy_pa_target, PLAJA_OPTION_DEFAULT::lazy_pa_target, "Perform target condition related refinement in CEGAR loop lazily.");)
    PLAJA_OPTION::SELECTION_REFINEMENT::print_options();
    OPTION_PARSER::print_flag(PLAJA_OPTION::add_guards, "Add (wp of) guards of spurious paths.");
    OPTION_PARSER::print_flag(PLAJA_OPTION::add_predicates, "Add (path wp of) predicates (already present).");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::exclude_entailments, PLAJA_OPTION_DEFAULT::exclude_entailments, "Do not add sub-predicates entailed on the spurious paths.");
    OPTION_PARSER::print_flag(PLAJA_OPTION::re_sub, "Resubstitute predicates during WP computation.");
    OPTION_PARSER::print_option(PLAJA_OPTION::predicates_file, OPTION_PARSER::fileStr, "Save (final) predicate set to file (as PA property)."); // "The reach expression of the input property will be included."
    OPTION_PARSER::print_flag(PLAJA_OPTION::save_intermediate_predicates, "Save (per-refinement) predicate sets as well.");
    OPTION_PARSER::print_local_backup();
    OPTION_PARSER::print_options_footline();
}

std::unique_ptr<SearchEngine> PACegarFactory::construct(const PLAJA::Configuration& config) const {
    return std::make_unique<PACegar>(PLAJA_UTILS::cast_ref<SearchEngineConfigPA>(config));
}

bool PACegarFactory::supports_property_sub(const PropertyInformation& prop_info) const { return prop_info.get_reach(); }

/**********************************************************************************************************************/

ProblemInstanceCheckerPaFactory::ProblemInstanceCheckerPaFactory():
    PABaseFactory(SearchEngineType::ProblemInstanceCheckerPaType) {
}

ProblemInstanceCheckerPaFactory::~ProblemInstanceCheckerPaFactory() = default;

std::unique_ptr<SearchEngine> ProblemInstanceCheckerPaFactory::construct(const PLAJA::Configuration& config) const {
    return std::make_unique<ProblemInstanceCheckerPa>(PLAJA_UTILS::cast_ref<SearchEngineConfigPA>(config));
}

bool ProblemInstanceCheckerPaFactory::supports_property_sub(const PropertyInformation& prop_info) const {
    return prop_info._property_type() == PropertyInformation::ProblemInstanceProperty;
}

namespace SEARCH_ENGINE_FACTORY {

    std::unique_ptr<SearchEngineFactory> construct_problem_instance_checker_pa_factory() {
        return std::make_unique<ProblemInstanceCheckerPaFactory>();
    }
}