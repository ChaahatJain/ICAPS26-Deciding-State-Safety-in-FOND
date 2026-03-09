//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "reward_instant.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "expression/expression.h"
#include "non_standard/reward_accumulation.h"


// class:

RewardInstant::RewardInstant(): accumulationExpression(), rewardAccumulation(), instant() {}

RewardInstant::~RewardInstant() = default;

// setter:

void RewardInstant::set_accumulation_expression(std::unique_ptr<Expression>&& acc_exp) { accumulationExpression = std::move(acc_exp); }

void RewardInstant::set_reward_accumulation(std::unique_ptr<RewardAccumulation>&& reward_acc) { rewardAccumulation = std::move(reward_acc); }

void RewardInstant::set_instant(std::unique_ptr<Expression>&& instant_) { instant = std::move(instant_); }

//

std::unique_ptr<RewardInstant> RewardInstant::deepCopy() const {
    std::unique_ptr<RewardInstant> copy(new RewardInstant());
    if (accumulationExpression) copy->accumulationExpression = accumulationExpression->deepCopy_Exp();
    if (rewardAccumulation) { copy->rewardAccumulation = rewardAccumulation->deepCopy(); }
    if (instant) copy->instant = instant->deepCopy_Exp();
    return copy;
}

// override:

void RewardInstant::accept(AstVisitor* astVisitor) { astVisitor->visit(this); }

void RewardInstant::accept(AstVisitorConst* astVisitor) const { astVisitor->visit(this); }


