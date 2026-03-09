//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "ensemble_sat_in_z3_base.h"
#include "../../../../option_parser/option_parser.h"
#include "../../../factories/predicate_abstraction/pa_options.h"
#include "../../../factories/configuration.h"
#include "../../../smt/base/solution_checker_instance.h"
#include "../../../smt_ensemble/solver/solver_veritas.h"
#include "../../../smt_ensemble/solver/solution_veritas.h"
#include "../../../smt_ensemble/to_z3/encoding_information.h"
#include "../../../states/state_values.h"
#include "../../smt/model_z3_pa.h"
#include "../smt/model_veritas_pa.h"
#include "../ensemble_sat_checker/ensemble_sat_checker_factory.h"
#include "../../../smt/context_z3.h"

namespace PLAJA_OPTION { extern const std::string preprocess_query; }

EnsembleSatInZ3Base::EnsembleSatInZ3Base(const Jani2Ensemble& jani_2_ensemble, const PLAJA::Configuration& config):
    EnsembleSatChecker(jani_2_ensemble, config)
    , modelZ3(*std::static_pointer_cast<ModelZ3PA>(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3)))
    , encodingInformation(VERITAS_TO_Z3::EncodingInformation::construct_encoding_information(*PLAJA_GLOBAL::optionParser)) {

    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))

}

EnsembleSatInZ3Base::~EnsembleSatInZ3Base() = default;

//

int EnsembleSatInZ3Base::check_relaxed_for_exact() {
    // relaxed check
    const bool rlt = solver->check();

    if (not rlt) {
        solver->reset();
        return 0;
    }

    // check exact solution
    save_solution_if(rlt);
    if (solutionChecker->check(*lastSolution->get_solution(0), set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get(), lastSolution->get_solution(1))) {
        solver->reset();
        return 1;
    } else { save_solution_if(false); }

    solver->reset();
    return -1; // inconclusive
}
