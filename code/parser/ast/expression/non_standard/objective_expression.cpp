//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "objective_expression.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../non_standard/reward_accumulation.h"
#include "../../type/declaration_type.h"

/* Static. */

const std::string& ObjectiveExpression::get_op_string() {
    static const std::string op_string = "objective";
    return op_string;
}

/**/

ObjectiveExpression::ObjectiveExpression() = default;

ObjectiveExpression::~ObjectiveExpression() = default;

/* Setter. */

void ObjectiveExpression::set_reward_accumulation(std::unique_ptr<RewardAccumulation>&& reward_acc) { rewardAccumulation = std::move(reward_acc); }

/* Auxiliary. */

PLAJA::floating ObjectiveExpression::compute_step_reward(const StateBase* source, const StateBase* successor) const {
    PLAJA::floating reward = 0;
    if (stepReward) {
        PLAJA_ASSERT(rewardAccumulation)
        PLAJA_ASSERT(not rewardAccumulation->accumulate_time())
        if (rewardAccumulation->accumulate_exit()) { reward += stepReward->evaluateFloating(source); }
        if (rewardAccumulation->accumulate_steps()) { reward += stepReward->evaluateFloating(successor); }
    }
    return reward;
}

// override:

void ObjectiveExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ObjectiveExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<PropertyExpression> ObjectiveExpression::deepCopy_PropExp() const { return deep_copy(); }

//

std::unique_ptr<ObjectiveExpression> ObjectiveExpression::deep_copy() const {
    std::unique_ptr<ObjectiveExpression> copy(new ObjectiveExpression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    // misc:
    if (goal) { copy->set_goal(goal->deepCopy_Exp()); }
    if (goalPotential) { copy->set_goal_potential(goalPotential->deepCopy_Exp()); }
    if (stepReward) { copy->set_step_reward(stepReward->deepCopy_Exp()); }
    if (rewardAccumulation) { copy->set_reward_accumulation(rewardAccumulation->deepCopy()); }
    return copy;
}
