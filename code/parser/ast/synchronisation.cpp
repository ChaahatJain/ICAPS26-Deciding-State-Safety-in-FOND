//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "synchronisation.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"

Synchronisation::Synchronisation():
    synchronise()
    , resultAction(PLAJA_UTILS::emptyString)
    , resultActionID(ACTION::silentAction)
    , syncID(SYNCHRONISATION::nonSyncID /*invalid for syncs*/) {
}

Synchronisation::~Synchronisation() = default;

// construction:

void Synchronisation::reserve(std::size_t sync_cap) { synchronise.reserve(sync_cap); }

void Synchronisation::add_sync(const std::string& sync_action, ActionID_type sync_action_id) { synchronise.emplace_back(sync_action, sync_action_id); }

//

std::unique_ptr<Synchronisation> Synchronisation::deepCopy() const {
    std::unique_ptr<Synchronisation> copy(new Synchronisation());
    copy->copy_comment(*this);
    copy->synchronise = synchronise;
    copy->resultAction = resultAction;
    copy->syncID = syncID;
    return copy;
}

// override:

void Synchronisation::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void Synchronisation::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
