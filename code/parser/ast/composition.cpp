//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "composition.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "composition_element.h"
#include "synchronisation.h"

Composition::Composition() = default;

Composition::~Composition() = default;

// construction:

void Composition::reserve(std::size_t elements_cap, std::size_t syncs_cap) {
    elements.reserve(elements_cap);
    syncs.reserve(syncs_cap);
}

void Composition::add_element(std::unique_ptr<CompositionElement>&& element) {
    elements.push_back(std::move(element));
}

void Composition::add_synchronisation(std::unique_ptr<Synchronisation>&& sync) {
    syncs.push_back(std::move(sync));
}

// setter:

void Composition::set_element(std::unique_ptr<CompositionElement>&& elem, AutomatonIndex_type instance_index) {
    PLAJA_ASSERT(instance_index < elements.size())
    elements[instance_index] = std::move(elem);
}

void Composition::set_synchronisation(std::unique_ptr<Synchronisation>&& sync, SyncIndex_type sync_index) {
    PLAJA_ASSERT(sync_index < syncs.size())
    syncs[sync_index] = std::move(sync);
}

//

std::unique_ptr<Composition> Composition::deepCopy() const {
    std::unique_ptr<Composition> copy(new Composition());
    copy->copy_comment(*this);

    copy->elements.reserve(elements.size());
    for (const auto& elem: elements) { copy->elements.emplace_back(elem->deepCopy()); }

    copy->syncs.reserve(syncs.size());
    for (const auto& sync: syncs) { copy->syncs.emplace_back(sync->deepCopy()); }

    return copy;
}

// override:

void Composition::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void Composition::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
