//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OP_INC_DATA_PA_H
#define PLAJA_OP_INC_DATA_PA_H

#include <unordered_set>
#include "../../../../include/pa_config_const.h"
#include "../../../../parser/using_parser.h"
#include "../../../../utils/default_constructors.h"

/** Struct to encapsulate data passed bottom-down during incrementation. */
struct OpIncDataPa final {

private:
    mutable Z3_IN_PLAJA::SMTSolver* solver;

#ifndef LAZY_PA
    std::unordered_set<VariableID_type> varsInNewlyAddedPreds;
#endif

public:
    explicit OpIncDataPa(Z3_IN_PLAJA::SMTSolver& solver):
        solver(&solver) {
    }

    ~OpIncDataPa() = default;
    DELETE_CONSTRUCTOR(OpIncDataPa)

    inline Z3_IN_PLAJA::SMTSolver& get_solver() const { return *solver; }

#ifdef LAZY_PA

    inline const std::unordered_set<VariableID_type>& get_vars_in_newly_added_preds() const { PLAJA_ABORT }

    inline std::unordered_set<VariableID_type>& access_vars_in_newly_added_preds() { PLAJA_ABORT }

#else

    inline const std::unordered_set<VariableID_type>& get_vars_in_newly_added_preds() const { return varsInNewlyAddedPreds; }

    inline std::unordered_set<VariableID_type>& access_vars_in_newly_added_preds() { return varsInNewlyAddedPreds; }

#endif

};

#endif //PLAJA_OP_INC_DATA_PA_H
