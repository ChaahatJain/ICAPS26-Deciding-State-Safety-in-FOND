//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_OPTIONS_H
#define PLAJA_PA_OPTIONS_H

#include "../../../include/ct_config_const.h"
#include "../smt_base/smt_options.h"

namespace PLAJA_OPTION {

    // (Policy) Predicate Abstraction
    extern const std::string explicit_initial_state_enum;
    extern const std::string pa_state_reachability;
    extern const std::string bfs_per_start_state;
    extern const std::string inc_op_app_test;
    extern const std::string predicate_relations;
    extern const std::string predicate_entailments;
    extern const std::string incremental_search;
    extern const std::string check_for_pa_terminal_states;
    extern const std::string lazyNonTerminalPush;
    extern const std::string lazyPolicyInterfacePush;

    // PACegar
    extern const std::string max_num_preds;
    extern const std::string guards_as_preds;
    extern const std::string terminal_as_preds;
    extern const std::string pa_state_aware;
    extern const std::string check_policy_spuriousness;
    extern const std::string compute_wp;
    FIELD_IF_LAZY_PA(extern const std::string lazy_pa_state;)
    FIELD_IF_LAZY_PA(extern const std::string lazy_pa_app;)
    FIELD_IF_LAZY_PA(extern const std::string lazy_pa_sel;)
    FIELD_IF_LAZY_PA(extern const std::string lazy_pa_target;)
    extern const std::string add_guards;
    extern const std::string add_predicates;
    extern const std::string exclude_entailments;
    extern const std::string re_sub;
    extern const std::string predicates_file;
    extern const std::string save_intermediate_predicates;

    extern const std::string cacheWitnesses; // only used internally

}

namespace PLAJA_OPTION_DEFAULT {

    constexpr bool inc_op_app_test(not PLAJA_GLOBAL::marabouDisBaselineSupport);
    constexpr bool predicate_relations(true);
    constexpr bool predicate_entailments(true);
    constexpr bool incremental_search(true);
    constexpr bool check_for_pa_terminal_states(PLAJA_GLOBAL::enableTerminalStateSupport);
    constexpr bool lazyNonTerminalPush(true);
    constexpr bool lazyPolicyInterfacePush(false);

    constexpr int max_num_preds(16777216); // Just a large number, 2^24.
    constexpr bool guards_as_preds(false);
    constexpr bool terminal_as_preds(false);
    constexpr bool pa_state_aware(false);
    constexpr bool check_policy_spuriousness(true);
    constexpr bool compute_wp(true);
    FIELD_IF_LAZY_PA(constexpr bool lazy_pa_state(PLAJA_GLOBAL::lazyPA);)
    FIELD_IF_LAZY_PA(constexpr bool lazy_pa_app(false);)
    FIELD_IF_LAZY_PA(constexpr bool lazy_pa_sel(PLAJA_GLOBAL::lazyPA);)
    FIELD_IF_LAZY_PA(constexpr bool lazy_pa_target(false);)
    constexpr bool exclude_entailments(true);
}

#endif //PLAJA_PA_OPTIONS_H
