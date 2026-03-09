//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_REWARD_ACCUMULATION_H
#define PLAJA_REWARD_ACCUMULATION_H

#include <memory>
#include <vector>
#include "../ast_element.h"

/**
 * In JANI standard this is not an object but an array.
 */
class RewardAccumulation final: public AstElement {

public:
    enum RewardAccValue { STEPS, TIME, EXIT };

private:
    std::vector<RewardAccValue> rewardAccumulation;

    RewardAccumulation(const RewardAccumulation& reward_accumulation);
public:
    RewardAccumulation();
    ~RewardAccumulation() override;

    // static:
    static const std::string& reward_acc_value_to_str(RewardAccValue reward_acc_value);
    static std::unique_ptr<RewardAccValue> str_to_reward_acc_value(const std::string& reward_acc_str); // nullptr if invalid string

    // construction:
    inline void reserve(std::size_t cap) { rewardAccumulation.reserve(cap); }
    inline void add_reward_accumulation_value(RewardAccValue reward_acc_val) { PLAJA_ASSERT(not accumulate(reward_acc_val)) rewardAccumulation.push_back(reward_acc_val); }

    // setter:
    inline void set_reward_acc_value(RewardAccValue reward_acc_val, std::size_t index) { PLAJA_ASSERT(index < rewardAccumulation.size()) rewardAccumulation[index] = reward_acc_val; }

    // getter:
    [[nodiscard]] inline std::size_t size() const { return rewardAccumulation.size(); }
    [[nodiscard]] inline RewardAccValue get_reward_acc_value(std::size_t index) const { PLAJA_ASSERT(index < rewardAccumulation.size()) return rewardAccumulation[index]; }
    [[nodiscard]] inline const std::vector<RewardAccValue>& _reward_accumulation() const { return rewardAccumulation; }

    // auxiliary:
    [[nodiscard]] bool accumulate(RewardAccValue reward_acc_value) const;
    [[nodiscard]] inline bool accumulate_steps() const { return accumulate(RewardAccValue::STEPS); }
    [[nodiscard]] inline bool accumulate_time() const { return accumulate(RewardAccValue::TIME); }
    [[nodiscard]] inline bool accumulate_exit() const { return accumulate(RewardAccValue::EXIT); }

    // override:
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<RewardAccumulation> deepCopy() const { return std::unique_ptr<RewardAccumulation>(new RewardAccumulation(*this)); }

};

#endif //PLAJA_REWARD_ACCUMULATION_H
