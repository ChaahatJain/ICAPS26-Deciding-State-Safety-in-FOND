//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <list>
#include "is_constant.h"
#include "../ast/expression/non_standard/constant_array_access_expression.h"
#include "../ast/expression/array_access_expression.h"
#include "../ast/expression/constant_expression.h"
#include "../ast/expression/derivative_expression.h"
#include "../ast/iterators/model_iterator.h"
#include "../ast/assignment.h"
#include "../ast/automaton.h"
#include "../ast/constant_declaration.h"
#include "../ast/model.h"
#include "../ast/variable_declaration.h"
#include "ast_visitor.h"
#include "set_variable_index.h"

IsConstant::IsConstant(const Model& model) {
    model.accept(this);
}

IsConstant::~IsConstant() = default;

bool IsConstant::is_constant(VariableID_type var_id) const {
    return not nonConstantVars.count(var_id);
}

bool IsConstant::is_constant(VariableID_type var_id, const Model& model) {
    return IsConstant(model).is_constant(var_id);
}

/* */

void IsConstant::visit(const Assignment* assignment) {
    nonConstantVars.insert(assignment->get_ref()->get_variable_id());
}

void IsConstant::visit(const VariableDeclaration* var_decl) {
    if (var_decl->is_transient()) { nonConstantVars.insert(var_decl->get_id()); }
}

void IsConstant::visit(const DerivativeExpression* exp) {
    nonConstantVars.insert(exp->get_id());
}

/**********************************************************************************************************************/

class TransformConstVars final: public AstVisitor {
private:
    const std::unordered_map<VariableID_type, const ConstantDeclaration*>* idToConsts;

    void visit(ArrayAccessExpression* expr) override;
    void visit(VariableExpression* expr) override;
    void visit(DerivativeExpression* expr) override;

public:
    explicit TransformConstVars(const std::unordered_map<VariableID_type, const ConstantDeclaration*>& id_to_consts);
    ~TransformConstVars() override;
    DELETE_CONSTRUCTOR(TransformConstVars)

};

TransformConstVars::TransformConstVars(const std::unordered_map<VariableID_type, const ConstantDeclaration*>& id_to_consts):
    idToConsts(&id_to_consts) {
}

TransformConstVars::~TransformConstVars() = default;

void TransformConstVars::visit(ArrayAccessExpression* expr) {
    /* Index. */
    AST_ACCEPT_IF(expr->get_index, expr->set_index, Expression)

    /* Array. */
    auto it = idToConsts->find(expr->get_variable_id());
    if (it == idToConsts->end()) { return; }
    auto aa_const = std::make_unique<ConstantArrayAccessExpression>(it->second);
    aa_const->set_index(expr->set_index(nullptr));
    aa_const->determine_type();
    set_to_replace(std::move(aa_const));
}

void TransformConstVars::visit(VariableExpression* expr) {
    PLAJA_ASSERT_EXPECT(expr->get_index() == STATE_INDEX::none) // So far we transform during preprocessing only.
    auto it = idToConsts->find(expr->get_id());
    if (it == idToConsts->end()) { return; }
    const auto* const_decl = it->second;
    set_to_replace(std::make_unique<ConstantExpression>(*const_decl));
}

void TransformConstVars::visit(DerivativeExpression* expr) {
    MAYBE_UNUSED(expr)
    PLAJA_ASSERT(not idToConsts->count(expr->get_id()))
    return;
}

namespace SET_CONSTS {

    void transform_to_consts(const std::unordered_map<VariableID_type, const ConstantDeclaration*>& id_to_consts, AstElement& ast_element) {
        PLAJA_ASSERT(not PLAJA_UTILS::is_derived_type<VariableExpression>(ast_element)) // Should call on variable encapsulating structure.
        TransformConstVars transform_const_vars(id_to_consts);
        ast_element.accept(&transform_const_vars);
    }

    void transform_const_vars(Model& model) {

        const IsConstant is_constant(model);

        if (is_constant.get_non_constant_vars().size() == model.get_number_all_variables()) { return; }

        std::list<std::unique_ptr<VariableDeclaration>> transformed_vars; // Keep var declaration until var expressions are transformed.
        std::unordered_map<VariableID_type, const ConstantDeclaration*> id_to_consts;

        for (std::size_t index = 0; index < model.get_number_variables();) {
            const auto* var = model.get_variable(index);
            const auto id = var->get_id();

            if (is_constant.is_constant(id)) {
                transformed_vars.push_back(model.remove_variable(index));
                id_to_consts.emplace(id, model.transform_variable_to_const(*transformed_vars.back()));
                continue;
            } else { ++index; }

        }

        const auto num_instances = model.get_number_automataInstances();
        for (AutomatonIndex_type instance = 0; instance < num_instances; ++instance) {
            const auto* automaton = model.get_automatonInstance(instance);

            for (std::size_t index = 0; index < automaton->get_number_variables();) {
                const auto* var = automaton->get_variable(index);
                const auto id = var->get_id();

                if (is_constant.is_constant(id)) {
                    transformed_vars.push_back(model.remove_variable(instance, index));
                    id_to_consts.emplace(id, model.transform_variable_to_const(*transformed_vars.back()));
                    continue;
                } else { ++index; }

            }
        }

        if (transformed_vars.empty()) { return; }

        transform_to_consts(id_to_consts, model);

        /* Update variables ids. */
        std::unordered_map<VariableID_type, VariableID_type> old_to_new;
        VariableID_type var_id = 0;

        for (auto it = ModelIterator::globalVariableIterator(model); !it.end(); ++it) {
            if (var_id != it->get_id()) { old_to_new.emplace(it->get_id(), var_id); }
            ++var_id;
        }

        for (auto it_automaton = ModelIterator::automatonInstanceIterator(model); !it_automaton.end(); ++it_automaton) {

            for (auto it = it_automaton->variableIterator(); !it.end(); ++it) {
                if (var_id != it->get_id()) { old_to_new.emplace(it->get_id(), var_id); }
                ++var_id;
            }

        }

        SET_VARS::update_variable_id(old_to_new, model);

    }

}