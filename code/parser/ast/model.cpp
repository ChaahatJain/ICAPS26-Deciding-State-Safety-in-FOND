//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "model.h"
#include "../../exception/plaja_exception.h"
#include "../../include/ct_config_const.h"
#include "../../search/information/model_information.h"
#include "../../utils/utils.h"
#include "../visitor/extern/variables_of.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "expression/non_standard/location_value_expression.h"
#include "expression/array_access_expression.h"
#include "expression/variable_expression.h"
#include "type/declaration_type.h"
#include "action.h"
#include "automaton.h"
#include "composition.h"
#include "constant_declaration.h"
#include "edge.h"
#include "metadata.h"
#include "property.h"
#include "variable_declaration.h"

Model::Model(ModelType model_type):
    janiVersion(1)
    , name(PLAJA_UTILS::emptyString)
    , modelType(model_type)
    , modelFeatures()
    , actions()
    , constants()
    , variables()
    , restrictInitialExpression()
    CONSTRUCT_IF_COMMENT_PARSING(restrictInitialComment(PLAJA_UTILS::emptyString))
    , properties()
    , system()
    , automata()
    , automataInstances()
CONSTRUCT_IF_COMMENT_PARSING(metaData()) {
}

Model::~Model() = default;

// static:

namespace MODEL {

    const std::string modelTypeToStr[] { "lts", /*"dtmc", "ctmc",*/ "mdp", /*"ctmdp", "ma", "ta", "pta", "sta", "ha", "pha", "sha"*/ }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, Model::ModelType> strToModelType { { "lts", Model::LTS }, /*{"dtmc", Model::DTMC}, {"ctmc", Model::CTMC},*/
                                                                             { "mdp", Model::MDP }, /*{"ctmdp", Model::CTMDP}, {"ma", Model::MA}, {"ta", Model::TA},  {"pta", Model::PTA}, {"sta", Model::STA}, {"ha", Model::HA}, {"pha", Model::PHA}, {"sha", Model::SHA}*/ }; // NOLINT(cert-err58-cpp)

    const std::string modelFeatureToStr[] { "arrays", /*"datatypes",*/ "derived-operators", /*"edge-priorities", "functions", "hyperbolic-functions", "named-expressions", "nondet-selections",*/ "state-exit-rewards", /*"tradeoff-properties", "trigonometric-functions"*/ }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, Model::ModelFeature> strToModelFeature { { "arrays",             Model::ARRAYS }, /*{"datatypes", Model::DATATYPES},*/
                                                                                   { "derived-operators",  Model::DERIVED_OPERATORS }, /*{"edge-priorities", Model::EDGE_PRIORITIES}, {"functions", FUNCTIONS} , {"hyperbolic-functions", Model::HYPERBOLIC_FUNCTIONS}, {"named-expressions", Model::NAMED_EXPRESSIONS}, {"nondet-selections", NONDET_SELECTION},*/
                                                                                   { "state-exit-rewards", Model::STATE_EXIT_REWARDS }, /*{"tradeoff-properties", Model::TRADEOFF_PROPERTIES}, {"trigonometric-functions", Model::TRIGONOMETRIC_FUNCTIONS}*/ }; // NOLINT(cert-err58-cpp)
}

const std::string& Model::model_type_to_str(Model::ModelType type) { return MODEL::modelTypeToStr[type]; }

std::unique_ptr<Model::ModelType> Model::str_to_model_type(const std::string& type_str) {
    auto it = MODEL::strToModelType.find(type_str);
    if (it == MODEL::strToModelType.end()) { return nullptr; }
    else { return std::make_unique<ModelType>(it->second); }
}

const std::string& Model::model_feature_to_str(Model::ModelFeature feature) { return MODEL::modelFeatureToStr[feature]; }

std::unique_ptr<Model::ModelFeature> Model::str_to_model_feature(const std::string& feature_str) {
    auto it = MODEL::strToModelFeature.find(feature_str);
    if (it == MODEL::strToModelFeature.end()) { return nullptr; }
    else { return std::make_unique<ModelFeature>(it->second); }
}

// construction:

void Model::reserve(std::size_t model_features_cap, std::size_t actions_cap, std::size_t constants_cap, std::size_t variables_cap, std::size_t properties_cap) {
    modelFeatures.reserve(model_features_cap);
    actions.reserve(actions_cap);
    constants.reserve(constants_cap);
    variables.reserve(variables_cap);
    properties.reserve(properties_cap);
}

