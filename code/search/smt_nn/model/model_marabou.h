//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MODEL_MARABOU_H
#define PLAJA_MODEL_MARABOU_H

#include <list>
#include <memory>
#include <vector>
#include "../../../include/marabou_include/forward_marabou.h"
#include "../../../include/marabou_include/milp_solver_bound_tightening_type.h"
#include "../../../include/ct_config_const.h"
#include "../../information/forward_information.h"
#include "../../smt/base/model_smt.h"
#include "../../using_search.h"
#include "../forward_smt_nn.h"
#include "../vars_in_marabou.h"

class ModelMarabou: public ModelSmt {

protected:
    MARABOU_IN_PLAJA::Context* context;

    std::vector<std::unique_ptr<StateIndexesInMarabou>> stateIndexesInMarabou;
    std::vector<std::pair<MarabouVarIndex_type, MarabouVarIndex_type>> maxVarPerStep; // Latter is with target vars.

    std::unique_ptr<MarabouConstraint> startMarabou;
    // std::unique_ptr<MarabouConstraint> startMarabouWithInit; // Rewriting startMarabou as a disjunction is too expensive on our benchmarks.
    std::vector<std::unique_ptr<MarabouConstraint>> reachMarabou;

    std::vector<std::unique_ptr<NNInMarabou>> nnInMarabouPerStep;
#ifdef ENABLE_APPLICABILITY_FILTERING
    std::unique_ptr<MARABOU_IN_PLAJA::OutputConstraints> outputConstraints;
#else
    std::unique_ptr<MARABOU_IN_PLAJA::OutputConstraintsNoFilter> outputConstraints;
#endif

    /* Ops. */
    std::unique_ptr<MARABOU_IN_PLAJA::SuccessorGenerator> successorGenerator;

    /* Tightening. */
    MILPSolverBoundTighteningType nnBounds; // Do a initial pass of lp-based bound tightening?
    MILPSolverBoundTighteningType nnBoundsPerLabel;
    std::vector<std::vector<std::unique_ptr<Tightening>>> tighteningsPerLabel;

    void init_output_constraints();

    void generate_state_indexes(StateIndexesInMarabou& state_indexes, bool do_locs, bool mark_input) const;

    [[nodiscard]] std::vector<MarabouVarIndex_type> extract_nn_inputs(const StateIndexesInMarabou& state_indexes) const; // NN after state indexes

    void match_state_vars_to_nn(const NNInMarabou& nn_in_marabou, StateIndexesInMarabou& state_indexes) const; // state indexes after NN

public:
    explicit ModelMarabou(const PLAJA::Configuration& config, bool init_suc_gen = true);
    ~ModelMarabou() override;
    DELETE_CONSTRUCTOR(ModelMarabou)

    /******************************************************************************************************************/

    [[nodiscard]] std::shared_ptr<MARABOU_IN_PLAJA::Context> share_context() const;

    [[nodiscard]] inline MARABOU_IN_PLAJA::Context& get_context() const { return *context; }

    /******************************************************************************************************************/

    void set_start(std::unique_ptr<Expression>&& start) override;

    void set_reach(std::unique_ptr<Expression>&& reach) override;

    void set_non_terminal(std::unique_ptr<Expression>&& terminal, bool negate) override;

    /******************************************************************************************************************/

    void generate_steps(StepIndex_type max_step) override;

    [[nodiscard]] inline const StateIndexesInMarabou& get_state_indexes(StepIndex_type step = 0) const {
        PLAJA_ASSERT(step < stateIndexesInMarabou.size())
        return *stateIndexesInMarabou[step];
    }

    [[nodiscard]] inline StateIndexesInMarabou& get_state_indexes(StepIndex_type step = 0) {
        PLAJA_ASSERT(step < stateIndexesInMarabou.size())
        return *stateIndexesInMarabou[step];
    }

    /* Start & reach. */

    void add_start(MARABOU_IN_PLAJA::QueryConstructable& query) const;

    void add_init(MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step = 0) const;

    void exclude_start(MARABOU_IN_PLAJA::QueryConstructable& query, bool include_init, StepIndex_type step = 0) const;

    void add_reach(MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step = 0) const;

    void exclude_reach(MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step = 0) const;

    /* NN */

    [[nodiscard]] inline const NNInMarabou& get_nn_in_marabou(StepIndex_type step = 0) const {
        PLAJA_ASSERT(step < stateIndexesInMarabou.size())
        return *nnInMarabouPerStep[step];
    }

    [[nodiscard]] std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery> make_query(bool add_nn, bool add_target_vars, StepIndex_type step = 0) const;

    void add_nn_to_query(MARABOU_IN_PLAJA::MarabouQuery& query, StepIndex_type step = 0) const;

    void add_output_interface(MARABOU_IN_PLAJA::QueryConstructable& query, ActionLabel_type action_label, StepIndex_type step = 0) const;

    void add_output_interface_negation(MARABOU_IN_PLAJA::QueryConstructable& query, ActionLabel_type action_label, StepIndex_type step = 0) const;

    void add_output_interface_other_label(MARABOU_IN_PLAJA::MarabouQuery& query, ActionLabel_type action_label, StepIndex_type step = 0) const;

    void add_output_interface_for_op(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, StepIndex_type step = 0) const;

    /* action op */

    [[nodiscard]] const std::vector<const ActionOpMarabou*>& get_action_structure(ActionLabel_type label) const;

    [[nodiscard]] const ActionOpMarabou& get_action_op(ActionOpID_type op_id) const;

    void add_guard(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, bool do_locs, StepIndex_type step = 0) const;

    void add_update(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step = 0) const;

    void add_action_op(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step = 0) const;

    [[nodiscard]] std::unique_ptr<MarabouConstraint> guards_to_marabou(ActionLabel_type action_label, StepIndex_type step = 0) const;

    void add_guard_disjunction(MARABOU_IN_PLAJA::QueryConstructable& query, ActionLabel_type action_label, StepIndex_type step = 0) const;

    void add_choice_point(MARABOU_IN_PLAJA::QueryConstructable& query, bool nn_aware, StepIndex_type step) const;

    void add_loop_exclusion(MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step_index) const;

    /* aux */

    [[nodiscard]] std::unique_ptr<MarabouConstraint> to_marabou(const Expression& expr, StepIndex_type step_index) const;

    void add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query, const Expression& expr, StepIndex_type step_index) const;

    /* single step interface */
    [[nodiscard]] inline const StateIndexesInMarabou& get_src_in_marabou() const { return get_state_indexes(0); }

    /** Solver interface. *********************************************************************************************/

   [[nodiscard]] std::unique_ptr<PLAJA::SmtSolver> init_solver(const PLAJA::Configuration& config, StepIndex_type step) const final;

    [[nodiscard]] std::unique_ptr<PLAJA::SmtConstraint> to_smt(const Expression& expr, StepIndex_type step) const final;

    void add_to_solver(PLAJA::SmtSolver& solver, const Expression& expr, StepIndex_type step) const final;

    /** @return false if the query has been derived unsat. */
    static bool lp_tightening(MARABOU_IN_PLAJA::QueryConstructable& query, MILPSolverBoundTighteningType type, LayerIndex_type layer_index = LAYER_INDEX::none);

    bool lp_tightening_output(MARABOU_IN_PLAJA::QueryConstructable& query, MILPSolverBoundTighteningType type) const;

};

#endif //PLAJA_MODEL_MARABOU_H
