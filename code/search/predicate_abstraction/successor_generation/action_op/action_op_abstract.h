//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_ABSTRACT_H
#define PLAJA_ACTION_OP_ABSTRACT_H

#include <z3++.h>
#include "../../../../parser/ast/forward_ast.h"
#include "../../../../stats/forward_stats.h"
#include "../../../smt/successor_generation/action_op_to_z3.h"
#include "../update/update_abstract.h"

class ActionOpAbstract: public ActionOpToZ3 {
    friend class SuccessorGeneratorAbstract; //
    friend struct SuccessorGeneratorAbstractExplorer; //

private:
    std::vector<ConstantIdType> opConstants; // Any (non-inlined array) constants in the operator (including updates)?
    std::vector<VariableID_type> guardVars;
    z3::expr guards;
    // z3::expr opInZ3; // cached z3 encoding of operator

protected:
    void optimize_guards(Z3_IN_PLAJA::SMTSolver& solver);
    void compute_updates();
    // void compute_op_in_z3();

    [[nodiscard]] inline const std::vector<VariableID_type>& _guards_vars() const { return guardVars; };

public:
    /** @param initialize, compute update structures */
    ActionOpAbstract(const ActionOp& parent, const ModelZ3& model_z3, bool initialize = true, PLAJA::StatsBase* shared_stats = nullptr);
    virtual ~ActionOpAbstract();
    DELETE_CONSTRUCTOR(ActionOpAbstract)

    [[nodiscard]] inline const UpdateAbstract& get_update_abstract(UpdateIndex_type index) const { return static_cast<const UpdateAbstract&>(get_update(index)); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

    [[nodiscard]] inline const z3::expr& _guards() const { return guards; }

    // [[nodiscard]] inline const z3::expr& _op_in_z3() const { return opInZ3; } // The encoding is invalid under "no copy assignments" as a target condition (encoded over the target) variables may be falsely fulfilled.

    void add_src_bounds(Z3_IN_PLAJA::SMTSolver& solver) const;

    /**
     * Adds static structures to solver.
     * It is in the responsibility of the caller to push/pop appropriately.
     */
    void add_to_solver(Z3_IN_PLAJA::SMTSolver& solver) const;

    bool guard_enabled(Z3_IN_PLAJA::SMTSolver& solver) const;

    bool guard_enabled_inc(Z3_IN_PLAJA::SMTSolver& solver) const; // does not pop afterwards

    bool guard_entailed(Z3_IN_PLAJA::SMTSolver& solver) const;

    using UpdateIterator = UpdateIteratorBase<UpdateToZ3, UpdateAbstract>;

    [[nodiscard]] inline UpdateIterator updateIterator() const { return UpdateIterator(updates); }

};

#endif //PLAJA_ACTION_OP_ABSTRACT_H
