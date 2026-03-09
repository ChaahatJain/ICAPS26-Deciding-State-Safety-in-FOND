//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "nn_sat_in_z3_base.h"
#include "../../../../option_parser/option_parser.h"
#include "../../../factories/predicate_abstraction/pa_options.h"
#include "../../../factories/configuration.h"
#include "../../../smt/base/solution_checker_instance.h"
#include "../../../smt_nn/solver/smt_solver_marabou.h"
#include "../../../smt_nn/solver/solution_marabou.h"
#include "../../../smt_nn/to_z3/encoding_information.h"
#include "../../../smt_nn/utils/preprocessed_bounds.h"
#include "../../../states/state_values.h"
#include "../../smt/model_z3_pa.h"
#include "../smt/model_marabou_pa.h"
#include "../../solution_checker_pa.h"
#include "../nn_sat_checker/nn_sat_checker_factory.h"

namespace PLAJA_OPTION { extern const std::string preprocess_query; }

NNSatInZ3Base::NNSatInZ3Base(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config):
    NNSatChecker(jani_2_nnet, config)
    , modelZ3(*std::static_pointer_cast<ModelZ3PA>(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3)))
    , encodingInformation(MARABOU_TO_Z3::EncodingInformation::construct_encoding_information(*PLAJA_GLOBAL::optionParser))
    , relaxedForExact(config.is_flag_set(PLAJA_OPTION::relaxed_for_exact))
    , preprocessQuery(config.is_flag_set(PLAJA_OPTION::preprocess_query)) {

    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    PLAJA_ASSERT(PLAJA_UTILS::is_derived_ptr_type<ModelZ3PA>(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3).get()))
}

NNSatInZ3Base::~NNSatInZ3Base() = default;

//

int NNSatInZ3Base::check_relaxed_for_exact(std::unique_ptr<MARABOU_IN_PLAJA::PreprocessedBounds>* preprocessed_bounds) {
    PLAJA_ASSERT(not solverMarabou->is_exact())

    // relaxed check
    const bool rlt = solverMarabou->check_relaxed();

    if (not rlt) {
        solverMarabou->reset();
        return 0;
    }

    // check exact solution
    save_solution_if(rlt);
    if (solutionChecker->check(*lastSolution->get_solution(0), set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get(), lastSolution->get_solution(1))) {
        solverMarabou->reset();
        return 1;
    } else { save_solution_if(false); }

    // extract bounds if requested:
    if (preprocessed_bounds) { *preprocessed_bounds = solverMarabou->extract_bounds(); }

    solverMarabou->reset();
    return -1; // inconclusive
}
