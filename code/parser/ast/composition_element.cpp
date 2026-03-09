//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "composition_element.h"

#include <utility>
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"

CompositionElement::CompositionElement(std::string element_automaton):
    elementAutomaton(std::move(element_automaton)) {
}

CompositionElement::~CompositionElement() = default;

// construction:

void CompositionElement::reserve(std::size_t input_enables_cap) {
    inputEnables.reserve(input_enables_cap);
}

void CompositionElement::add_inputEnable(const std::string& input_enable, ActionID_type input_enabled_id) {
    inputEnables.emplace_back(input_enable, input_enabled_id);
}

std::unique_ptr<CompositionElement> CompositionElement::deepCopy() const {
    std::unique_ptr<CompositionElement> copy(new CompositionElement(elementAutomaton));
    copy->copy_comment(*this);
    copy->inputEnables = inputEnables;
    return copy;
}

// override:

void CompositionElement::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void CompositionElement::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }







