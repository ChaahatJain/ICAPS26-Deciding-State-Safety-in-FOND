//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "expectation_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../expression/expression.h"
#include "../non_standard/reward_accumulation.h"
#include "../type/real_type.h"
#include "../reward_instant.h"

ExpectationExpression::ExpectationExpression(ExpectationQualifier exp_op):
    op(exp_op)
    , value()
    , rewardAccumulation()
    , reach()
    , stepInstant()
    , timeInstant()
    , rewardInstants() {
}

ExpectationExpression::~ExpectationExpression() = default;

// static:

namespace EXPECTATION_EXPRESSION {
    const std::string expQualifierToStr[] { "Emin", "Emax" }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, ExpectationExpression::ExpectationQualifier> strToExpQualifier { { "Emin", ExpectationExpression::EMIN },
                                                                                                           { "Emax", ExpectationExpression::EMAX } }; // NOLINT(cert-err58-cpp)
}

const std::string& ExpectationExpression::exp_qualifier_to_str(ExpectationExpression::ExpectationQualifier qualifier) { return EXPECTATION_EXPRESSION::expQualifierToStr[qualifier]; }

std::unique_ptr<ExpectationExpression::ExpectationQualifier> ExpectationExpression::str_to_exp_qualifier(const std::string& qualifier_str) {
    auto it = EXPECTATION_EXPRESSION::strToExpQualifier.find(qualifier_str);
    if (it == EXPECTATION_EXPRESSION::strToExpQualifier.end()) { return nullptr; }
    else { return std::make_unique<ExpectationQualifier>(it->second); }
}

// construction:

void ExpectationExpression::reserve(std::size_t reward_instants_cap) { rewardInstants.reserve(reward_instants_cap); }

void ExpectationExpression::add_reward_instant(std::unique_ptr<RewardInstant>&& reward_instant) { rewardInstants.push_back(std::move(reward_instant)); }

// setter:

void ExpectationExpression::set_value(std::unique_ptr<Expression> val) { value = std::move(val); }

void ExpectationExpression::set_reward_accumulation(std::unique_ptr<RewardAccumulation>&& reward_acc) { rewardAccumulation = std::move(reward_acc); }

void ExpectationExpression::set_step_instant(std::unique_ptr<Expression>&& step_inst) { stepInstant = std::move(step_inst); }

void ExpectationExpression::set_time_instant(std::unique_ptr<Expression>&& time_inst) { timeInstant = std::move(time_inst); }

[[maybe_unused]] void ExpectationExpression::set_reward_instant(std::unique_ptr<RewardInstant>&& reward_inst, std::size_t index) {
    PLAJA_ASSERT(index < rewardInstants.size())
    rewardInstants[index] = std::move(reward_inst);
}

// override:

const DeclarationType* ExpectationExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        resultType = std::make_unique<RealType>();
        return resultType.get();
    }
}

void ExpectationExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ExpectationExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<PropertyExpression> ExpectationExpression::deepCopy_PropExp() const {
    std::unique_ptr<ExpectationExpression> copy(new ExpectationExpression(op));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    if (value) { copy->set_value(value->deepCopy_Exp()); }
    if (rewardAccumulation) { copy->set_reward_accumulation(rewardAccumulation->deepCopy()); }
    if (reach) { copy->set_reach(reach->deepCopy_PropExp()); }
    if (stepInstant) { copy->set_step_instant(stepInstant->deepCopy_Exp()); }
    if (timeInstant) { copy->set_time_instant(timeInstant->deepCopy_Exp()); }
    copy->reserve(rewardInstants.size());
    for (const auto& reward_instant: rewardInstants) { copy->add_reward_instant(reward_instant->deepCopy()); }
    return copy;
}







