//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "reward_bound.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "expression/expression.h"
#include "non_standard/reward_accumulation.h"
#include "property_interval.h"


// instance :

RewardBound::RewardBound(): accumulationExpression(), rewardAccumulation(), bounds() {}

RewardBound::~RewardBound() = default;

// setter:

void RewardBound::set_accumulation_expression(std::unique_ptr<Expression>&& acc_exp) { accumulationExpression = std::move(acc_exp); }

void RewardBound::set_reward_accumulation(std::unique_ptr<RewardAccumulation>&& reward_acc) { rewardAccumulation = std::move(reward_acc); }

void RewardBound::set_bounds(std::unique_ptr<PropertyInterval> bounds_) { bounds = std::move(bounds_); }

//

std::unique_ptr<RewardBound> RewardBound::deepCopy() const {
    std::unique_ptr<RewardBound> copy(new RewardBound());
    if (accumulationExpression) copy->accumulationExpression = accumulationExpression->deepCopy_Exp();
    if (rewardAccumulation) { copy->rewardAccumulation = rewardAccumulation->deepCopy(); }
    if (bounds) copy->bounds = bounds->deepCopy();
    return copy;
}


// override:

void RewardBound::accept(AstVisitor* astVisitor) { astVisitor->visit(this); }

void RewardBound::accept(AstVisitorConst* astVisitor) const { astVisitor->visit(this); }
