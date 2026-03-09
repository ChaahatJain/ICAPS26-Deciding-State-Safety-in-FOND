//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OBJECTIVE_EXPRESSION_H
#define PLAJA_OBJECTIVE_EXPRESSION_H

#include "../../forward_ast.h"
#include "../forward_expression.h"
#include "../expression.h"

class ObjectiveExpression: public PropertyExpression {

private:
    std::unique_ptr<Expression> goal;
    // Reward/cost:
    std::unique_ptr<Expression> goalPotential;
    std::unique_ptr<Expression> stepReward;
    std::unique_ptr<RewardAccumulation> rewardAccumulation;

public:
    ObjectiveExpression();
    ~ObjectiveExpression() override;
    DELETE_CONSTRUCTOR(ObjectiveExpression)

    [[nodiscard]] std::unique_ptr<ObjectiveExpression> deep_copy() const;

    /* Static. */

    [[nodiscard]] static const std::string& get_op_string();

    /* Setter. */

    inline void set_goal(std::unique_ptr<Expression>&& goal_r) { goal = std::move(goal_r); }

    inline void set_goal_potential(std::unique_ptr<Expression>&& goal_potential) { goalPotential = std::move(goal_potential); }

    inline void set_step_reward(std::unique_ptr<Expression>&& step_reward) { stepReward = std::move(step_reward); }

    void set_reward_accumulation(std::unique_ptr<RewardAccumulation>&& reward_acc);

    /* Getter */

    [[nodiscard]] inline const Expression* get_goal() const { return goal.get(); }

    [[nodiscard]] inline Expression* get_goal() { return goal.get(); }

    [[nodiscard]] inline const Expression* get_goal_potential() const { return goalPotential.get(); }

    [[nodiscard]] inline Expression* get_goal_potential() { return goalPotential.get(); }

    [[nodiscard]] inline const Expression* get_step_reward() const { return stepReward.get(); }

    [[nodiscard]] inline Expression* get_step_reward() { return stepReward.get(); }

    [[nodiscard]] inline const RewardAccumulation* get_reward_accumulation() const { return rewardAccumulation.get(); }

    [[nodiscard]] inline RewardAccumulation* get_reward_accumulation() { return rewardAccumulation.get(); }

    /* Auxiliary. */
    PLAJA::floating compute_step_reward(const StateBase* source, const StateBase* successor) const;

    /* Override. */
    [[nodiscard]] std::unique_ptr<PropertyExpression> deepCopy_PropExp() const override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_OBJECTIVE_EXPRESSION_H
