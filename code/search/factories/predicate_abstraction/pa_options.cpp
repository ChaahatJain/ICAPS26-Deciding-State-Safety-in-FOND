//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <limits>
#include "pa_options.h"

namespace PLAJA_OPTION {

    // (Policy) Predicate Abstraction
    const std::string explicit_initial_state_enum("explicit-initial-state-enum"); // NOLINT(cert-err58-cpp)
#ifndef PA_ONLY_REACHABLE
    const std::string pa_state_reachability("pa-state-reachability"); // NOLINT(cert-err58-cpp)
#endif
    const std::string bfs_per_start_state("bfs-per-start-state"); // NOLINT(cert-err58-cpp)
    const std::string inc_op_app_test("inc-op-app-test"); // NOLINT(cert-err58-cpp)
    const std::string predicate_relations("predicate-relations"); // NOLINT(cert-err58-cpp)
    const std::string predicate_entailments("predicate-entailments"); // NOLINT(cert-err58-cpp)
    const std::string incremental_search("incremental-search"); // NOLINT(cert-err58-cpp)
    const std::string check_for_pa_terminal_states("check-for-pa-terminal-states"); // NOLINT(cert-err58-cpp)
    const std::string lazyNonTerminalPush("lazy-non-terminal-push"); // NOLINT(cert-err58-cpp)
    const std::string lazyPolicyInterfacePush("lazy-policy-interface-push"); // NOLINT(cert-err58-cpp)

    // PACegar
    const std::string max_num_preds("max-num-preds"); // NOLINT(cert-err58-cpp)
    const std::string guards_as_preds("guards-as-preds"); // NOLINT(cert-err58-cpp)
    const std::string terminal_as_preds("terminal-as-preds"); // NOLINT(cert-err58-cpp)
    const std::string pa_state_aware("pa-state-aware"); // NOLINT(cert-err58-cpp)
    const std::string check_policy_spuriousness("check-policy-spuriousness"); // NOLINT(cert-err58-cpp)
    const std::string compute_wp("compute-wp"); // NOLINT(cert-err58-cpp)
    FIELD_IF_LAZY_PA(const std::string lazy_pa_state("lazy-pa-state");) // NOLINT(cert-err58-cpp)
    FIELD_IF_LAZY_PA(const std::string lazy_pa_app("lazy-pa-app");) // NOLINT(cert-err58-cpp)
    FIELD_IF_LAZY_PA(const std::string lazy_pa_sel("lazy-pa-sel");) // NOLINT(cert-err58-cpp)
    FIELD_IF_LAZY_PA(const std::string lazy_pa_target("lazy-pa-target");) // NOLINT(cert-err58-cpp)
    const std::string add_guards("add_guards"); // NOLINT(cert-err58-cpp)
    const std::string add_predicates("add_predicates"); // NOLINT(cert-err58-cpp)
    const std::string exclude_entailments("exclude_entailments"); // NOLINT(cert-err58-cpp)
    const std::string re_sub("re_sub"); // NOLINT(cert-err58-cpp)
    const std::string predicates_file("predicates-file"); // NOLINT(cert-err58-cpp)
    const std::string save_intermediate_predicates("save_intermediate_predicates"); // NOLINT(cert-err58-cpp)

    const std::string cacheWitnesses("cache-witnesses"); // NOLINT(cert-err58-cpp)

}