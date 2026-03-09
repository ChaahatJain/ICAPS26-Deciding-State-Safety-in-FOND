//
// This file is part of the PlaJA code base (2019 - 2023).
//

#ifndef PLAJA_TO_VERITAS_VISITOR_H
#define PLAJA_TO_VERITAS_VISITOR_H

#include <memory>
#include "../../forward_smt_veritas.h"
#include "../../using_veritas.h"

class Expression;

namespace VERITAS_IN_PLAJA {

    extern std::unique_ptr<VeritasConstraint> to_veritas_constraint(const Expression& exp, const StateIndexesInVeritas& state_indexes);
    extern std::unique_ptr<PredicateConstraint> to_predicate_constraint(const Expression& constraint, const StateIndexesInVeritas& state_indexes);

    extern std::unique_ptr<VeritasConstraint> to_assignment_constraint(VeritasVarIndex_type target_var, const Expression& assignment_sum, const StateIndexesInVeritas& state_indexes);
    extern std::unique_ptr<VeritasConstraint> to_lb_constraint(VeritasVarIndex_type target_var, const Expression& lb_sum, const StateIndexesInVeritas& state_indexes);
    extern std::unique_ptr<VeritasConstraint> to_ub_constraint(VeritasVarIndex_type target_var, const Expression& ub_sum, const StateIndexesInVeritas& state_indexes);

    extern std::unique_ptr<VeritasConstraint> to_assignment_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type target_var, VeritasFloating_type scalar);
    extern std::unique_ptr<VeritasConstraint> to_copy_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type target_var, VeritasVarIndex_type src_var);

}

#endif //PLAJA_TO_VERITAS_VISITOR_H
