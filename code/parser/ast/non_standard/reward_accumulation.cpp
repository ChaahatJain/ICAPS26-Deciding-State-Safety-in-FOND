//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "reward_accumulation.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"

RewardAccumulation::RewardAccumulation(const RewardAccumulation& reward_accumulation): rewardAccumulation(reward_accumulation.rewardAccumulation) {}

RewardAccumulation::RewardAccumulation() = default;

RewardAccumulation::~RewardAccumulation() = default;

// static:

namespace JANI { // only used locally
    const std::string STEPS = "steps";
    const std::string TIME = "time";
    const std::string EXIT = "exit";
}

const std::string& RewardAccumulation::reward_acc_value_to_str(RewardAccumulation::RewardAccValue reward_acc_value) {
    switch (reward_acc_value) {
        case STEPS: { return JANI::STEPS; }
        case TIME: { return JANI::TIME; }
        case EXIT: { return JANI::EXIT; }
        default: PLAJA_ABORT
    }
    PLAJA_ABORT
}

std::unique_ptr<RewardAccumulation::RewardAccValue> RewardAccumulation::str_to_reward_acc_value(const std::string& reward_acc_str) {
    if (reward_acc_str == JANI::STEPS) { return std::make_unique<RewardAccValue>(RewardAccValue::STEPS); }
    if (reward_acc_str == JANI::TIME) { return std::make_unique<RewardAccValue>(RewardAccValue::TIME); }
    if (reward_acc_str == JANI::EXIT) { return std::make_unique<RewardAccValue>(RewardAccValue::EXIT); }
    return nullptr;
}

// auxiliary:

bool RewardAccumulation::accumulate(RewardAccumulation::RewardAccValue reward_acc_value) const {
    return std::any_of(rewardAccumulation.cbegin(), rewardAccumulation.cend(), [reward_acc_value](RewardAccValue acc_value){ return acc_value == reward_acc_value; } );
}

// override:

void RewardAccumulation::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void RewardAccumulation::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }