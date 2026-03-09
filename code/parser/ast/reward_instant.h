//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_REWARD_INSTANT_H
#define PLAJA_REWARD_INSTANT_H

#include <memory>
#include <vector>
#include "ast_element.h"

// forward declaration:
class Expression;
class RewardAccumulation;

class RewardInstant: public AstElement {
private:
    std::unique_ptr<Expression> accumulationExpression;
    std::unique_ptr<RewardAccumulation> rewardAccumulation;
    std::unique_ptr<Expression> instant;

public:
    explicit RewardInstant();
    ~RewardInstant() override;

    // setter:
    void set_accumulation_expression(std::unique_ptr<Expression>&& acc_exp);
    void set_reward_accumulation(std::unique_ptr<RewardAccumulation>&& reward_acc);
    void set_instant(std::unique_ptr<Expression>&& instant_);

    // getter:
    [[nodiscard]] inline Expression* get_accumulation_expression() { return accumulationExpression.get(); }
    [[nodiscard]] inline const Expression* get_accumulation_expression() const { return accumulationExpression.get(); }
    [[nodiscard]] inline const RewardAccumulation* get_reward_accumulation() const { return rewardAccumulation.get(); }
    [[nodiscard]] inline RewardAccumulation* get_reward_accumulation() { return rewardAccumulation.get(); }
    [[nodiscard]] inline Expression* get_instant() { return instant.get(); }
    [[nodiscard]] inline const Expression* get_instant() const { return instant.get(); }

    /**
     * Deep copy of a reward instant.
     * @return
     */
    [[nodiscard]] std::unique_ptr<RewardInstant> deepCopy() const;

    // override:
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
};


#endif //PLAJA_REWARD_INSTANT_H
