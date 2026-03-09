//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PATH_EXPRESSION_H
#define PLAJA_PATH_EXPRESSION_H

#include "property_expression.h"
#include "../iterators/ast_iterator.h"

// forward declaration
class PropertyInterval;
class RewardBound;


class PathExpression final: public PropertyExpression {
public:
    enum PathOp { UNTIL, WEAK_UNTIL, EVENTUALLY, ALWAYS, RELEASE };
private:
    PathOp op;
    std::unique_ptr<PropertyExpression> left;     // <- none for EVENTUALLY, ALWAYS
    std::unique_ptr<PropertyExpression> right;    // <- exp for EVENTUALLY, ALWAYS
    std::unique_ptr<PropertyInterval> stepBounds;
    std::unique_ptr<PropertyInterval> timeBounds;
    std::vector<std::unique_ptr<RewardBound>> rewardBounds;
public:
    explicit PathExpression(PathOp path_op);
    ~PathExpression() override;

    // static:
    static const std::string& path_op_to_str(PathOp op);
    static std::unique_ptr<PathOp> str_to_path_op(const std::string& op_str);
    static bool is_binary(PathOp path_op);

    // construction:
    void reserve(std::size_t reward_bounds_cap);
    void add_rewardBound(std::unique_ptr<RewardBound>&& reward_bound);

    // setter:
    [[maybe_unused]] inline void set_op(PathOp path_op) { op = path_op; }
    inline void set_left(std::unique_ptr<PropertyExpression>&& left_) { left = std::move(left_); }
    inline void set_right(std::unique_ptr<PropertyExpression>&& right_) { right = std::move(right_); }
    void set_step_bounds(std::unique_ptr<PropertyInterval>&& step_bounds);
    void set_time_bounds(std::unique_ptr<PropertyInterval>&& time_bounds);

    [[maybe_unused]] void set_reward_bound(std::unique_ptr<RewardBound>&& reward_bound, std::size_t index);

    // getter:
    [[nodiscard]] inline PathOp get_op() const { return op; }
    [[nodiscard]] inline PropertyExpression* get_left() { return left.get(); }
    [[nodiscard]] inline const PropertyExpression* get_left() const { return left.get(); }
    [[nodiscard]] inline PropertyExpression* get_right() { return right.get(); }
    [[nodiscard]] inline const PropertyExpression* get_right() const { return right.get();}
    [[nodiscard]] inline PropertyInterval* get_step_bounds() { return stepBounds.get(); }
    [[nodiscard]] inline const PropertyInterval* get_step_bounds() const { return stepBounds.get(); }
    [[nodiscard]] inline PropertyInterval* get_time_bounds() { return timeBounds.get(); }
    [[nodiscard]] inline const PropertyInterval* get_time_bounds() const { return timeBounds.get(); }
    [[nodiscard]] inline std::size_t get_number_reward_bounds() const { return rewardBounds.size(); }
    [[nodiscard]] inline RewardBound* get_reward_bound(std::size_t index) { PLAJA_ASSERT(index < rewardBounds.size()) return rewardBounds[index].get(); }
    [[nodiscard]] inline const RewardBound* get_reward_bound(std::size_t index) const { PLAJA_ASSERT(index < rewardBounds.size()) return rewardBounds[index].get(); }
    [[nodiscard]] inline AstConstIterator<RewardBound> rewardBoundIterator() const { return AstConstIterator(rewardBounds); }
    [[nodiscard]] inline AstIterator<RewardBound> rewardBoundIterator() { return AstIterator(rewardBounds); }

    // override:
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<PropertyExpression> deepCopy_PropExp() const override;
};


#endif //PLAJA_PATH_EXPRESSION_H
