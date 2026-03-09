//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "to_nuxmv_options.h"

namespace PLAJA_OPTION {

    const std::string nuxmv_model("nuxmv-model"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_use_ex_model("nuxmv-use-ex-model"); // NOLINT(cert-err58-cpp)

    const std::string nuxmv_config_file("nuxmv-config-file"); // NOLINT(cert-err58-cpp)

    const std::string nuxmv_config("nuxmv-config"); // NOLINT(cert-err58-cpp)

    const std::string nuxmv_default_engine("nuxmv-default-engine"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_engine_config("nuxmv-engine-config"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_bmc_len("nuxmv-bmc-len"); // NOLINT(cert-err58-cpp)

    const std::string nuxmv_ex("nuxmv-ex"); // NOLINT(cert-err58-cpp)

    const std::string nuxmv_use_case("nuxmv-use-case"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_invar_for_reals("nuxmv-invar-for-reals"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_def_guards("nuxmv-def-guards"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_def_updates("nuxmv-def-updates"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_def_reach("nuxmv-def-reach"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_use_guard_per_update("nuxmv-use-guard-per-update"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_merge_updates("nuxmv-merges-updates"); // NOLINT(cert-err58-cpp)

    const std::string nuxmv_backwards_def_order("nuxmv-backwards-def-order"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_backwards_bool_vars("nuxmv-backwards-bool-vars"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_backwards_copy_updates("nuxmv-backwards-copy-updates"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_backwards_op_exclusion("nuxmv-backwards-op-exclusion"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_backwards_def_goal("nuxmv-backwards-def-goal"); // NOLINT(cert-err58-cpp)

    const std::string nuxmv_backwards_nn_precision("nuxmv-backwards-nn-precision"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_backwards_policy_selection("nuxmv-backwards-policy-selection"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_backwards_policy_module("nuxmv-backwards-policy-module"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_backwards_policy_module_str("nuxmv-backwards-policy-module-str"); // NOLINT(cert-err58-cpp)

    const std::string nuxmv_backwards_suppress_ast_special("nuxmv-backwards-suppress-ast-special"); // NOLINT(cert-err58-cpp)
    const std::string nuxmv_backwards_special_trivial_bounds("nuxmv-backwards-special-trivial-bounds"); // NOLINT(cert-err58-cpp)

}

namespace PLAJA_OPTION_DEFAULT {
    const std::string nuxmv_model("./nuxmv_model.smv"); // NOLINT(cert-err58-cpp);
}