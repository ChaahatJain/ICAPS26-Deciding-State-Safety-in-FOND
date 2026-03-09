//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MODEL_Z3_H
#define PLAJA_MODEL_Z3_H

#include <unordered_map>
#include <vector>
#include "../../../include/factory_config_const.h"
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../stats/forward_stats.h"
#include "../base/model_smt.h"
#include "../utils/forward_z3.h"
#include "../forward_smt_z3.h"
#include "../using_smt.h"
#include "model_z3_forward_declarations.h"

class ActionOpAbstract;

class SuccessorGeneratorAbstract;

/** Base class, parameterized with respect to source and target variables. */
class ModelZ3: public ModelSmt {

protected:
    Z3_IN_PLAJA::Context* context;

    const bool nnSupport; // for z3 we usually do not need nn support ...
    const bool nnMultiStepSupport; // ... especially not multi-step

    /* Z3 structures. */

    std::unordered_map<ConstantIdType, Z3_IN_PLAJA::VarId_type> constants; // Array constants will not be inlined.
    std::vector<std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3>> variablesPerStep;

    std::unique_ptr<z3::expr> startZ3;
    std::unique_ptr<z3::expr> startZ3WithInit;
    std::vector<std::unique_ptr<z3::expr>> reachZ3;

    /* NN. */
    std::vector<std::unique_ptr<NNInZ3>> nnInZ3PerStep;
#ifdef ENABLE_APPLICABILITY_FILTERING
    std::unique_ptr<Z3_IN_PLAJA::OutputConstraints> outputConstraints;
#else
    std::unique_ptr<Z3_IN_PLAJA::OutputConstraintsNoFilter> outputConstraints;
#endif

#ifdef BUILD_PA
    /* Ops. */ // TODO for now just cache PA-base ops, long-term analogously to Marabou, i.e., store multi-step-capable op in model and cache in succ-gen.
    std::unordered_map<ActionOpID_type, const ActionOpAbstract*> actionOps;
    std::unordered_map<ActionLabel_type, std::list<const ActionOpAbstract*>> actionOpsPerLabel;
#endif
    std::unique_ptr<SuccessorGeneratorToZ3> successorGenerationToZ3; // TODO for now cache for convenience.

    [[nodiscard]] std::vector<Z3_IN_PLAJA::VarId_type> extract_nn_inputs(const Z3_IN_PLAJA::StateVarsInZ3& state_vars) const;

public:
    explicit ModelZ3(const PLAJA::Configuration& config);
    ~ModelZ3() override;
    DELETE_CONSTRUCTOR(ModelZ3)

    /******************************************************************************************************************/

    [[nodiscard]] std::shared_ptr<Z3_IN_PLAJA::Context> share_context() const;

    [[nodiscard]] inline Z3_IN_PLAJA::Context& get_context() const { return *context; }

    [[nodiscard]] z3::context& get_context_z3() const;

    /******************************************************************************************************************/

    [[nodiscard]] inline bool has_constant_var(ConstantIdType id) const { return constants.count(id); }

    [[nodiscard]] inline Z3_IN_PLAJA::VarId_type get_constant_var(ConstantIdType id) const {
        PLAJA_ASSERT(has_constant_var(id))
        return constants.at(id);
    }

    /******************************************************************************************************************/

    void set_start(std::unique_ptr<Expression>&& start) override;

    void set_reach(std::unique_ptr<Expression>&& reach) override;

    [[nodiscard]] bool initial_state_is_subsumed_by_start() const final;

    /******************************************************************************************************************/

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3> generate_state_variables(StepIndex_type step_index) const;

    /* Z3 model structure generation. */

    [[nodiscard]] z3::expr compute_assignment(const Assignment& assignment, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, std::unordered_map<VariableID_type, std::vector<z3::expr>>* updated_array_indexes, DestinationZ3* parent) const;