void Model::reserve_automata(std::size_t automata_cap, std::size_t automata_instances_cap) {
    automata.reserve(automata_cap);
    automataInstances.reserve(automata_instances_cap);
}

void Model::add_model_feature(ModelFeature model_feature) { modelFeatures.push_back(model_feature); }

void Model::add_action(std::unique_ptr<Action>&& action) { actions.push_back(std::move(action)); }

void Model::add_constant(std::unique_ptr<ConstantDeclaration>&& constant) { constants.push_back(std::move(constant)); }

void Model::add_variable(std::unique_ptr<VariableDeclaration>&& variable) { variables.push_back(std::move(variable)); }

void Model::add_property(std::unique_ptr<Property>&& prop) { properties.push_back(std::move(prop)); }

void Model::add_automaton(AutomatonIndex_type instance_index) {
    PLAJA_ASSERT(0 <= instance_index && instance_index < automataInstances.size())
    automata.push_back(instance_index);
}

void Model::add_automatonInstance(std::unique_ptr<Automaton>&& automaton_instance) {
    PLAJA_ASSERT(automaton_instance->get_index() == automataInstances.size())
    automataInstances.push_back(std::move(automaton_instance));
}


// setter:

[[maybe_unused]] void Model::set_action(std::unique_ptr<Action>&& action, std::size_t index) {
    PLAJA_ASSERT(index < actions.size())
    actions[index] = std::move(action);
}

[[maybe_unused]] std::unique_ptr<ConstantDeclaration> Model::set_constant(std::unique_ptr<ConstantDeclaration>&& constant, std::size_t index) {
    PLAJA_ASSERT(index < constants.size())
    std::swap(constants[index], constant);
    return std::move(constant);
}

std::unique_ptr<VariableDeclaration> Model::set_variable(std::unique_ptr<VariableDeclaration>&& var, std::size_t index) {
    PLAJA_ASSERT(index < variables.size())
    std::swap(variables[index], var);
    return std::move(var);
}

void Model::set_restrict_initial_expression(std::unique_ptr<Expression>&& restrict_init_exp) { restrictInitialExpression = std::move(restrict_init_exp); }

[[maybe_unused]] void Model::set_property(std::unique_ptr<Property>&& prop, std::size_t index) {
    PLAJA_ASSERT(index < properties.size())
    properties[index] = std::move(prop);
}

void Model::set_system(std::unique_ptr<Composition>&& sys) { system = std::move(sys); }

[[maybe_unused]] void Model::set_automaton(AutomatonIndex_type instance, std::size_t index) {
    PLAJA_ASSERT(index < automata.size())
    automata[index] = instance;
}

[[maybe_unused]] void Model::set_automatonInstance(std::unique_ptr<Automaton>&& automaton_instance, AutomatonIndex_type instance_index) {
    PLAJA_ASSERT(automaton_instance->get_index() == instance_index)
    PLAJA_ASSERT(instance_index < automataInstances.size())
    automataInstances[instance_index] = std::move(automaton_instance);
}

void Model::set_metadata(std::unique_ptr<Metadata>&& meta_data) { SET_IF_COMMENT_PARSING(metaData, std::move(meta_data)) }

/* getter */

const Automaton* Model::get_automaton(std::size_t index) const {
    const AutomatonIndex_type automaton_index = automata[index];
    PLAJA_ASSERT(automaton_index != AUTOMATON::invalid)
    PLAJA_ASSERT(0 <= automaton_index && automaton_index < automataInstances.size())
    return automataInstances[automaton_index].get();
}

#ifndef NDEBUG

const Automaton* Model::get_automaton_by_name(const std::string& automaton_name) const {
    for (const auto& automaton: automataInstances) {
        if (automaton->get_name() == automaton_name) { return automaton.get(); }
    }
    throw PlajaException("No automaton with name: " + automaton_name);
}

#endif

//

void Model::compute_model_information() { modelInformation = std::make_unique<ModelInformation>(*this); }

