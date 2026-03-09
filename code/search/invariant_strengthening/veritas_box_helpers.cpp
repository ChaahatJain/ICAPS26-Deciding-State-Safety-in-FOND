#include "veritas_box_helpers.h"
#include "../smt_ensemble/using_veritas.h"
#include "../../parser/ast/expression/expression_utils.h"
#include "../../parser/ast/expression/lvalue_expression.h"
#include "../../parser/ast/expression/special_cases/nary_expression.h"
#include "../../parser/ast/expression/special_cases/linear_expression.h"
#include "../../parser/ast/expression/array_value_expression.h"
#include "../../parser/ast/expression/array_access_expression.h"
#include "../../parser/ast/expression/binary_op_expression.h"
#include "../../parser/ast/expression/constant_value_expression.h"
#include "../../parser/ast/expression/unary_op_expression.h"
#include "../../parser/ast/expression/variable_expression.h"
#include "../../utils/floating_utils.h"
#include "../../utils/utils.h"
#include <set>
#include <algorithm>


// Custom equality operator for comparing vectors of vectors
bool areBoxesEqual(const std::vector<veritas::Interval>& box_a, const std::vector<veritas::Interval>& box_b) {
    for (int i = 0; i < box_a.size(); ++i) {
        auto bound1 = box_a[i]; auto bound2 = box_b[i];
        if (bound1.lo != bound2.lo or bound1.hi != bound2.hi) {
            return false;
        }
    }
    return true;
}

veritas::AddTree boxTree(const std::vector<veritas::Interval>& box, int num_actions) {
    veritas::AddTree trees(num_actions, veritas::AddTreeType::REGR);
    veritas::Tree tree(num_actions);
    std::string prefix = "";
    for (int i = 0; i < box.size(); ++i) {
        auto b = box[i];
        auto lb = b.lo; auto ub = b.hi; 
        tree.split(tree[prefix.c_str()], {i, lb});
        tree.split(tree[(prefix + "r").c_str()], {i, ub});
        for (int j = 0; j < num_actions; ++j) {
            tree.leaf_value(tree[(prefix + "l").c_str()], j) = 0;
            tree.leaf_value(tree[(prefix + "rr").c_str()], j) = 0;
        }
        prefix += "rl";
    }
    for (int j = 0; j < num_actions; ++j) {
        tree.leaf_value(tree[prefix.c_str()], j) = VERITAS_IN_PLAJA::negative_infinity;
    }
    trees.add_tree(tree);
    return trees;
}

std::unique_ptr<Expression> box_to_condition(const Model& model, const std::vector<veritas::Interval>& box) {

    const auto& model_info = model.get_model_information();

    auto var_condition = std::make_unique<NaryExpression>(BinaryOpExpression::AND);
    var_condition->reserve(2*box.size());

    for (auto it = model_info.stateIndexIteratorInt(false); !it.end(); ++it) {
        const auto state_index = it.state_index();
        auto var_expr = model.gen_var_expr(state_index, nullptr);
        auto bounds = box[state_index - 1];
        auto lb = bounds.lo; auto ub = bounds.hi;
        if (PLAJA_UTILS::cast_ref<LValueExpression>(*var_expr).is_boolean()) {
            if (lb == 1) { var_condition->add_sub(std::move(var_expr)); }
            else {
                auto not_var = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT);
                not_var->set_operand(std::move(var_expr));
                var_condition->add_sub(std::move(not_var));
            }
        } else {
            var_condition->add_sub(LinearExpression::construct_bound(std::move(var_expr), PLAJA_EXPRESSION::make_int(lb), BinaryOpExpression::GE));
            var_expr = model.gen_var_expr(state_index, nullptr);
            var_condition->add_sub(LinearExpression::construct_bound(std::move(var_expr), PLAJA_EXPRESSION::make_int(ub), BinaryOpExpression::LT));
        }
    }

    for (auto it = model_info.stateIndexIteratorFloat(); !it.end(); ++it) {
        const auto state_index = it.state_index();
        auto var_expr = model.gen_var_expr(state_index, nullptr);
        auto bounds = box[state_index - 1];
        auto lb = bounds.lo; auto ub = bounds.hi;
        var_condition->add_sub(LinearExpression::construct_bound(std::move(var_expr), PLAJA_EXPRESSION::make_real(lb), BinaryOpExpression::GE));
        var_expr = model.gen_var_expr(state_index, nullptr);
        var_condition->add_sub(LinearExpression::construct_bound(std::move(var_expr), PLAJA_EXPRESSION::make_real(ub), BinaryOpExpression::LT));       
    }
    return var_condition;
}

