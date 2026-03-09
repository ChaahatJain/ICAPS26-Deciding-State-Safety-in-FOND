//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_COMPOSITION_H
#define PLAJA_COMPOSITION_H

#include "../using_parser.h"
#include "iterators/ast_iterator.h"
#include "ast_element.h"
#include "commentable.h"

// forward declaration:
class CompositionElement;
class Synchronisation;

class Composition: public AstElement, public Commentable {
private:
    std::vector<std::unique_ptr<CompositionElement>> elements;
    std::vector<std::unique_ptr<Synchronisation>> syncs;
public:
    Composition();
    ~Composition() override;

    // construct:
    void reserve(std::size_t elements_cap, std::size_t syncs_cap);
    void add_element(std::unique_ptr<CompositionElement>&& element);
    void add_synchronisation(std::unique_ptr<Synchronisation>&& sync);

    // setter:
    void set_element(std::unique_ptr<CompositionElement>&& elem, AutomatonIndex_type instance_index);
    void set_synchronisation(std::unique_ptr<Synchronisation>&& sync, SyncIndex_type sync_index);

    // getter:
    [[nodiscard]] inline std::size_t get_number_elements() const { return elements.size(); }
    [[nodiscard]] inline CompositionElement* get_element(AutomatonIndex_type instance_index) { PLAJA_ASSERT(instance_index < elements.size()) return elements[instance_index].get(); }
    [[nodiscard]] inline const CompositionElement* get_element(AutomatonIndex_type instance_index) const { PLAJA_ASSERT(instance_index < elements.size()) return elements[instance_index].get(); }
    [[nodiscard]] inline std::size_t get_number_syncs() const { return syncs.size(); }
    [[nodiscard]] inline Synchronisation* get_sync(SyncIndex_type sync_index) { PLAJA_ASSERT(sync_index < syncs.size()) return syncs[sync_index].get(); }
    [[nodiscard]] inline const Synchronisation* get_sync(SyncIndex_type sync_index) const { PLAJA_ASSERT(sync_index < syncs.size()) return syncs[sync_index].get(); }

    // iterators:
    [[nodiscard]] inline AstConstIterator<CompositionElement> elementIterator() const { return AstConstIterator(elements); }
    [[nodiscard]] inline AstIterator<CompositionElement> elementIterator() { return AstIterator(elements); }
    [[nodiscard]] inline AstConstIterator<Synchronisation> syncIterator() const { return AstConstIterator(syncs); }
    [[nodiscard]] inline AstIterator<Synchronisation> syncIterator() { return AstIterator(syncs); }

    /**
     * Deep copy of a composition.
     * @return
     */
    [[nodiscard]] std::unique_ptr<Composition> deepCopy() const;

    // override:
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;

};


#endif //PLAJA_COMPOSITION_H