std::unique_ptr<Model> Model::deep_copy() const {
    PLAJA_LOG_DEBUG(PLAJA_UTILS::to_red_string("Warning: Deep copy model might invalidate child-to-parent references."))
    PLAJA_ABORT_IF(not PLAJA_GLOBAL::debug) // Probably not used anyway. Crash for now.

    std::unique_ptr<Model> copy(new Model(modelType));
    copy->janiVersion = janiVersion;
    copy->name = name;
    copy->modelFeatures = modelFeatures;

    copy->actions.reserve(actions.size());
    for (const auto& action: actions) { copy->actions.emplace_back(action->deepCopy()); }

    copy->constants.reserve(constants.size());
    for (const auto& constant: constants) { copy->constants.emplace_back(constant->deep_copy()); }

    copy->variables.reserve(variables.size());
    for (const auto& var: variables) { copy->variables.emplace_back(var->deep_copy()); }

    if (restrictInitialExpression) { copy->restrictInitialExpression = restrictInitialExpression->deepCopy_Exp(); }
    COPY_IF_COMMENT_PARSING(copy, restrictInitialComment)

    copy->properties.reserve(properties.size());
    for (const auto& prop: properties) { copy->properties.emplace_back(prop->deepCopy()); }

    if (system) { copy->system = system->deepCopy(); }

    copy->automata.reserve(automata.size());
    for (const AutomatonIndex_type automaton: automata) { copy->automata.push_back(automaton); }

    copy->automataInstances.reserve(automataInstances.size());
    for (const auto& automaton: automataInstances) { copy->automataInstances.emplace_back(automaton->deepCopy()); }

    COPY_IF_COMMENT_PARSING(copy, metaData->deepCopy())

    return copy;
}

/* override */

void Model::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void Model::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

/* further auxiliaries */

const std::string& Model::get_action_name(ActionID_type action_id) const {
    return ACTION::silentAction == action_id ? ACTION::silentName : get_action(action_id)->get_name();
}

VariableDeclaration* Model::get_variable_by_id(VariableID_type id) {
    VariableID_type id_tmp = id;
    /* Globals. */
    if (id_tmp < variables.size()) { return variables[id_tmp].get(); }
    /* Locals. */
    id_tmp = id_tmp - variables.size();
    const std::size_t l = automataInstances.size();
    for (std::size_t i = 0; i < l; ++i) {
        if (id_tmp < automataInstances[i]->get_number_variables()) { return automataInstances[i]->get_variable(id_tmp); }
        id_tmp = id_tmp - automataInstances[i]->get_number_variables();
    }
    return nullptr;
}

std::unique_ptr<VariableDeclaration> Model::remove_variable(std::size_t index) {
    PLAJA_ASSERT(get_variable(index))
    auto it = variables.begin() + PLAJA_UTILS::cast_numeric<long>(index);
    PLAJA_ASSERT(it->get() == get_variable(index))
    std::unique_ptr<VariableDeclaration> var_removed = std::move(*it);
    variables.erase(it);
    return var_removed;
}

std::unique_ptr<VariableDeclaration> Model::remove_variable(AutomatonIndex_type automaton_index, std::size_t index) {
    auto* automaton = get_automatonInstance(automaton_index);
    return automaton->remove_variable(index);
}

std::unique_ptr<ConstantDeclaration> Model::transform_variable_to_const(VariableDeclaration& var, ConstantIdType id) {
    auto transformed_const = std::make_unique<ConstantDeclaration>(id);
    transformed_const->set_name(var.get_name());
    transformed_const->set_declaration_type(var.set_declarationType(nullptr));
    transformed_const->set_value(var.set_initialValue(nullptr));
    return transformed_const;
}

const ConstantDeclaration* Model::transform_variable_to_const(VariableDeclaration& var) {
    auto transformed_const = transform_variable_to_const(var, constants.size());
    add_constant(std::move(transformed_const));
    return constants.back().get();
}

std::size_t Model::get_number_all_variables() const {
    std::size_t rlt = variables.size();
    for (const auto& automaton: automataInstances) {
        rlt += automaton->get_number_variables();
    }
    return rlt;
}

const VariableDeclaration* Model::get_variable_by_id(VariableID_type id) const {
    VariableID_type id_tmp = id;
    // globals:
    if (id_tmp < variables.size()) { return variables[id_tmp].get(); }
    // locals:
    id_tmp = id_tmp - variables.size();
    const auto l = automataInstances.size();
    for (std::size_t i = 0; i < l; ++i) {
        if (id_tmp < automataInstances[i]->get_number_variables()) { return automataInstances[i]->get_variable(id_tmp); }
        id_tmp = id_tmp - automataInstances[i]->get_number_variables();
    }
    return nullptr;
}

