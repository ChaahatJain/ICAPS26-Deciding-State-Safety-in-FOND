//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MODEL_ITERATOR_H
#define PLAJA_MODEL_ITERATOR_H

#include "../model.h"
#include "ast_iterator.h"

class ModelIterator {
public:
    [[nodiscard]] inline static AstConstIterator<Action> actionIterator(const Model& model) { return AstConstIterator(model.actions); }
    [[nodiscard]] inline static AstIterator<Action> actionIterator(Model& model) { return AstIterator(model.actions); }
    [[nodiscard]] inline static AstConstIterator<ConstantDeclaration> constantIterator(const Model& model) { return AstConstIterator(model.constants); }
    [[nodiscard]] inline static AstIterator<ConstantDeclaration> constantIterator(Model& model) { return AstIterator(model.constants); }
    [[nodiscard]] inline static AstConstIterator<VariableDeclaration> globalVariableIterator(const Model& model) { return AstConstIterator(model.variables); }
    [[nodiscard]] inline static AstIterator<VariableDeclaration> globalVariableIterator(Model& model) { return AstIterator(model.variables); }
    [[nodiscard]] inline static AstConstIterator<Property> propertyIterator(const Model& model) { return AstConstIterator(model.properties); }
    [[nodiscard]] inline static AstIterator<Property> propertyIterator(Model& model) { return AstIterator(model.properties); }
    [[nodiscard]] inline static AstConstIterator<Automaton> automatonInstanceIterator(const Model& model) { return AstConstIterator(model.automataInstances); }
    [[nodiscard]] inline static AstIterator<Automaton> automatonInstanceIterator(Model& model) { return AstIterator(model.automataInstances); }
};


#endif //PLAJA_MODEL_ITERATOR_H
