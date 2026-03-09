//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "set_variable_index.h"
#include "../../exception/semantics_exception.h"
#include "../../include/ct_config_const.h"
#include "../../search/information/model_information.h"
#include "../ast/expression/non_standard/variable_value_expression.h"
#include "../ast/expression/variable_expression.h"
#include "../ast/variable_declaration.h"
#include "../ast/iterators/model_iterator.h"
#include "../ast/destination.h"
#include "../ast/edge.h"
#include "ast_visitor.h"

class SetVarIdById final: public AstVisitor {

private:
    const std::unordered_map<VariableID_type, VariableID_type>* oldToNew;

public:
    explicit SetVarIdById(const std::unordered_map<VariableID_type, VariableID_type>& old_to_new);
    ~SetVarIdById() override;
    DELETE_CONSTRUCTOR(SetVarIdById)

    void visit(VariableDeclaration* var_decl) override;
    void visit(VariableExpression* expr) override;
};

SetVarIdById::SetVarIdById(const std::unordered_map<VariableID_type, VariableID_type>& old_to_new):
    oldToNew(&old_to_new) {
}

SetVarIdById::~SetVarIdById() = default;

void SetVarIdById::visit(VariableDeclaration* var_decl) {
    PLAJA_ASSERT_EXPECT(var_decl->get_index() == STATE_INDEX::none) // Otherwise index probably to be updated as well.
    auto it = oldToNew->find(var_decl->get_id());
    if (it == oldToNew->end()) { return; }
    var_decl->set_id(it->second);
}

void SetVarIdById::visit(VariableExpression* expr) {
    if constexpr (PLAJA_GLOBAL::debug) {
        auto it = oldToNew->find(expr->get_id());
        if (it == oldToNew->end()) { return; }
        PLAJA_ASSERT(expr->get_id() == it->second) // Should directly reference declaration.
    }
}

namespace SET_VARS {

    void update_variable_id(const std::unordered_map<VariableID_type, VariableID_type>& old_to_new, AstElement& ast_element) {
        SetVarIdById set_vars(old_to_new);
        ast_element.accept(&set_vars);
    }

}

/**********************************************************************************************************************/

#ifndef NDEBUG

class SetVarIndexBase: public AstVisitor {
protected:
    void visit(Destination* destination) override;
    void visit(Edge* edge) override;
    void visit(LocationValueExpression* exp) override;

    SetVarIndexBase();
public:
    ~SetVarIndexBase() override = 0;
    DELETE_CONSTRUCTOR(SetVarIndexBase)
};

SetVarIndexBase::SetVarIndexBase() = default;

SetVarIndexBase::~SetVarIndexBase() = default;

void SetVarIndexBase::visit(Destination* destination) {
    PLAJA_ASSERT(automatonIndex != AUTOMATON::invalid)
    PLAJA_ASSERT(automatonIndex == destination->get_location_index()) // destination->set_destLocIndex(automatonIndex); // Set location index, note this assumes that automatonIndex = location variable index.
    if constexpr (PLAJA_GLOBAL::debug) { AstVisitor::visit(destination); } // No need to update subexpressions.
}

void SetVarIndexBase::visit(Edge* edge) {
    PLAJA_ASSERT(automatonIndex != AUTOMATON::invalid)
    PLAJA_ASSERT(edge->get_location_index() == automatonIndex) // edge->set_srcLocIndex(automatonIndex); // Set location index, note this assumes that automatonIndex = location variable index.
    if constexpr (PLAJA_GLOBAL::debug) { AstVisitor::visit(edge); }  // Actually no need to update sub-elements, besides destination.
    else { AST_ITERATOR(edge->destinationIterator(), Destination) }
}

void SetVarIndexBase::visit(LocationValueExpression* /*exp*/) { /* NOOP */ }

/**********************************************************************************************************************/

class SetVarIndexByID final: public SetVarIndexBase {
private:
    const std::vector<VariableIndex_type>* idToIndex;
    const Model* model;
    const ModelInformation* modelInfo; // if present check bounds variable-value expressions


