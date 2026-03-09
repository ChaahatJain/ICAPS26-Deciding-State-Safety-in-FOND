//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_MARABOU_VISITOR_H
#define PLAJA_TO_MARABOU_VISITOR_H

#include <memory>
#include "../../forward_smt_nn.h"
#include "../../using_marabou.h"

class Expression;

namespace MARABOU_IN_PLAJA {

    extern std::unique_ptr<MarabouConstraint> to_marabou_constraint(const Expression& exp, const StateIndexesInMarabou& state_indexes);
    extern std::unique_ptr<PredicateConstraint> to_predicate_constraint(const Expression& constraint, const StateIndexesInMarabou& state_indexes);

    extern std::unique_ptr<MarabouConstraint> to_assignment_constraint(MarabouVarIndex_type target_var, const Expression& assignment_sum, const StateIndexesInMarabou& state_indexes);
    extern std::unique_ptr<MarabouConstraint> to_lb_constraint(MarabouVarIndex_type target_var, const Expression& lb_sum, const StateIndexesInMarabou& state_indexes);
    extern std::unique_ptr<MarabouConstraint> to_ub_constraint(MarabouVarIndex_type target_var, const Expression& ub_sum, const StateIndexesInMarabou& state_indexes);

    extern std::unique_ptr<MarabouConstraint> to_assignment_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type target_var, MarabouFloating_type scalar);
    extern std::unique_ptr<MarabouConstraint> to_copy_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type target_var, MarabouVarIndex_type src_var);

}

#endif //PLAJA_TO_MARABOU_VISITOR_H
