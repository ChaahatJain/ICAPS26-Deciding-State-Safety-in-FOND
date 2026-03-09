//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPECTATION_EXPRESSION_H
#define PLAJA_EXPECTATION_EXPRESSION_H


#include "property_expression.h"
#include "../iterators/ast_iterator.h"

// forward declaration
class Expression;
class RewardInstant;
class RewardAccumulation;


class ExpectationExpression final: public PropertyExpression {

public:
    enum ExpectationQualifier { EMIN, EMAX };

private:
    ExpectationQualifier op;
    std::unique_ptr<Expression> value;
    std::unique_ptr<RewardAccumulation> rewardAccumulation;
    std::unique_ptr<PropertyExpression> reach;
    std::unique_ptr<Expression> stepInstant;
    std::unique_ptr<Expression> timeInstant;
    std::vector<std::unique_ptr<RewardInstant>> rewardInstants;

public:
    explicit ExpectationExpression(ExpectationQualifier exp_op);
    ~ExpectationExpression() override;

    // static:
    static const std::string& exp_qualifier_to_str(ExpectationQualifier qualifier);
    static std::unique_ptr<ExpectationQualifier> str_to_exp_qualifier(const std::string& qualifier_str);

    // construction:
    void reserve(std::size_t reward_instants_cap);
    void add_reward_instant(std::unique_ptr<RewardInstant>&& reward_instant);

    // setter:
    [[maybe_unused]] inline void set_op(ExpectationQualifier exp_op) { op = exp_op; }
    void set_value(std::unique_ptr<Expression> val);
    inline void set_reach(std::unique_ptr<PropertyExpression> reach_) { reach = std::move(reach_); }
    void set_reward_accumulation(std::unique_ptr<RewardAccumulation>&& reward_acc);
    void set_step_instant(std::unique_ptr<Expression>&& step_inst);
    void set_time_instant(std::unique_ptr<Expression>&& time_inst);
    void set_reward_instant(std::unique_ptr<RewardInstant>&& reward_instant, std::size_t index);

    // getter:
    [[nodiscard]] inline ExpectationQualifier get_op() const { return op; }
    [[nodiscard]] inline Expression* get_value() { return value.get(); }
    [[nodiscard]] inline const Expression* get_value() const { return value.get(); }
    [[nodiscard]] inline PropertyExpression* get_reach() { return reach.get(); }
    [[nodiscard]] inline const PropertyExpression* get_reach() const { return reach.get(); }
    [[nodiscard]] inline const RewardAccumulation* get_reward_accumulation() const { return rewardAccumulation.get(); }
    [[nodiscard]] inline RewardAccumulation* get_reward_accumulation() { return rewardAccumulation.get(); }
    [[nodiscard]] inline Expression* get_step_instant() { return stepInstant.get(); }
    [[nodiscard]] inline const Expression* get_step_instant() const { return stepInstant.get(); }
    [[nodiscard]] inline Expression* get_time_instant() { return timeInstant.get(); }
    [[nodiscard]] inline const Expression* get_time_instant() const { return timeInstant.get(); }
    [[nodiscard]] inline std::size_t get_number_reward_instants() const { return rewardInstants.size(); }
    [[nodiscard]] inline RewardInstant* get_reward_instant(std::size_t index) { PLAJA_ASSERT(index < rewardInstants.size()) return rewardInstants[index].get(); }
    [[nodiscard]] inline const RewardInstant* get_reward_instant(std::size_t index) const { PLAJA_ASSERT(index < rewardInstants.size()) return rewardInstants[index].get(); }
    [[nodiscard]] inline AstConstIterator<RewardInstant> rewardInstantIterator() const { return AstConstIterator(rewardInstants); }
    [[nodiscard]] inline AstIterator<RewardInstant> rewardInstantIterator() { return AstIterator(rewardInstants); }

    // override:
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<PropertyExpression> deepCopy_PropExp() const override;
};


#endif //PLAJA_EXPECTATION_EXPRESSION_H