VariableIndex_type Model::get_variable_index_by_id(VariableID_type id) const { return get_variable_by_id(id)->get_index(); }

AutomatonIndex_type Model::get_variable_automaton_instance(VariableID_type id) const {
    VariableID_type id_tmp = id;
    // globals:
    if (id_tmp < variables.size()) { return AUTOMATON::invalid; }
    // locals:
    id_tmp = id_tmp - variables.size();
    const auto l = automataInstances.size();
    for (std::size_t i = 0; i < l; ++i) {
        if (id_tmp < automataInstances[i]->get_number_variables()) { return i; }
        id_tmp = id_tmp - automataInstances[i]->get_number_variables();
    }
    PLAJA_ABORT
}

#ifndef NDEBUG

const VariableDeclaration& Model::get_variable_by_name(const std::string& var_name) const {
    for (auto it = variableIterator(); !it.end(); ++it) {
        const auto* var = it.variable();
        if (var->get_name() == var_name) { return *var; }
    }
    throw PlajaException("No variable with name: " + var_name);
}

VariableID_type Model::get_variable_id_by_name(const std::string& var_name) const {
    return get_variable_by_name(var_name).get_id();
}

VariableID_type Model::get_variable_index_by_name(const std::string& var_name) const {
    return get_variable_by_name(var_name).get_index();
}

#endif

#ifdef RUNTIME_CHECKS

const VariableDeclaration& Model::get_variable_by_index(VariableIndex_type var_index) const {
#if 0
    VariableIndex_type var_index_ref = get_number_automataInstances();
    PLAJA_ASSERT(var_index >= var_index_ref)
    for (auto it = variableIterator(); !it.end(); ++it) {
        const auto* var = it.variable();
        var_index_ref += var->get_size();
        if (var_index_ref > var_index) { return *var; }
    }
    PLAJA_ABORT
#else
    PLAJA_ASSERT(modelInformation)
    return *get_variable_by_id(modelInformation->get_id(var_index));
#endif
}

std::string Model::gen_state_index_str(VariableIndex_type index) const {
    if (index < get_number_automataInstances()) {
        return get_automatonInstance(PLAJA_UTILS::cast_numeric<AutomatonIndex_type>(index))->get_name();
    } else {
        const auto& var_decl = get_variable_by_index(index);
        if (var_decl.is_array()) { return var_decl.get_name() + " (" + std::to_string(index - var_decl.get_index()) + " )"; }
        else { return var_decl.get_name(); }
    }
}

#endif

std::unique_ptr<Expression> Model::gen_loc_value_expr(AutomatonIndex_type location, PLAJA::integer value) const {
    const auto* automaton = get_automatonInstance(location);
    PLAJA_ASSERT(automaton)
    return std::make_unique<LocationValueExpression>(*automaton, automaton->get_location(value));
}

std::unique_ptr<Expression> Model::gen_var_expr(VariableIndex_type state_index, const VariableDeclaration* var_decl) const {
    PLAJA_ASSERT(modelInformation)
    if (not var_decl) { var_decl = get_variable_by_id(modelInformation->get_id(state_index)); }

    PLAJA_ASSERT(state_index >= var_decl->get_index());

    if (var_decl->is_array()) {
        auto aa = std::make_unique<ArrayAccessExpression>();
        aa->set_accessedArray(*var_decl);
        aa->set_index(state_index - var_decl->get_index());
        return aa;
    } else {
        return std::make_unique<VariableExpression>(*var_decl);
    }

}

std::unordered_set<VariableIndex_type> Model::collect_guard_variables(bool include_locs) const {

    std::unordered_set<VariableIndex_type> guard_vars;

    for (const auto& automaton_instance: automataInstances) {

        for (auto it = automaton_instance->edgeIterator(); !it.end(); ++it) {
            PLAJA_ASSERT(it())

            if (include_locs) { guard_vars.insert(it->get_location_index()); }

            if (it->get_guardExpression()) {
                guard_vars.merge(VARIABLES_OF::state_indexes_of(it->get_guardExpression(), false, false));
            }

        }

    }

    return guard_vars;

}

// const VariableIndex_type iterators

VariableIndex_type Model::VariableIterator::variable_index() const { return variable()->get_index(); }


