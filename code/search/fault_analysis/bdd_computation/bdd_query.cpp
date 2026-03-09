//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "bdd_query.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/ast/expression/variable_expression.h"
#include "../../../parser/ast/expression/integer_value_expression.h"
#include "../../../parser/ast/expression/constant_value_expression.h"
#include "../../../parser/ast/expression/binary_op_expression.h"
#include "../../../parser/ast/expression/unary_op_expression.h"
#include "../../../parser/ast/expression/special_cases/nary_expression.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../../parser/ast/expression/non_standard/let_expression.h"
#include "../../successor_generation/update.h"
#include "../../../parser/visitor/linear_constraints_checker.h"
#include "../../../exception/constructor_exception.h"
#include "predicates/equation.h"
#include "predicates/tightening.h"
#include <memory>
#include <typeinfo>
using namespace std;
BDD_IN_PLAJA::Query::Query():
    context(std::make_shared<BDD_IN_PLAJA::Context>()) {}

BDD_IN_PLAJA::Query::~Query() = default;

BDD BDD_IN_PLAJA::Query::unary_to_bdd(const UnaryOpExpression* expr) const {
    if(dynamic_cast<const NaryExpression*>(expr->get_operand())) {
       auto test = dynamic_cast<const NaryExpression*>(expr->get_operand());
        PLAJA_ASSERT(test)
        PLAJA_ASSERT(expr->get_op() == UnaryOpExpression::UnaryOp::NOT)
        return !(nary_to_bdd(test));     
    } else if (dynamic_cast<const LinearExpression*>(expr->get_operand())) {
        auto lin = dynamic_cast<const LinearExpression*>(expr->get_operand());
        PLAJA_ASSERT(expr->get_op() == UnaryOpExpression::UnaryOp::NOT)
        return !(lin_expr_to_bdd(lin));
    } else if (dynamic_cast<const BinaryOpExpression*>(expr->get_operand())) {
        auto bin = dynamic_cast<const BinaryOpExpression*>(expr->get_operand());
        return !(binary_op_expression_to_bdd(bin));
    } else {
        std::cout << typeid(*expr->get_operand()).name() << std::endl;
        expr->dump(true);
    }
    PLAJA_ABORT;
}

BDD BDD_IN_PLAJA::Query::nary_to_bdd(const NaryExpression* guard) const {
    std::vector<BDD> nary;
    for (auto index = 0; index < guard->get_size(); ++index) {
        auto sub = guard->get_sub(index);
        BDD sub_bdd = get_true();
        if (dynamic_cast<const LinearExpression*>(sub)) {
            auto sub_operation = dynamic_cast<const LinearExpression*>(sub); 
            sub_bdd = lin_expr_to_bdd(sub_operation);
            nary.push_back(sub_bdd);
        } else if (dynamic_cast<const NaryExpression*>(sub)) {
            auto sub_operation = dynamic_cast<const NaryExpression*>(sub);
            sub_bdd = nary_to_bdd(sub_operation);
            nary.push_back(sub_bdd);
        } else if (dynamic_cast<const BinaryOpExpression*>(sub)) {
            auto sub_operation = dynamic_cast<const BinaryOpExpression*>(sub);
            sub_bdd = binary_op_expression_to_bdd(sub_operation);
            nary.push_back(sub_bdd);
        } else if (dynamic_cast<const UnaryOpExpression*>(sub)) {
            auto sub_op = dynamic_cast<const UnaryOpExpression*>(sub);
            sub_bdd = unary_to_bdd(sub_op);
            nary.push_back(sub_bdd);
        }
        else {
            sub->dump(true);
            std::cout << typeid(*sub).name() << std::endl;
            PLAJA_ABORT;
        }
    }
    switch (guard->get_op()) {
        case BinaryOpExpression::BinaryOp::AND: {
            return computeConjunctionTree(nary);
        }
        case BinaryOpExpression::BinaryOp::OR: {
            return computeDisjunctionTree(nary);
        }
        default:
            PLAJA_ABORT;
    }
}

