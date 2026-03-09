//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <iostream>
#include "restrictions_smt_checker.h"
#include "../../../exception/not_supported_exception.h"
#include "../../../parser/jani_words.h"
#include "../../../utils/utils.h"

#include "../../../parser/ast/assignment.h"
#include "../../../parser/ast/variable_declaration.h"

#include "../../../parser/ast/expression/lvalue_expression.h"
#include "../../../parser/ast/expression/binary_op_expression.h"
#include "../../../parser/ast/expression/unary_op_expression.h"

RestrictionsSMTChecker::RestrictionsSMTChecker() = default;

RestrictionsSMTChecker::~RestrictionsSMTChecker() = default;

bool RestrictionsSMTChecker::check_restrictions(const AstElement* ast_element, bool catch_exception) {
    RestrictionsSMTChecker restrictions_checker;

    if (catch_exception) { // branch outside of try-catch to keep copy constructor of exception deleted
        try { ast_element->accept(&restrictions_checker); }
        catch (NotSupportedException& e) {
            PLAJA_LOG(e.what())
            return false;
        }
    } else { ast_element->accept(&restrictions_checker); }

    return true;
}

void RestrictionsSMTChecker::visit(const Assignment* assignment) {
    if (assignment->get_index() > 0) { throw NotSupportedException(assignment->to_string(), JANI::INDEX); }
    AstVisitorConst::visit(assignment);
}

void RestrictionsSMTChecker::visit(const VariableDeclaration* var_decl) {
    /* const std::string& var_name = var_decl->get_name();
    for (const auto* str: std::vector<const std::string*>{}) {
        if (PLAJA_UTILS::contains(var_name, *str)) { throw NotSupportedException(var_decl->to_string(), "Z3 reserved substring"); }
    } */
    if (var_decl->is_transient()) { throw NotSupportedException(var_decl->to_string(), JANI::TRANSIENT); }
}

void RestrictionsSMTChecker::visit(const BinaryOpExpression* exp) {
    if (exp->get_op() == BinaryOpExpression::LOG) { throw NotSupportedException(exp->to_string(), BinaryOpExpression::binary_op_to_str(BinaryOpExpression::LOG)); }
    AstVisitorConst::visit(exp);
}

void RestrictionsSMTChecker::visit(const UnaryOpExpression* exp) {
    switch (exp->get_op()) {
        case UnaryOpExpression::SGN: throw NotSupportedException(exp->to_string(), UnaryOpExpression::unary_op_to_str(UnaryOpExpression::SGN));
        case UnaryOpExpression::TRC: throw NotSupportedException(exp->to_string(), UnaryOpExpression::unary_op_to_str(UnaryOpExpression::TRC));
        default: break;
    }
    AstVisitorConst::visit(exp);
}