int get_union_index(const std::vector<veritas::Interval>& box1, const std::vector<veritas::Interval>& box2) {
    int index = -1;
    for (auto i = 0; i < box1.size(); ++i) {
        auto bound1 = box1[i];
        auto bound2 = box2[i];
        auto lb1 = bound1.lo; auto ub1 = bound1.hi;
        auto lb2 = bound2.lo; auto ub2 = bound2.hi;
        if (ub1 < lb2 or ub2 < lb1) {
            return -1;
        }
        if ((lb2 <= lb1 and lb1 <= ub2) or (lb1 <= lb2 and lb2 <= ub1)) {
            if (lb1 == lb2 and ub1 == ub2) {
                index = i;
                continue;
            }
            return check_exact_overlap(box1, box2, i);
        } 
    }
    return index;
}

int check_exact_overlap(const std::vector<veritas::Interval>& box1, const std::vector<veritas::Interval>& box2, const int union_index) {
    for (auto i = 0; i < box1.size(); ++i) {
        if (i == union_index) continue;
        auto bound1 = box1[i];
        auto bound2 = box2[i];
        auto lb1 = bound1.lo; auto ub1 = bound1.hi;
        auto lb2 = bound2.lo; auto ub2 = bound2.hi;
        if (lb1 == lb2 and ub1 == ub2) {
            continue;
        } else {
            return -1;
        }
    }
    return union_index;
}

bool union_possible(const std::vector<veritas::Interval>& box1, const std::vector<veritas::Interval>& box2) {
    int index = get_union_index(box1, box2);
    // if (index != -1) {
    //     dump(box1);
    //     dump(box2);
    //     std::cout << get_union_index(box1, box2) << std::endl;
    // }
    // if (index == 9) {
    //     std::cout << get_union_index(box1, box2);
    // }
    return index != -1;
}

veritas::Interval unionize(veritas::Interval bound1, veritas::Interval bound2) {
    auto lb = std::min(bound1.lo, bound2.lo);
    auto ub = std::max(bound1.hi, bound2.hi);
    veritas::Interval ival = {lb, ub};
    return ival;
}

std::vector<veritas::Interval> compute_union(const std::vector<veritas::Interval> box1, const std::vector<veritas::Interval> box2) {
    std::vector<veritas::Interval> box;
    box.reserve(box1.size());
    auto index = get_union_index(box1, box2);
    for (auto i = 0; i < box1.size(); ++i) {
        if (i == index) {
            box.push_back(unionize(box1[i], box2[i]));
        } else {
            box.push_back(box1[i]);
        }
    }
    // PLAJA_LOG("MERGE BELOW TWO INTO THIRD")
    // dump(box1); dump(box2); dump(box);
    return box;
}

std::vector<std::vector<veritas::Interval>> merge_boxes_once(std::vector<std::vector<veritas::Interval>> boxes) {
    std::vector<std::vector<veritas::Interval>> merged;
    merged.reserve(boxes.size());
    for (auto i = 0; i < boxes.size(); ++i) {
        bool merge = false;
        for (auto j = 0; j < merged.size(); ++j) {
            std::vector<veritas::Interval> box(merged[j].begin(), merged[j].end());
            std::vector<veritas::Interval> box1(boxes[i].begin(), boxes[i].end());
            if (union_possible(box, box1)) {
                merged[j] = compute_union(box, box1);
                merge = true;
                break;
            }
        }
        if (!merge) {
            merged.push_back(boxes[i]);
        }
    }
    return merged;
}

std::vector<std::vector<veritas::Interval>> merge_recursive(std::vector<std::vector<veritas::Interval>> boxes) {
    // std::cout << "Merge length: " << boxes.size() << std::endl;
    std::vector<std::vector<veritas::Interval>> unique_boxes;
    for (auto b : boxes) {
        bool seen = false;
        for (auto uni_b : unique_boxes) {
            if (areBoxesEqual(b, uni_b)) {
                seen = true;
                break;
            }
        }
        if (not seen) {
            unique_boxes.push_back(b);
        }
    }
    // std::cout << "Unique length: " << unique_boxes.size() << std::endl;

    std::vector<std::vector<veritas::Interval>> previous;
    std::vector<std::vector<veritas::Interval>> current = unique_boxes;

    while (current.size() != previous.size()) {
        previous = current;
        current = merge_boxes_once(current);
    }
    return current;
}

void dump(std::vector<veritas::Interval> box) {
    std::cout << "[";
    for (auto b: box) {
        std::cout << b << ", ";
    }
    std::cout << "]" << std::endl;
}