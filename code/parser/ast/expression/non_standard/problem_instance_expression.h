//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROBLEM_INSTANCE_EXPRESSION_H
#define PLAJA_PROBLEM_INSTANCE_EXPRESSION_H

#include "../../../../search/using_search.h"
#include "../../iterators/ast_iterator.h"
#include "../forward_expression.h"
#include "../property_expression.h"

class ActionOpTuple: public AstElement {
private:
    ActionOpID_type op;
    UpdateIndex_type update;
public:
    explicit ActionOpTuple(ActionOpID_type op_id = ACTION::noneOp, UpdateIndex_type update_index = ACTION::noneUpdate);
    ~ActionOpTuple() override;
    ActionOpTuple(const ActionOpTuple& other);
    ActionOpTuple(ActionOpTuple&& other); // NOLINT(*-noexcept-move-constructor)
    DELETE_ASSIGNMENT(ActionOpTuple)

    /* Setter. */

    inline void set_op(ActionOpID_type op_id) { op = op_id; }

    inline void set_update(UpdateIndex_type update_index) { update = update_index; }

    /* Getter. */

    [[nodiscard]] inline ActionOpID_type get_op() const { return op; }

    [[nodiscard]] inline UpdateIndex_type get_update() const { return update; }

    /* Override. */

    void accept(AstVisitor* ast_visitor) override;

    void accept(AstVisitorConst* ast_visitor) const override;

};

/**
 * Dump a problem instance in property style.
 * Mostly for debugging purposes.
 */
class ProblemInstanceExpression: public PropertyExpression {

private:
    StepIndex_type targetStep;
    StepIndex_type policyTargetStep; // Some problem instance are only interested in a policy-path-prefix.

    std::vector<std::unique_ptr<ActionOpTuple>> opPath;
    const Expression* start;
    std::unique_ptr<Expression> startPtr;
    const Expression* reach;
    std::unique_ptr<Expression> reachPtr;
    bool includesInit;

    const PredicatesExpression* predicates;
    std::unique_ptr<PredicatesExpression> predicatesPtr;

    std::vector<std::unique_ptr<ArrayValueExpression>> paStatePath; // Abstract state as plain value array.

public:
    ProblemInstanceExpression();
    ~ProblemInstanceExpression() override;
    DELETE_CONSTRUCTOR(ProblemInstanceExpression)

    /* Static. */

    [[nodiscard]] static const std::string& get_op_string();

    /* Construction. */

    void reserve_op_path();

    void add_op_path_step(std::unique_ptr<ActionOpTuple> action_op_tuple);

    inline void add_op_path_step(ActionOpID_type op, UpdateIndex_type update) { add_op_path_step(std::make_unique<ActionOpTuple>(op, update)); }

    inline void add_op_path_step(const ActionOpTuple& action_op_tuple) { add_op_path_step(action_op_tuple.get_op(), action_op_tuple.get_update()); }

    void reserve_pa_path();

    void add_pa_state_path_step(std::unique_ptr<ArrayValueExpression>&& pa_state);

    /* Setter. */

    inline void set_target_step(StepIndex_type target_step) { targetStep = target_step; }

    inline void set_policy_target_step(StepIndex_type target_step) { policyTargetStep = target_step; }

    void set_op_path_step(ActionOpID_type op, UpdateIndex_type update, StepIndex_type step); // NOLINT(*-easily-swappable-parameters)

    inline void set_op_path_step(const ActionOpTuple& action_op_tuple, StepIndex_type step) { set_op_path_step(action_op_tuple.get_op(), action_op_tuple.get_update(), step); }

    void set_start(const Expression* start);

    void set_start(std::unique_ptr<Expression>&& start);

    void set_reach(const Expression* reach);

    void set_reach(std::unique_ptr<Expression>&& reach);

    void set_includes_init(bool includes_init) { includesInit = includes_init; }

    void set_predicates(const PredicatesExpression* predicates);

    void set_predicates(std::unique_ptr<PredicatesExpression>&& predicates);

    void set_pa_state_path_step(std::unique_ptr<ArrayValueExpression>&& pa_state, StepIndex_type step);

    /* Getter. */

    [[nodiscard]] inline StepIndex_type get_target_step() const { return targetStep; }

    [[nodiscard]] inline StepIndex_type get_policy_target_step() const { return policyTargetStep; }

    [[nodiscard]] std::size_t get_op_path_size() const { return opPath.size(); }

    [[nodiscard]] const ActionOpTuple* get_op_path_step(StepIndex_type step) const;

    [[nodiscard]] inline AstConstIterator<ActionOpTuple> init_op_path_it() const { return AstConstIterator(opPath); }

    [[nodiscard]] inline const Expression* get_start() const { return start; }

    [[nodiscard]] inline Expression* get_start_ptr() { return startPtr.get(); }

    [[nodiscard]] inline const Expression* get_reach() const { return reach; }

    [[nodiscard]] inline bool get_includes_init() const { return includesInit; }

    [[nodiscard]] inline Expression* get_reach_ptr() { return reachPtr.get(); }

    [[nodiscard]] inline const PredicatesExpression* get_predicates() const { return predicates; }

    [[nodiscard]] inline PredicatesExpression* get_predicates_ptr() const { return predicatesPtr.get(); }

    [[nodiscard]] std::size_t get_pa_state_path_size() const { return paStatePath.size(); }

    [[nodiscard]] const ArrayValueExpression* get_pa_state_path_step(StepIndex_type step) const;

    [[nodiscard]] inline AstConstIterator<ArrayValueExpression> init_pa_state_path_it() const { return AstConstIterator(paStatePath); }

    [[nodiscard]] inline AstIterator<ArrayValueExpression> init_pa_state_path_it() { return AstIterator(paStatePath); }

    /* Override. */

    void accept(AstVisitor* ast_visitor) override;

    void accept(AstVisitorConst* ast_visitor) const override;

    [[nodiscard]] std::unique_ptr<PropertyExpression> deepCopy_PropExp() const override;

    /**/

    [[nodiscard]] std::unique_ptr<ProblemInstanceExpression> deep_copy() const;

};

#endif //PLAJA_PROBLEM_INSTANCE_EXPRESSION_H
