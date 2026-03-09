//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_MODEL_VERITAS_H
#define PLAJA_MODEL_VERITAS_H

#include <list>
#include <memory>
#include <vector>
#include "../../../include/ct_config_const.h"
#include "../../information/forward_information.h"
#include "../../smt/base/model_smt.h"
#include "../../using_search.h"
#include "../forward_smt_veritas.h"
#include "../using_veritas.h"
#include "../veritas_context.h"
#include "../vars_in_veritas.h"
#include "../veritas_query.h"

class StateIndexesInVeritas;

enum FilterType {None, Indicator, Naive};

class ModelVeritas: public ModelSmt {

protected:
    VERITAS_IN_PLAJA::Context* context;

    std::vector<std::unique_ptr<StateIndexesInVeritas>> stateIndexesInVeritas;
    std::vector<std::pair<VeritasVarIndex_type, VeritasVarIndex_type>> maxVarPerStep; // Latter is with target vars.

    std::unique_ptr<VeritasConstraint> startVeritas;
    // std::unique_ptr<VeritasConstraint> startVeritasWithInit; // Rewriting startVeritas as a disjunction is too expensive on our benchmarks.
    std::vector<std::unique_ptr<VeritasConstraint>> reachVeritas;

    std::vector<std::unique_ptr<VERITAS_IN_PLAJA::VeritasQuery>> ensembleInVeritasPerStep;
#ifdef ENABLE_APPLICABILITY_FILTERING
    std::vector<veritas::AddTree> applicabilityTrees;
    veritas::AddTree indicatorTrees = veritas::AddTree(1, veritas::AddTreeType::REGR);
    FilterType filter = None;
#endif

    /* Ops. */
    std::unique_ptr<VERITAS_IN_PLAJA::SuccessorGenerator> successorGenerator;

    void init_output_constraints();

    void generate_state_indexes(StateIndexesInVeritas& state_indexes, bool do_locs, bool mark_input) const;

    [[nodiscard]] std::vector<VeritasVarIndex_type> extract_ensemble_inputs(const StateIndexesInVeritas& state_indexes) const; // Ensemble after state indexes

    void match_state_vars_to_ensemble(const VERITAS_IN_PLAJA::VeritasQuery& ensemble_in_veritas, StateIndexesInVeritas& state_indexes) const; // state indexes after Ensemble


public:
    explicit ModelVeritas(const PLAJA::Configuration& config, bool init_suc_gen = true);
    ~ModelVeritas() override;
    DELETE_CONSTRUCTOR(ModelVeritas)

    /******************************************************************************************************************/

    [[nodiscard]] inline std::shared_ptr<VERITAS_IN_PLAJA::Context> share_context() const { return PLAJA_UTILS::cast_shared<VERITAS_IN_PLAJA::Context>(get_context_ptr()); }

    [[nodiscard]] inline VERITAS_IN_PLAJA::Context& get_context() const { return *context; }

    /******************************************************************************************************************/

    void set_start(std::unique_ptr<Expression>&& start) override;

    void set_reach(std::unique_ptr<Expression>&& reach) override;

    /******************************************************************************************************************/

#ifdef ENABLE_APPLICABILITY_FILTERING
    void set_filter(FilterType filt) { filter = filt; }
    bool no_filter() const { return filter == FilterType::None; }
    bool indicator_filter() const { return filter == FilterType::Indicator; }
    bool naive_filter() const { return filter == FilterType::Naive; }
#endif 

    /******************************************************************************************************************/

    void generate_steps(StepIndex_type max_step) override;

    [[nodiscard]] inline const StateIndexesInVeritas& get_state_indexes(StepIndex_type step = 0) const {
        PLAJA_ASSERT(step < stateIndexesInVeritas.size())
        return *stateIndexesInVeritas[step];
    }

    [[nodiscard]] inline StateIndexesInVeritas& get_state_indexes(StepIndex_type step = 0) {
        PLAJA_ASSERT(step < stateIndexesInVeritas.size())
        return *stateIndexesInVeritas[step];
    }

    /* Start & reach. */

    void add_start(VERITAS_IN_PLAJA::QueryConstructable& query) const;

    void add_init(VERITAS_IN_PLAJA::QueryConstructable& query, StepIndex_type step = 0) const;

    void exclude_start(VERITAS_IN_PLAJA::QueryConstructable& query, bool include_init, StepIndex_type step = 0) const;

    void add_reach(VERITAS_IN_PLAJA::QueryConstructable& query, StepIndex_type step = 0) const;

    void exclude_reach(VERITAS_IN_PLAJA::QueryConstructable& query, StepIndex_type step = 0) const;

    void add_applicability_trees() { init_output_constraints(); }

    /* Ensemble */

    [[nodiscard]] inline const VERITAS_IN_PLAJA::VeritasQuery& get_query_in_veritas(StepIndex_type step = 0) const {
        PLAJA_ASSERT(step < stateIndexesInVeritas.size())
        return *ensembleInVeritasPerStep[step];
    }

    veritas::AddTree get_policy_in_query() const {
        return ensembleInVeritasPerStep[0]->get_input_query();
    }

    [[nodiscard]] std::unique_ptr<VERITAS_IN_PLAJA::VeritasQuery> make_query(bool add_nn, bool add_target_vars, StepIndex_type step = 0) const;

    void add_policy_to_query(VERITAS_IN_PLAJA::VeritasQuery& query, StepIndex_type step = 0) const;

    veritas::AddTree get_applicability_filter(ActionLabel_type action_label); // This is primary action class. Add trees for all labels except this one

    /* action op */

    [[nodiscard]] const std::vector<const ActionOpVeritas*>& get_action_structure(ActionLabel_type label) const;

    [[nodiscard]] const ActionOpVeritas& get_action_op(ActionOpID_type op_id) const;

    VeritasVarIndex_type get_operator_aux_var(ActionOpID_type op) const { return context->get_aux_var(op); }

    void add_guard(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, bool do_locs, StepIndex_type step = 0) const;

    void add_update(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step = 0) const;

    void add_action_op(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step = 0) const;

    [[nodiscard]] std::unique_ptr<VeritasConstraint> guards_to_veritas(ActionLabel_type action_label, StepIndex_type step = 0) const;

    void add_guard_disjunction(VERITAS_IN_PLAJA::QueryConstructable& query, ActionLabel_type action_label, StepIndex_type step = 0) const;

    void add_choice_point(VERITAS_IN_PLAJA::QueryConstructable& query, bool nn_aware, StepIndex_type step) const;
    /* aux */

    [[nodiscard]] std::unique_ptr<VeritasConstraint> to_veritas(const Expression& expr, StepIndex_type step_index) const;

    void add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, const Expression& expr, StepIndex_type step_index) const;

    /* single step interface */
    [[nodiscard]] inline const StateIndexesInVeritas& get_src_in_veritas() const { return get_state_indexes(0); }

    /** Solver interface. *********************************************************************************************/
    [[nodiscard]] std::unique_ptr<PLAJA::SmtSolver> init_solver(const PLAJA::Configuration& config, StepIndex_type step) const final;

    [[nodiscard]] std::unique_ptr<PLAJA::SmtConstraint> to_smt(const Expression& expr, StepIndex_type step) const final;


    void add_to_solver(PLAJA::SmtSolver& solver, const Expression& expr, StepIndex_type step_index) const final;
};

#endif //PLAJA_MODEL_VERITAS_H