    void visit(Destination* destination) override;
    void visit(Edge* edge) override;
    void visit(LocationValueExpression* exp) override;
    void visit(VariableExpression* exp) override;
    void visit(VariableValueExpression* exp) override;

public:
    explicit SetVarIndexByID(const std::vector<VariableIndex_type>& id_to_index, const ModelInformation* model_info);
    explicit SetVarIndexByID(const Model& model);
    ~SetVarIndexByID() override;
    DELETE_CONSTRUCTOR(SetVarIndexByID)

};

SetVarIndexByID::SetVarIndexByID(const std::vector<VariableIndex_type>& id_to_index, const ModelInformation* model_info):
    idToIndex(&id_to_index)
    , model(nullptr)
    , modelInfo(model_info) {
}

SetVarIndexByID::SetVarIndexByID(const Model& model):
    idToIndex(nullptr)
    , model(&model)
    , modelInfo(&model.get_model_information()) {}

SetVarIndexByID::~SetVarIndexByID() = default;

void SetVarIndexByID::visit(Destination* destination) { SetVarIndexBase::visit(destination); }

void SetVarIndexByID::visit(Edge* edge) { SetVarIndexBase::visit(edge); }

void SetVarIndexByID::visit(LocationValueExpression* exp) { SetVarIndexBase::visit(exp); }

void SetVarIndexByID::visit(VariableExpression* exp) {
    if constexpr (PLAJA_GLOBAL::debug) { // Should directly reference declaration.
        MAYBE_UNUSED(exp)
        if (idToIndex) {
            PLAJA_ASSERT(exp->get_index() == (*idToIndex)[exp->get_id()])
        } else {
            MAYBE_UNUSED(model)
            PLAJA_ASSERT(model)
            PLAJA_ASSERT(exp->get_index() == model->get_variable_index_by_id(exp->get_id()))
        }
    }
}

void SetVarIndexByID::visit(VariableValueExpression* exp) {
    AstVisitor::visit(exp);

    /* Check bounds if model info present. */
    if (modelInfo) {
        const VariableIndex_type var_index = exp->get_var()->get_variable_index();
        PLAJA_ASSERT(exp->get_val()->is_constant())
        if (not modelInfo->is_valid(var_index, exp->get_val()->evaluate_floating_const())) { throw SemanticsException(exp->to_string()); }
    }
}

namespace SET_VARS {

    void set_var_index_by_id(const std::vector<VariableIndex_type>& id_to_index, AstElement& ast_el, const ModelInformation* model_info) {
        SetVarIndexByID visitor(id_to_index, model_info);
        ast_el.accept(&visitor);
    }

    void set_var_index_by_id(const Model& model, AstElement& ast_el) {
        SetVarIndexByID visitor(model);
        ast_el.accept(&visitor);
    }

}

/**********************************************************************************************************************/

class SetVarIndexByIndex final: public SetVarIndexBase {
private:
    const std::vector<int>& indexToIndex; // mapping from old index to new

    void visit(Destination* destination) override;
    void visit(Edge* edge) override;
    void visit(LocationValueExpression* exp) override;
    void visit(VariableExpression* exp) override;

public:
    explicit SetVarIndexByIndex(const std::vector<int>& index_to_index);
    ~SetVarIndexByIndex() override;
    DELETE_CONSTRUCTOR(SetVarIndexByIndex)
};

SetVarIndexByIndex::SetVarIndexByIndex(const std::vector<int>& index_to_index):
    indexToIndex(index_to_index) {
}

SetVarIndexByIndex::~SetVarIndexByIndex() = default;

void SetVarIndexByIndex::visit(Destination* destination) { SetVarIndexBase::visit(destination); }

void SetVarIndexByIndex::visit(Edge* edge) { SetVarIndexBase::visit(edge); }

void SetVarIndexByIndex::visit(LocationValueExpression* exp) { SetVarIndexBase::visit(exp); }

void SetVarIndexByIndex::visit(VariableExpression* exp) {
    if constexpr (PLAJA_GLOBAL::debug) { // Should directly reference declaration.
        MAYBE_UNUSED(indexToIndex)
        MAYBE_UNUSED(exp)
        PLAJA_ASSERT(exp->get_index() == indexToIndex.at(exp->get_index()))
    }
}

namespace SET_VARS {

    void set_var_index_by_index(const std::vector<int>& index_to_index, AstElement& ast_el) {
        SetVarIndexByIndex visitor(index_to_index);
        ast_el.accept(&visitor);
    }

}

#endif