BDD BDD_IN_PLAJA::Query::state_condition_to_bdd(const StateConditionExpression* state) const {
    if (dynamic_cast<const NaryExpression*>(state->get_constraint())) {
        auto g = dynamic_cast<const NaryExpression*>(state->get_constraint());
        return nary_to_bdd(g);
    } else if (dynamic_cast<const LinearExpression*>(state->get_constraint())) {
        auto g = dynamic_cast<const LinearExpression*>(state->get_constraint());
        return lin_expr_to_bdd(g);
    } else if (dynamic_cast<const LetExpression*>(state->get_constraint())) {
        auto g = dynamic_cast<const LetExpression*>(state->get_constraint());
        auto expr = dynamic_cast<const NaryExpression*>(g->get_expression());
        return nary_to_bdd(expr);
    } else if (dynamic_cast<const BinaryOpExpression*>(state->get_constraint())) {
        auto g = dynamic_cast<const BinaryOpExpression*>(state->get_constraint());
        return binary_op_expression_to_bdd(g);
    }
    state->get_constraint()->dump(true);
    std::cout << typeid(*state->get_constraint()).name() << std::endl;
    PLAJA_ABORT;
}

BDD BDD_IN_PLAJA::Query::binary_op_expression_to_bdd(const BinaryOpExpression* bin) const {
    std::vector<BDD> sides;
    if (dynamic_cast<const BinaryOpExpression*>(bin->get_left())) {
        auto left = dynamic_cast<const BinaryOpExpression*>(bin->get_left());
        sides.push_back(binary_op_expression_to_bdd(left));
    } else if (dynamic_cast<const LinearExpression*>(bin->get_left())) {
        auto left = dynamic_cast<const LinearExpression*>(bin->get_left());
        sides.push_back(lin_expr_to_bdd(left));
    } else if (dynamic_cast<const NaryExpression*>(bin->get_left())) {
        auto left = dynamic_cast<const NaryExpression*>(bin->get_left());
        sides.push_back(nary_to_bdd(left));
    } else if (dynamic_cast<const UnaryOpExpression*>(bin->get_left())) {
        auto left = dynamic_cast<const UnaryOpExpression*>(bin->get_left());
        sides.push_back(unary_to_bdd(left));
    } else {
        std::cout << typeid(*bin->get_left()).name() << std::endl;
        PLAJA_ABORT;
    }

    if (dynamic_cast<const BinaryOpExpression*>(bin->get_right())) {
        auto right = dynamic_cast<const BinaryOpExpression*>(bin->get_right());
        sides.push_back(binary_op_expression_to_bdd(right));
    } else if (dynamic_cast<const LinearExpression*>(bin->get_right())) {
        auto right = dynamic_cast<const LinearExpression*>(bin->get_right());
        sides.push_back(lin_expr_to_bdd(right));
    } else if (dynamic_cast<const NaryExpression*>(bin->get_right())) {
        auto right = dynamic_cast<const NaryExpression*>(bin->get_right());
        sides.push_back(nary_to_bdd(right));
    } else if (dynamic_cast<const UnaryOpExpression*>(bin->get_right())) {
        auto right = dynamic_cast<const UnaryOpExpression*>(bin->get_right());
        sides.push_back(unary_to_bdd(right));
    } else {
        std::cout << typeid(*bin->get_right()).name() << std::endl;
        PLAJA_ABORT;
    }

    switch (bin->get_op()) {
        case BinaryOpExpression::BinaryOp::AND: return computeConjunctionTree(sides);
        case BinaryOpExpression::BinaryOp::OR: return computeDisjunctionTree(sides);
        default:
            PLAJA_ABORT;
    }
}


BDD BDD_IN_PLAJA::Query::guard_to_bdd(ActionOp action_op) {
    // Assumes guard is just a conjunction of constraints.
    BDD g = context->get_true();
    for (auto it_guard = action_op.guardIterator(); !it_guard.end(); ++it_guard) {
        if (dynamic_cast<const NaryExpression*>(it_guard.operator()())) {
            auto guard = dynamic_cast<const NaryExpression*>(it_guard.operator()());
           g = g & nary_to_bdd(guard);
        } else if (dynamic_cast<const BinaryOpExpression*>(it_guard.operator()())) {
            auto guard = dynamic_cast<const BinaryOpExpression*>(it_guard.operator()());
            g = g & binary_op_expression_to_bdd(guard);
        } else if (dynamic_cast<const LinearExpression*>(it_guard.operator()())) {
            auto guard = dynamic_cast<const LinearExpression*>(it_guard.operator()());
            g = g & lin_expr_to_bdd(guard);
        } else {    
            action_op.dump(nullptr, true);
            std::cout << "Unknown Expression: " << typeid(*it_guard).name() << std::endl;
        }
        
    }
    return g;
}

