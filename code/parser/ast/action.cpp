//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "action.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"

Action::Action(std::string name, ActionID_type id):
    name(std::move(name))
    , id(id) {
}

Action::~Action() = default;

std::unique_ptr<Action> Action::deepCopy() const {
    std::unique_ptr<Action> copy(new Action(name, id));
    copy->copy_comment(*this);
    return copy;
}

// override:

void Action::accept(AstVisitor* astVisitor) {
    astVisitor->visit(this);
}

void Action::accept(AstVisitorConst* astVisitor) const {
    astVisitor->visit(this);
}

namespace ACTION { const std::string silentName { "-1 (silent)" }; }