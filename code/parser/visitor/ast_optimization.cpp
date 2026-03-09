//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ast_optimization.h"
#include "../../exception/semantics_exception.h"
#include "../../search/using_search.h"
#include "../../utils/floating_utils.h"
#include "../ast/expression/non_standard/state_condition_expression.h"
#include "../ast/expression/non_standard/state_values_expression.h"
#include "../ast/expression/non_standard/states_values_expression.h"
// #include "../ast/expression/special_cases/linear_integer_expression.h"
#include "../ast/expression/bool_value_expression.h"
#include "../ast/expression/binary_op_expression.h"
#include "../ast/expression/constant_expression.h"
#include "../ast/expression/integer_value_expression.h"
#include "../ast/expression/ite_expression.h"
#include "../ast/expression/real_value_expression.h"
#include "../ast/expression/unary_op_expression.h"
#include "../ast/type/declaration_type.h"
#include "../ast/model.h"
#include "../ast/automaton.h"
#include "../ast/destination.h"
#include "../ast/edge.h"
#include "extern/visitor_base.h"

AstOptimization::AstOptimization() = default;

AstOptimization::~AstOptimization() = default;

//

bool AstOptimization::evaluate_constant(const Expression& expr) {
    if (expr.is_constant()) {
        const auto* decl_type = expr.get_type();
        if (decl_type->is_boolean_type()) { set_to_replace(std::make_unique<BoolValueExpression>(expr.evaluate_integer_const())); }
        else if (decl_type->is_integer_type()) { set_to_replace(std::make_unique<IntegerValueExpression>(expr.evaluate_integer_const())); }
        else if (decl_type->is_floating_type()) { set_to_replace(std::make_unique<RealValueExpression>(RealValueExpression::NONE_C, expr.evaluate_floating_const())); }
        else { PLAJA_ABORT }
        return true;
    }
    return false;
}

void AstOptimization::optimize_ast(std::unique_ptr<AstElement>& ast_elem) {
    AstOptimization ast_opt;
    ast_elem->accept(&ast_opt);
    if (ast_opt.replace_by_result) { ast_elem = ast_opt.move_result<AstElement>(); }
}

void AstOptimization::optimize_ast(Model& model) {
    AstOptimization ast_opt;
    model.accept(&ast_opt);
    PLAJA_ASSERT(not ast_opt.replace_by_result)
}

//

void AstOptimization::visit(Automaton* automaton) {
    AstVisitor::visit(automaton);

    std::vector<std::unique_ptr<Edge>> edges;
    for (auto it = automaton->edgeIterator(); !it.end(); ++it) {
        const auto* guard = it->get_guardExpression();
        if (not guard or not guard->is_constant() or guard->evaluate_integer_const()) { edges.push_back(it.set()); }
    }
    automaton->set_edges(std::move(edges));

}

void AstOptimization::visit(Edge* edge) { // for some on-the-fly checks & optimizations (specifically wrt. probability distribution)
    AstVisitor::visit(edge);

    // check probability
    bool const_prob_distro = true;
    bool zero_prob_edge = false;
    PLAJA::Prob_type prob_sum = 0.0;
    for (auto it = edge->destinationIterator(); !it.end(); ++it) {
        const auto* prob_exp_dest = it->get_probabilityExpression();
        if (prob_exp_dest) {
            if (prob_exp_dest->is_constant()) {
                const PLAJA::Prob_type prob_dest = prob_exp_dest->evaluate_floating_const();
                prob_sum += prob_dest;
                if (PLAJA_FLOATS::is_zero(prob_dest, PLAJA::probabilityPrecision)) { zero_prob_edge = true; }
            } else { const_prob_distro = false; }
        } else { prob_sum += 1.0; }
        if (not const_prob_distro and zero_prob_edge) { break; } // all checks done ...
    }
    if (const_prob_distro) { if (not PLAJA_FLOATS::equal(prob_sum, 1.0, PLAJA::probabilityPrecision)) { throw SemanticsException(edge->to_string()); } }

    // optimize edges (remove 0-probability destinations)
    if (zero_prob_edge) {
        std::vector<std::unique_ptr<Destination>> destinations;
        for (auto it = edge->destinationIterator(); !it.end(); ++it) {
            const auto* prob_exp_dest = it->get_probabilityExpression();
            if (prob_exp_dest and prob_exp_dest->is_constant() and PLAJA_FLOATS::is_zero(prob_exp_dest->evaluate_floating_const(), PLAJA::probabilityPrecision)) { continue; }
            else { destinations.push_back(it.set()); }
        }
        edge->set_destinations(std::move(destinations));
    }

}

//

void AstOptimization::visit(BinaryOpExpression* exp) {
    if (not evaluate_constant(*exp)) { AstVisitor::visit(exp); }

    const auto* left = exp->get_left();
    const auto* right = exp->get_right();

    switch (exp->get_op()) {

        case BinaryOpExpression::OR: {
            if (left->is_constant()) {
                if (left->evaluate_integer_const()) { set_to_replace(exp->set_left()); } // simplify to "true"
                else { set_to_replace(exp->set_right()); } // remove "false"
            } else if (right->is_constant()) {
                if (right->evaluate_integer_const()) { set_to_replace(exp->set_right()); } // simplify to "true"
                else { set_to_replace(exp->set_left()); } // remove "false"
            }
            break;
        }

        case BinaryOpExpression::AND: {
            if (left->is_constant()) {
                if (left->evaluate_integer_const()) { set_to_replace(exp->set_right()); } // remove "true"
                else { set_to_replace(exp->set_left()); } // simplify to "false"
            } else if (right->is_constant()) {
                if (right->evaluate_integer_const()) { set_to_replace(exp->set_left()); } // remove "true"
                else { set_to_replace(exp->set_right()); } // simplify to "false"
            }
            break;
        }

        case BinaryOpExpression::IMPLIES: { // A -> B (!A or B)
            if (left->is_constant()) {
                if (left->evaluate_integer_const()) { set_to_replace(exp->set_right()); } // remove "true" premise
                else { set_to_replace(std::make_unique<BoolValueExpression>(true)); } // simplify to "true"
            } else if (right->is_constant()) {
                if (right->evaluate_integer_const()) { set_to_replace(std::make_unique<BoolValueExpression>(true)); } // simplify to "true"
                else {
                    auto not_left = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT); // simplify to "not premise"
                    not_left->set_operand(exp->set_left());
                    set_to_replace(std::move(not_left));
                }
            }
            break;
        }

        default: break;
    }
}

void AstOptimization::visit(ConstantExpression* exp) { if (not evaluate_constant(*exp)) { AstVisitor::visit(exp); } }

void AstOptimization::visit(ITE_Expression* exp) {
    if (not evaluate_constant(*exp)) { AstVisitor::visit(exp); }
    if (exp->get_condition()->is_constant()) {
        if (exp->get_condition()->evaluate_integer_const()) { set_to_replace(exp->set_consequence(nullptr)); }
        else { set_to_replace(exp->set_alternative(nullptr)); }
    }
}

void AstOptimization::visit(UnaryOpExpression* exp) { if (not evaluate_constant(*exp)) { AstVisitor::visit(exp); } }

// non-standard:

void AstOptimization::visit(StateConditionExpression* exp) { if (not evaluate_constant(*exp)) { AstVisitor::visit(exp); } }

void AstOptimization::visit(StateValuesExpression* exp) { if (not evaluate_constant(*exp)) { AstVisitor::visit(exp); } }

void AstOptimization::visit(StatesValuesExpression* exp) { if (not evaluate_constant(*exp)) { AstVisitor::visit(exp); } }

//

/**********************************************************************************************************************/

namespace AST_OPTIMIZATION {

    void optimize_ast(std::unique_ptr<AstElement>& ast_elem) { AstOptimization::optimize_ast(ast_elem); }

    void optimize_ast(std::unique_ptr<Expression>& expr) { PLAJA_VISITOR::visit_cast<Expression, AST_OPTIMIZATION::optimize_ast>(expr); }

    void optimize_ast(Model& model) { AstOptimization::optimize_ast(model); }

}