BDD BDD_IN_PLAJA::Query::update_to_bdd(const Update& update) {
    BDD u = context->get_true();
    for (auto it = update.assignmentIterator(0); !it.end(); ++it) {
        const VariableIndex_type target_index = it.variable_index() + context->get_number_of_inputs() - 1;
        const auto* assignment_expression = it.value();
        PLAJA_ASSERT(assignment_expression)
        if (not LinearConstraintsChecker::is_linear_assignment(*assignment_expression, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE))) { throw ConstructorException(assignment_expression->to_string(), PLAJA_EXCEPTION::nonLinear); }
        if (dynamic_cast<const IntegerValueExpression*>(assignment_expression)) {
            auto int_expr = dynamic_cast<const IntegerValueExpression*>(assignment_expression);
            std::vector<BDD_IN_PLAJA::Context::Addend> addends;
            addends.emplace_back(BDD_IN_PLAJA::Context::Addend(target_index, 1));
            auto equation = BDD_IN_PLAJA::Equation(addends, int_expr->get_value());
            u = u & equation.to_bdd(context);
        } else if (dynamic_cast<const VariableExpression*>(assignment_expression)) {
            auto var_expr = dynamic_cast<const VariableExpression*>(assignment_expression);
            std::vector<BDD_IN_PLAJA::Context::Addend> addends;
            addends.emplace_back(BDD_IN_PLAJA::Context::Addend(target_index, 1));
            addends.emplace_back(BDD_IN_PLAJA::Context::Addend(var_expr->get_index(), -1));
            auto equation = BDD_IN_PLAJA::Equation(addends, 0);
            u = u & equation.to_bdd(context);
        } else if (dynamic_cast<const LinearExpression*>(assignment_expression)) {
            auto test = dynamic_cast<const LinearExpression*>(assignment_expression);
            u = u & lin_expr_to_bdd(test, target_index);
        } else {
            std::cout << "Unknown Expression type: " << typeid(*assignment_expression).name() << std::endl;
            PLAJA_ASSERT(false);
        }
        PLAJA_ABORT_IF(u == get_false()) 
        PLAJA_ABORT_IF(get_true() == u)
       
    }
    
    return u;
}
std::vector<BDD> BDD_IN_PLAJA::Query::updates_to_bdd(ActionOp action_op) {
    std::vector<BDD> updates;
    for (auto it_upd = action_op.updateIterator(); !it_upd.end(); ++it_upd) {
        auto update = action_op.get_update(it_upd.update_index());
        updates.push_back(update_to_bdd(update));
    }
    return updates;
}

std::vector<int> BDD_IN_PLAJA::Query::targets_in_updates(ActionOp action_op) {
    std::vector<int> variables;
    for (auto it_upd = action_op.updateIterator(); !it_upd.end(); ++it_upd) {
        auto update = action_op.get_update(it_upd.update_index());
        for (auto it = update.assignmentIterator(0); !it.end(); ++it) {
            const VariableIndex_type target_index = it.variable_index() + context->get_number_of_inputs() - 1;
            variables.push_back(target_index);
        }
    }
    return variables;
}

void BDD_IN_PLAJA::Query::operator_to_bdd(ActionOp action_op) {
    // action_op.dump(nullptr, true);
    BDD guard = guard_to_bdd(action_op);
    PLAJA_ABORT_IF(get_true() == guard)
    std::vector<BDD> updates = updates_to_bdd(action_op);
    // std::vector<int> targets = targets_in_updates(action_op);
    context->save_operator_BDD(action_op._op_id(), guard, updates, {});
}