    [[nodiscard]] std::unique_ptr<DestinationZ3> compute_destination(const Destination& dest, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;

    [[nodiscard]] std::unique_ptr<EdgeZ3> compute_edge(const Edge& edge, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;

    [[nodiscard]] std::unique_ptr<AutomatonZ3> compute_automaton(const Automaton& automaton, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;

    [[nodiscard]] std::vector<std::unique_ptr<AutomatonZ3>> compute_model(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;

    /* Expression to z3. */

    [[nodiscard]] ToZ3Expr to_z3(const Expression& expr, StepIndex_type step_index) const;

    [[nodiscard]] z3::expr to_z3_condition(const Expression& expr, StepIndex_type step_index) const;

    void add_expression(Z3_IN_PLAJA::SMTSolver& solver, const Expression& condition, StepIndex_type step_index) const;

    /** Steps. ********************************************************************************************************/

    void generate_steps(StepIndex_type max_step) override;

    [[nodiscard]] inline const Z3_IN_PLAJA::StateVarsInZ3& get_state_vars(StepIndex_type step = 0) const {
        PLAJA_ASSERT(step < variablesPerStep.size())
        return *variablesPerStep[step];
    }

    [[nodiscard]] const z3::expr& get_var(VariableID_type var, StepIndex_type step = 0) const;

    void register_constant(Z3_IN_PLAJA::SMTSolver& solver, ConstantIdType constant) const;

    void register_bound(Z3_IN_PLAJA::SMTSolver& solver, VariableID_type var, StepIndex_type step = 0) const;

    void register_bounds(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step, bool ignore_locs) const;

    /** Should be used if bounds are expected to be non-present. */
    inline void add_bounds(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step, bool ignore_locs) const { register_bounds(solver, step, ignore_locs); }

    void register_bounds(Z3_IN_PLAJA::SMTSolver& solver, const Expression& expr, StepIndex_type step = 0) const;

    void add_start(Z3_IN_PLAJA::SMTSolver& solver, bool include_init) const;

    void add_init(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step = 0) const;

    void exclude_start(Z3_IN_PLAJA::SMTSolver& solver, bool include_init, StepIndex_type step = 0) const;

    void add_reach(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step = 0) const;

    void exclude_reach(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step = 0) const;

    [[nodiscard]] inline const NNInZ3& get_nn_in_smt(StepIndex_type step) const {
        PLAJA_ASSERT(step < nnInZ3PerStep.size())
        return *nnInZ3PerStep[step];
    }

    void add_nn(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const;

    // [[nodiscard]] const std::vector<z3::expr>& get_output_interfaces(StepIndex_type step = 0) const { PLAJA_ASSERT(step < outputInterfaces.size()) return outputInterfaces[step]; }

    void add_output_interface(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step = 0) const;

    void add_output_interface_for_op(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type action_op, StepIndex_type step = 0) const;

#ifdef BUILD_PA
    void share_action_ops(const SuccessorGeneratorAbstract& successor_generator);

    [[nodiscard]] inline const ActionOpAbstract& get_action_op(ActionOpID_type op_id) const {
        PLAJA_ASSERT(actionOps.count(op_id))
        return *actionOps.at(op_id);
    }

    [[nodiscard]] inline const std::list<const ActionOpAbstract*>& get_action_ops(ActionLabel_type label) const {
        PLAJA_ASSERT(actionOpsPerLabel.count(label))
        return actionOpsPerLabel.at(label);
    }

#endif

    const ActionOpToZ3& get_action_op_structure(ActionOpID_type op) const;

    void add_guard(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type op, bool do_locs, StepIndex_type step) const;

    void add_update(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type op, UpdateIndex_type update_index, bool do_locs, bool do_copy, StepIndex_type step) const;

    void add_action_op(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type op, UpdateIndex_type update_index, bool do_locs, bool do_copy, StepIndex_type step) const;

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> guards_to_smt(ActionLabel_type action_label, StepIndex_type step) const;

    void add_guard_disjunction(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const;

    void add_choice_point(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step, bool nn_aware) const;

    void add_loop_exclusion(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step_index) const;

    /** Single step interface. ****************************************************************************************/

    [[nodiscard]] inline const Z3_IN_PLAJA::StateVarsInZ3& get_src_vars() const { return get_state_vars(0); }

    [[nodiscard]] inline const Z3_IN_PLAJA::StateVarsInZ3& get_target_vars() const { return get_state_vars(1); }

    [[nodiscard]] inline const z3::expr& get_src_var(VariableID_type var) const { return get_var(var, 0); }

    [[nodiscard]] inline const z3::expr& get_target_var(VariableID_type var) const { return get_var(var, 1); }

    inline void register_src_bound(Z3_IN_PLAJA::SMTSolver& solver, VariableID_type var) const { register_bound(solver, var, 0); }

    inline void register_target_bound(Z3_IN_PLAJA::SMTSolver& solver, VariableID_type var) const { register_bound(solver, var, 1); }

    inline void register_src_bounds(Z3_IN_PLAJA::SMTSolver& solver, bool ignore_locs) const { register_bounds(solver, 0, ignore_locs); }

    inline void add_src_bounds(Z3_IN_PLAJA::SMTSolver& solver, bool ignore_locs) const { add_bounds(solver, 0, ignore_locs); }

    inline void register_target_bounds(Z3_IN_PLAJA::SMTSolver& solver, bool ignore_locs) const { register_bounds(solver, 1, ignore_locs); }

    inline void register_src_bounds(Z3_IN_PLAJA::SMTSolver& solver, const Expression& expr) const { register_bounds(solver, expr, 0); }

    inline void register_target_bounds(Z3_IN_PLAJA::SMTSolver& solver, const Expression& expr) const { register_bounds(solver, expr, 1); }

    /** Solver interface. *********************************************************************************************/

    [[nodiscard]] std::unique_ptr<PLAJA::SmtSolver> init_solver(const PLAJA::Configuration& config, StepIndex_type step) const final;

    [[nodiscard]] std::unique_ptr<PLAJA::SmtConstraint> to_smt(const Expression& expr, StepIndex_type step) const final;

    void add_to_solver(PLAJA::SmtSolver& solver, const Expression& expr, StepIndex_type step) const final;

};

#endif //PLAJA_MODEL_Z3_H
