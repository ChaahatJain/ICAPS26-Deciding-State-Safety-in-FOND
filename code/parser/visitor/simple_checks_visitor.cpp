//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "simple_checks_visitor.h"
#include "../ast/ast_element.h"
#include "../ast/model.h"
#include "../ast/variable_declaration.h"
#include "../ast/expression/variable_expression.h"
#include "../ast/type/declaration_type.h"

SimpleChecksVisitor::SimpleChecksVisitor(const Model* model_): model(model_), sampling_free(true), containsContinuous(false), containsClocks(false), foundTransients(false) {}

SimpleChecksVisitor::~SimpleChecksVisitor() = default;

//

bool SimpleChecksVisitor::contains_clocks(const AstElement* ast_el) {
    SimpleChecksVisitor checkReferenceClocks(nullptr);
    ast_el->accept(&checkReferenceClocks);
    return checkReferenceClocks.containsClocks;
}

bool SimpleChecksVisitor::contains_continuous(const AstElement* ast_el) {
    SimpleChecksVisitor checkReferenceContinuous(nullptr);
    ast_el->accept(&checkReferenceContinuous);
    return checkReferenceContinuous.containsContinuous;
}

bool SimpleChecksVisitor::contains_transients(const AstElement* ast_el, const Model& model) {
    SimpleChecksVisitor checkReferenceTransients(&model);
    ast_el->accept(&checkReferenceTransients);
    return checkReferenceTransients.foundTransients;
}

bool SimpleChecksVisitor::is_sampling_free(const AstElement* ast_el) {
    SimpleChecksVisitor samplingFreeVisitor(nullptr);
    ast_el->accept(&samplingFreeVisitor);
    return samplingFreeVisitor.sampling_free;
}

//

void SimpleChecksVisitor::visit(const ClockType* /*type_*/) { containsClocks = true; }

void SimpleChecksVisitor::visit(const ContinuousType* /*type_*/) { containsContinuous = true; }

void SimpleChecksVisitor::visit(const VariableDeclaration* varDecl) { foundTransients = varDecl->is_transient(); }

void SimpleChecksVisitor::visit(const VariableExpression* exp) {
    AstVisitorConst::visit(exp);
    // Check transient:
    if (model) { model->get_variable_by_id(exp->get_id())->accept(this); }
}

void SimpleChecksVisitor::visit(const DistributionSamplingExpression* /*exp*/) { sampling_free = false; }