BDD BDD_IN_PLAJA::Query::lin_expr_to_bdd(const LinearExpression* expr, int target) const {
    if (expr->is_linear_sum()) {
        return eq_to_bdd(expr, target);
    } else {
        return bound_to_bdd(expr);
    }
}

BDD BDD_IN_PLAJA::Query::eq_to_bdd(const LinearExpression* expr, int target) const {
    std::vector<BDD_IN_PLAJA::Context::Addend> addends;
    int invert = 1;
    if (target != -1) {
        addends.emplace_back(BDD_IN_PLAJA::Context::Addend(target, 1));
        invert = -1;
    }
    for (auto it = expr->addendIterator(); !it.end(); ++it) {
        auto factor = dynamic_cast<const IntegerValueExpression*>(it.factor());
        addends.emplace_back(BDD_IN_PLAJA::Context::Addend(it.var_id(), invert*factor->get_value()));
    }
    auto scalar = dynamic_cast<const IntegerValueExpression*>(expr->get_scalar());
    auto equation = BDD_IN_PLAJA::Equation(addends, scalar->get_value());
    return equation.to_bdd(context);
}

BDD BDD_IN_PLAJA::Query::bound_to_bdd(const LinearExpression* expr, bool is_output) const {
    if (expr->get_op() == BinaryOpExpression::BinaryOp::EQ) {
        return eq_to_bdd(expr, -1);
    }

    if (expr->get_op() == BinaryOpExpression::BinaryOp::NE) {
        return !eq_to_bdd(expr, -1);
    }

    int invert = expr->get_op() == BinaryOpExpression::BinaryOp::LE ? 1 : -1;
    int output = is_output ? context->get_number_of_inputs() : 0;
    std::vector<BDD_IN_PLAJA::Context::Addend> addends;
    for (auto it = expr->addendIterator(); !it.end(); ++it) {
        auto factor = dynamic_cast<const IntegerValueExpression*>(it.factor());
        addends.emplace_back(BDD_IN_PLAJA::Context::Addend(it.var_id() + output, invert*factor->get_value()));
    }
    auto scalar = dynamic_cast<const IntegerValueExpression*>(expr->get_scalar());
    auto bound = BDD_IN_PLAJA::Tightening(addends, invert*(scalar->get_value() + invert));
    return bound.to_bdd(context);
}

BDD BDD_IN_PLAJA::Query::get_relational_product(BDD rho, BDD update, bool abstractSource, bool debug) {
    BDD conj = rho & update & context->get_unused_biimp(rho, update);
    if (debug) {
        cout << "Rho: " << rho << endl;
        cout << "Update: " << update << endl;
        cout << "Conj: " << conj << endl;
    }
    if (!abstractSource) {
        auto targets = context->get_target_variables();
        for (auto b : targets) {
            conj = conj.ExistAbstract(b);
        }
    } else {
        auto sources = context->get_source_variables();
        for (auto b : sources) {
            conj = conj.ExistAbstract(b);
        }
    }
    if (debug) {
        cout << "Conj final: " << conj << endl;
    }
    return conj;
}

BDD BDD_IN_PLAJA::Query::computeConjunctionTree(std::vector<BDD> bdds) const {
    if (bdds.size() == 0) {
        return get_true();
    }
    if (bdds.size() == 1) {
        return bdds.at(0);
    }
    int mid = bdds.size()/2;
    std::vector<BDD> left(bdds.begin(), bdds.begin() + mid);
    std::vector<BDD> right(bdds.begin() + mid, bdds.end());
    return computeConjunctionTree(left) & computeConjunctionTree(right);
}


BDD BDD_IN_PLAJA::Query::computeDisjunctionTree(std::vector<BDD> bdds) const {
    if (bdds.size() == 0) {
        return get_false();
    }
    if (bdds.size() == 1) {
        return bdds.at(0);
    }
    int mid = bdds.size()/2;
    std::vector<BDD> left(bdds.begin(), bdds.begin() + mid);
    std::vector<BDD> right(bdds.begin() + mid, bdds.end());
    return computeDisjunctionTree(left) | computeDisjunctionTree(right);
}

