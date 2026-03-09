//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <iostream>
#include "restrictions_checker_explicit.h"
#include "../../exception/not_supported_exception.h"
#include "../../parser/ast/type/declaration_type.h"
#include "../../parser/ast/assignment.h"
#include "../../parser/ast/variable_declaration.h"
#include "../../parser/jani_words.h"
#include "../../utils/utils.h"

RestrictionsCheckerExplicit::RestrictionsCheckerExplicit() = default;

RestrictionsCheckerExplicit::~RestrictionsCheckerExplicit() = default;

bool RestrictionsCheckerExplicit::check_restrictions(const AstElement* ast_element, bool catch_exception) {
    RestrictionsCheckerExplicit restrictions_checker;

    if (catch_exception) { // branch outside of try-catch to keep copy constructor of exception deleted
        try { ast_element->accept(&restrictions_checker); }
        catch (NotSupportedException& e) {
            PLAJA_LOG(e.what());
            return false;
        }
    } else { ast_element->accept(&restrictions_checker); }

    return true;
}

bool RestrictionsCheckerExplicit::check_restrictions(const Model* model, bool catch_exception) {
    return check_restrictions(cast_model(model), catch_exception);
}

void RestrictionsCheckerExplicit::visit(const Assignment* assignment) {
    AstVisitorConst::visit(assignment);

    if (not assignment->is_deterministic()) {
        throw NotSupportedException(assignment->to_string(), JANI::VALUE); // need deterministic value assignment
    }
}

void RestrictionsCheckerExplicit::visit(const VariableDeclaration* var_decl) {

    if (var_decl->get_type()->is_floating_type()) {
        static bool print_warning(true);
        if (print_warning) {
            PLAJA_LOG(PLAJA_UTILS::to_red_string("Explicit engine is potentially incomplete when using continuous variables."))
            print_warning = false;
        }
    }

}
