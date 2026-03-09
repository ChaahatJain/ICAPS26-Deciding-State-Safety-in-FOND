//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_ENSEMBLE_SAT_CHECKER_Z3_H
#define PLAJA_ENSEMBLE_SAT_CHECKER_Z3_H

#include <memory>
#include <vector>
#include "../../../information/jani2ensemble/using_jani2ensemble.h"
#include "../../../using_search.h"
#include "../../using_predicate_abstraction.h"
#include "ensemble_sat_in_z3_base.h"

// forward declaration:
namespace Z3_IN_PLAJA { class SMTSolver; class VeritasToZ3Vars; }


class EnsembleSatCheckerZ3 final: public EnsembleSatInZ3Base {

private:
    std::unique_ptr<VERITAS_IN_PLAJA::VeritasQuery> ensemble_as_query;
    std::unique_ptr<Z3_IN_PLAJA::VeritasToZ3Vars> variables;
    std::vector<z3::expr> outputInterfacePerIndex;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> ensembleSatSolver;

    void add_source_state_z3();
    void add_guard_z3();
    void add_guard_negation_z3();
    void add_update_z3();
    void add_target_predicate_state_z3();
    void add_output_interface_z3();
    void load_policy(ActionLabel_type action_label);


    bool check_z3();
    bool check_() override;

public:
    EnsembleSatCheckerZ3(const Jani2Ensemble& jani_2_ensemble, const PLAJA::Configuration& config);
    ~EnsembleSatCheckerZ3() final;
    DELETE_CONSTRUCTOR(EnsembleSatCheckerZ3)

};


#endif //PLAJA_ENSEMBLE_SAT_CHECKER_Z3_H
