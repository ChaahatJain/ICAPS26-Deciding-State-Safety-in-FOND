//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "path_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/bool_type.h"
#include "../property_interval.h"
#include "../reward_bound.h"

PathExpression::PathExpression(PathOp path_op):
    op(path_op)
    , left()
    , right()
    , stepBounds()
    , timeBounds()
    , rewardBounds() {}

PathExpression::~PathExpression() = default;

// static:

namespace PATH_EXPRESSION {
    const std::string pathOpToStr[] { "U", "W", "F", "G", "R" }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, PathExpression::PathOp> strToPathOp { { "U", PathExpression::UNTIL },
                                                                                { "W", PathExpression::WEAK_UNTIL },
                                                                                { "F", PathExpression::EVENTUALLY },
                                                                                { "G", PathExpression::ALWAYS },
                                                                                { "R", PathExpression::RELEASE } }; // NOLINT(cert-err58-cpp)
}

const std::string& PathExpression::path_op_to_str(PathExpression::PathOp op) { return PATH_EXPRESSION::pathOpToStr[op]; }

std::unique_ptr<PathExpression::PathOp> PathExpression::str_to_path_op(const std::string& op_str) {
    auto it = PATH_EXPRESSION::strToPathOp.find(op_str);
    if (it == PATH_EXPRESSION::strToPathOp.end()) { return nullptr; }
    else { return std::make_unique<PathOp>(it->second); }
}

bool PathExpression::is_binary(PathOp path_op) {
    switch (path_op) {
        case PathOp::UNTIL:
        case PathOp::WEAK_UNTIL:
        case PathOp::RELEASE: return true;
            // EVENTUALLY,ALWAYS
        default: return false;
    }
}

// construction:

void PathExpression::reserve(std::size_t reward_bounds_cap) {
    rewardBounds.reserve(reward_bounds_cap);
}

void PathExpression::add_rewardBound(std::unique_ptr<RewardBound>&& reward_bound) {
    rewardBounds.push_back(std::move(reward_bound));
}

// setter:

void PathExpression::set_step_bounds(std::unique_ptr<PropertyInterval>&& step_bounds) { stepBounds = std::move(step_bounds); }

void PathExpression::set_time_bounds(std::unique_ptr<PropertyInterval>&& time_bounds) { timeBounds = std::move(time_bounds); }

[[maybe_unused]] void PathExpression::set_reward_bound(std::unique_ptr<RewardBound>&& reward_bound, std::size_t index) {
    PLAJA_ASSERT(index < rewardBounds.size())
    rewardBounds[index] = std::move(reward_bound);
}

// override:

const DeclarationType* PathExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        resultType = std::make_unique<BoolType>();
        return resultType.get();
    }
}

void PathExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void PathExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<PropertyExpression> PathExpression::deepCopy_PropExp() const {
    std::unique_ptr<PathExpression> copy(new PathExpression(op));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    if (left) { copy->set_left(left->deepCopy_PropExp()); }
    if (right) { copy->set_right(right->deepCopy_PropExp()); }
    if (stepBounds) { copy->set_step_bounds(stepBounds->deepCopy()); }
    if (timeBounds) { copy->set_time_bounds(timeBounds->deepCopy()); }
    copy->reserve(rewardBounds.size());
    for (const auto& reward_bound: rewardBounds) { copy->add_rewardBound(reward_bound->deepCopy()); }
    return copy;
}






