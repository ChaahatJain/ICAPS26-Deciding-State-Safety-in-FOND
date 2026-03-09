//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "model_information.h"
#include "../../exception/semantics_exception.h"
#include "../../include/ct_config_const.h"
#include "../../option_parser/option_parser.h"
#include "../../option_parser/plaja_options.h"
#include "../../parser/ast/expression/array_value_expression.h"
#include "../../parser/ast/expression/expression_utils.h"
#include "../../parser/ast/iterators/model_iterator.h"
#include "../../parser/ast/type/array_type.h"
#include "../../parser/ast/type/basic_type.h"
#include "../../parser/ast/type/bounded_type.h"
#include "../../parser/visitor/extern/ast_optimization.h"
#include "../../parser/visitor/extern/ast_specialization.h"
#include "../../parser/visitor/set_constants.h"
#include "../../parser/visitor/set_variable_index.h"
#include "../../parser/ast/action.h"
#include "../../parser/ast/automaton.h"
#include "../../parser/ast/composition.h"
#include "../../parser/ast/constant_declaration.h"
#include "../../parser/ast/edge.h"
#include "../../parser/ast/model.h"
#include "../../parser/ast/synchronisation.h"
#include "../../parser/ast/variable_declaration.h"
#include "../../parser/visitor/is_constant.h"
#include "../../utils/floating_utils.h"
#include "../states/state_values.h"

/* Static. */

const ModelInformation& ModelInformation::get_model_info(const Model& model) { return model.get_model_information(); }

/* */

ModelInformation::ModelInformation(Model& model):
    syncInformation(model)
    , syncOpFactor(0)
    , updateOpFactor(0) {
    compute_constant_information(model);
    compute_variable_information(model);
    AST_OPTIMIZATION::optimize_ast(model); // after setting constants and variables we optimize the AST -- *before* assigning edge/sync info ...
    AST_SPECIALIZATION::specialize_ast(model);
    compute_edge_and_sync_information(model);
}

ModelInformation::~ModelInformation() = default;

// auxiliary:

void ModelInformation::add_boolean_information() {
    lowerBoundsInt.push_back(0);
    upperBoundsInt.push_back(1);
    hashFactorsInt.push_back(hashFactorsInt.back() * rangesInt.back()); // ranges, hash-factor from the *last*
    rangesInt.push_back(2);
}

void ModelInformation::add_bounded_int_information(PLAJA::integer lower_bound, PLAJA::integer upper_bound) {
    lowerBoundsInt.push_back(lower_bound);
    upperBoundsInt.push_back(upper_bound);
    hashFactorsInt.push_back(hashFactorsInt.back() * rangesInt.back()); // ranges, hash-factor from the *last*
    rangesInt.push_back(upperBoundsInt.back() - lowerBoundsInt.back() + 1); // *Note*, this is not just the difference but one must add *+1* // TODO handle overflow ?
}

void ModelInformation::add_bounded_float_information(PLAJA::floating lower_bound, PLAJA::floating upper_bound) {
    lowerBoundsFloat.push_back(lower_bound);
    upperBoundsFloat.push_back(upper_bound);
}

// main computations:

void ModelInformation::compute_variable_information(Model& model) {
    std::vector<VariableIndex_type> var_id_to_index;

    const auto num_automata_instances = get_number_automata_instances();

    /* Cache initial values. */
    std::vector<PLAJA::integer> initial_values_int;
    std::vector<PLAJA::floating> initial_values_float;

    /* Locations. */
    for (AutomatonIndex_type automaton_index = 0; automaton_index < num_automata_instances; ++automaton_index) {
        const Automaton* automaton = model.get_automatonInstance(automaton_index);
        initial_values_int.push_back(automaton->get_initialLocation(0)); // currently exactly one
        lowerBoundsInt.push_back(0);
        upperBoundsInt.push_back(PLAJA_UTILS::cast_numeric<int>(automaton->get_number_locations()) - 1);
        hashFactorsInt.push_back(hashFactorsInt.empty() ? 1 : (hashFactorsInt.back() * rangesInt.back()));
        rangesInt.push_back(PLAJA_UTILS::cast_numeric<int>(automaton->get_number_locations()));
        varIndexToId.push_back(VARIABLE::none);
    }

    /* Compute actual variable information. */
    VariableIndex_type var_index = get_automata_offset(); // the state index of a variable, for actual vars start with automata offset
    for (auto var_it = model.variableIterator(); !var_it.end(); ++var_it) {

        auto* var_decl = model.get_variable_by_id(var_it.variable_id()); // Cannot use iterator access to variable due to "const".
        const auto* type = var_decl->get_type();

        var_decl->set_index(var_index);
        if constexpr (PLAJA_GLOBAL::debug) { var_id_to_index.push_back(var_index); } // Legacy: To set var index in var expressions, by now the latter directly reference var declarations.

        if (type->is_array_type()) { // Array type.
            PLAJA_ASSERT(var_decl->get_initialValue())
            JANI_ASSERT(var_decl->get_initialValue()->is_constant())
            const auto* element_type = PLAJA_UTILS::cast_ptr<ArrayType>(type)->get_element_type();

            if (element_type->is_floating_type()) {

                auto array_values = var_decl->get_initialValue()->evaluate_floating_array_const();
                const auto ll = array_values.size();
                var_decl->set_size(ll); // set size
                arraySizes[var_decl->get_id()] = ll; // store array size
                var_index += ll; // update variable index
                for (auto val: array_values) { // initial values and index-2-id
                    initial_values_float.push_back(val);
                    varIndexToId.push_back(var_it.variable_id());
                }

                /* Additional data. */
                PLAJA_ASSERT(element_type->is_bounded_type())
                const auto* bounded_type = PLAJA_UTILS::cast_ptr<BoundedType>(element_type);
                JANI_ASSERT(bounded_type->get_lower_bound() and bounded_type->get_upper_bound())
                const auto lower_bound = bounded_type->get_lower_bound()->evaluate_floating_const();
                const auto upper_bound = bounded_type->get_upper_bound()->evaluate_floating_const();
                if (not PLAJA_FLOATS::lt(lower_bound, upper_bound, PLAJA::floatingPrecision)) { throw SemanticsException(type->to_string()); }
                for (auto val: array_values) {
                    if (PLAJA_FLOATS::lt(val, lower_bound) or PLAJA_FLOATS::lt(upper_bound, val)) { throw SemanticsException(var_decl->to_string()); }
                    add_bounded_float_information(lower_bound, upper_bound);
                }

            } else {

                auto array_values = var_decl->get_initialValue()->evaluate_integer_array_const();
                const auto ll = array_values.size();
                var_decl->set_size(ll); // set size
                arraySizes[var_decl->get_id()] = ll; // store array size
                var_index += ll; // update variable index
                for (auto val: array_values) { // initial values and index-2-id
                    initial_values_int.push_back(val);
                    varIndexToId.push_back(var_it.variable_id());
                }

                /* Additional data. */
                if (DeclarationType::Bool == element_type->get_kind()) {
                    for (std::size_t j = 0; j < ll; ++j) { add_boolean_information(); }
                } else if (DeclarationType::Bounded == element_type->get_kind()) {
                    const auto* bounded_type = PLAJA_UTILS::cast_ptr<BoundedType>(element_type);
                    JANI_ASSERT(bounded_type->get_lower_bound() and bounded_type->get_upper_bound())
                    const auto lower_bound = bounded_type->get_lower_bound()->evaluate_integer_const();
                    const auto upper_bound = bounded_type->get_upper_bound()->evaluate_integer_const();
                    if (upper_bound <= lower_bound) { throw SemanticsException(type->to_string()); }
                    for (auto val: array_values) {
                        if (val < lower_bound or upper_bound < val) { throw SemanticsException(var_decl->to_string()); }
                        add_bounded_int_information(lower_bound, upper_bound);
                    }

                } else {
                    PLAJA_ABORT
                }
            }

            /**********************************************************************************************************/
        } else { // Trivial types.

            PLAJA_ASSERT(type->is_trivial_type())
            var_decl->set_size(1); // set size
            varIndexToId.push_back(var_it.variable_id());
            ++var_index; // update variable index;

            // initial value:
            JANI_ASSERT(var_decl->get_initialValue())
            if (type->is_floating_type()) { initial_values_float.push_back(var_decl->get_initialValue()->evaluate_floating_const()); }
            else { initial_values_int.push_back(var_decl->get_initialValue()->evaluate_integer_const()); }

            /* Additional data. */
            if (type->is_boolean_type()) { add_boolean_information(); }
            else if (type->is_bounded_type()) {

                const auto* bounded_type = PLAJA_UTILS::cast_ptr<BoundedType>(type);
                JANI_ASSERT(bounded_type->get_lower_bound() and bounded_type->get_upper_bound())

                if (bounded_type->is_floating_type()) {
                    const auto lower_bound = bounded_type->get_lower_bound()->evaluate_floating_const();
                    const auto upper_bound = bounded_type->get_upper_bound()->evaluate_floating_const();
                    if (not PLAJA_FLOATS::lt(lower_bound, upper_bound, PLAJA::floatingPrecision)) { throw SemanticsException(type->to_string()); }
                    if (PLAJA_FLOATS::lt(initial_values_float.back(), lower_bound) or PLAJA_FLOATS::gt(initial_values_float.back(), upper_bound)) { throw SemanticsException(var_decl->to_string()); }
                    add_bounded_float_information(lower_bound, upper_bound);
                } else {
                    PLAJA_ASSERT(bounded_type->is_integer_type())
                    const auto lower_bound = bounded_type->get_lower_bound()->evaluate_integer_const();
                    const auto upper_bound = bounded_type->get_upper_bound()->evaluate_integer_const();
                    if (upper_bound <= lower_bound) { throw SemanticsException(type->to_string()); }
                    if (initial_values_int.back() < lower_bound or upper_bound < initial_values_int.back()) { throw SemanticsException(var_decl->to_string()); }
                    add_bounded_int_information(lower_bound, upper_bound);
                }

            } else { PLAJA_ABORT }

        }

        /* Transient variables. */
        if (var_decl->is_transient()) { transientVariables.push_back(var_decl->get_index()); }
    }

    lowerBoundsInt.shrink_to_fit();
    upperBoundsInt.shrink_to_fit();
    rangesInt.shrink_to_fit();
    hashFactorsInt.shrink_to_fit();
    lowerBoundsFloat.shrink_to_fit();
    upperBoundsFloat.shrink_to_fit();

    initialValues = std::make_unique<StateValues>(initial_values_int, initial_values_float); // model's initial state
    varIndexToId.shrink_to_fit();

    if constexpr (PLAJA_GLOBAL::debug) { SET_VARS::set_var_index_by_id(var_id_to_index, model, this); } // Modify ast, and check bounds in variable-value expressions.
}

void ModelInformation::compute_constant_information(Model& model) {

    if (PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::const_vars_to_consts)) { SET_CONSTS::transform_const_vars(model); }

    std::unordered_map<ConstantIdType, const ConstantDeclaration*> const_to_value; // Mapping. // Legacy, constant occurrences directly reference declaration.

    for (auto it = ModelIterator::constantIterator(model); !it.end(); ++it) {

        if constexpr (PLAJA_GLOBAL::debug) { SET_CONSTANTS::set_constants(const_to_value, *it); } // If constant refers to other constants with smaller index.

        auto& const_decl = *it;
        JANI_ASSERT(const_decl.get_type() and const_decl.get_value())
        const auto* type = const_decl.get_type();

        if (DeclarationType::Array == type->get_kind()) { // Array type.

            const auto* array_type = PLAJA_UTILS::cast_ptr<ArrayType>(type);
            const auto* element_type = array_type->get_element_type();
            JANI_ASSERT(element_type)

            auto array_value_expr = std::make_unique<ArrayValueExpression>();

            if (element_type->is_floating_type()) {

                auto array_value = const_decl.get_value()->evaluate_floating_array_const();
                array_value_expr->reserve(array_value.size());
                for (auto value: array_value) { array_value_expr->add_element(PLAJA_EXPRESSION::make_real(value)); }

                if (element_type->is_bounded_type()) {
                    const auto* bounded_type = PLAJA_UTILS::cast_ptr<BoundedType>(element_type);
                    const auto lower_bound = bounded_type->get_lower_bound()->evaluate_floating_const();
                    const auto upper_bound = bounded_type->get_upper_bound()->evaluate_floating_const();
                    if (PLAJA_FLOATS::lt(upper_bound, lower_bound, PLAJA::floatingPrecision)) { throw SemanticsException(type->to_string()); }

                    for (auto value: array_value) {
                        if (PLAJA_FLOATS::lt(value, lower_bound) or PLAJA_FLOATS::gt(upper_bound, value)) { throw SemanticsException(const_decl.to_string()); }
                    }
                }

            } else if (element_type->is_integer_type()) {

                auto array_value = const_decl.get_value()->evaluate_integer_array_const();
                array_value_expr->reserve(array_value.size());
                for (auto value: array_value) { array_value_expr->add_element(PLAJA_EXPRESSION::make_int(value)); }

                if (element_type->is_bounded_type()) {
                    const auto* bounded_type = PLAJA_UTILS::cast_ptr<BoundedType>(element_type);
                    const auto lower_bound = bounded_type->get_lower_bound()->evaluate_integer_const();
                    const auto upper_bound = bounded_type->get_upper_bound()->evaluate_integer_const();
                    if (upper_bound < lower_bound) { throw SemanticsException(type->to_string()); }

                    for (auto value: array_value) {
                        if (value < lower_bound or upper_bound < value) { throw SemanticsException(const_decl.to_string()); }
                    }
                }

            } else if (element_type->is_boolean_type()) {

                auto array_value = const_decl.get_value()->evaluate_integer_array_const();
                array_value_expr->reserve(array_value.size());
                for (auto value: array_value) { array_value_expr->add_element(PLAJA_EXPRESSION::make_bool(value)); }

            } else {
                PLAJA_ABORT
            }

            array_value_expr->determine_type();
            PLAJA_ASSERT(array_type->is_assignable_from(*array_value_expr->get_type()))
            const_decl.set_value(std::move(array_value_expr));

            constants.push_back(const_decl.get_id()); // Array constants are no inline.

            /**********************************************************************************************************/
        } else if (DeclarationType::Bounded == type->get_kind()) { // Bounded type.

            const auto* bounded_type = PLAJA_UTILS::cast_ptr<BoundedType>(type);

            if (DeclarationType::Int == bounded_type->get_base()->get_kind()) {

                const auto value = const_decl.get_value()->evaluate_integer_const();
                const_decl.set_value(PLAJA_EXPRESSION::make_int(value));

                const auto lower_bound = bounded_type->get_lower_bound()->evaluate_integer_const();
                const auto upper_bound = bounded_type->get_upper_bound()->evaluate_integer_const();
                if (upper_bound < lower_bound) { throw SemanticsException(type->to_string()); }
                if (value < lower_bound or upper_bound < value) { throw SemanticsException(const_decl.to_string()); }

            } else if (DeclarationType::Real == bounded_type->get_base()->get_kind()) {

                const auto value = const_decl.get_value()->evaluate_floating_const();
                const_decl.set_value(PLAJA_EXPRESSION::make_real(value));

                const auto lower_bound = bounded_type->get_lower_bound()->evaluate_floating_const();
                const auto upper_bound = bounded_type->get_upper_bound()->evaluate_floating_const();
                if (PLAJA_FLOATS::lt(upper_bound, lower_bound, PLAJA::floatingPrecision)) { throw SemanticsException(type->to_string()); }
                if (PLAJA_FLOATS::lt(value, lower_bound) or PLAJA_FLOATS::lt(upper_bound, value)) { throw SemanticsException(const_decl.to_string()); }

            } else { PLAJA_ABORT }

        } else { // Basic type.

            switch (type->get_kind()) {

                case DeclarationType::Bool: {
                    const_decl.set_value(PLAJA_EXPRESSION::make_bool(const_decl.get_value()->evaluate_integer_const()));
                    break;
                }

                case DeclarationType::Int: {
                    const_decl.set_value(PLAJA_EXPRESSION::make_int(const_decl.get_value()->evaluate_integer_const()));
                    break;
                }

                case DeclarationType::Real: {
                    const_decl.set_value(PLAJA_EXPRESSION::make_real(const_decl.get_value()->evaluate_floating_const()));
                    break;
                }

                default: { PLAJA_ABORT }
            }
        }

        if constexpr (PLAJA_GLOBAL::debug) { const_to_value.emplace(const_decl.get_id(), &const_decl); }

    }

    if constexpr (PLAJA_GLOBAL::debug) { SET_CONSTANTS::set_constants(const_to_value, model); } // Modify ast.

}

void ModelInformation::compute_edge_and_sync_information(Model& model) {
    static_assert(ACTION::silentAction == -1);
    static_assert(EDGE::nullEdge == 0);

    // Sort edges per automaton per action:
    std::vector<std::vector<std::list<Edge*>>> edges_per_automaton_per_action(model.get_number_automataInstances());
    for (auto& per_automaton: edges_per_automaton_per_action) {
        per_automaton.resize(model.get_number_actions() + 1); // +1 due to silent action
    }
    std::size_t j, l;
    for (AutomatonIndex_type automaton_index = 0; automaton_index < syncInformation.num_automata_instances; ++automaton_index) {
        Automaton* automaton_instance = model.get_automatonInstance(automaton_index);
        l = automaton_instance->get_number_edges();
        for (j = 0; j < l; ++j) {
            Edge* edge = automaton_instance->get_edge(j);
            PLAJA_ASSERT(edge->get_action_id() != ACTION::nullAction)
            edges_per_automaton_per_action[automaton_index][edge->get_action_id() + 1 /*for silent action*/].push_back(edge);
        }
    }

    // Set EdgeID tight w.r.t. ActionID per automaton:
    for (auto& per_automaton: edges_per_automaton_per_action) {
        EdgeID_type edge_id = 1; // initialized with 1 since 0 is reserved for null-edge
        for (auto& per_action: per_automaton) {
            for (auto* edge: per_action) {
                edge->set_id(edge_id++);
            }
        }
    }

    // Set SyncID:
    Composition* system = model.get_system();
    l = system->get_number_syncs();
    for (SyncIndex_type sync_index = 0; sync_index < l; ++sync_index) {
        system->get_sync(sync_index)->set_syncID(sync_index_to_id(sync_index));
    }

    // compute action operator info:
    compute_action_op_index_information(model, edges_per_automaton_per_action);
}

void ModelInformation::compute_action_op_index_information(Model& model, const std::vector<std::vector<std::list<Edge*>>>& edges_per_automaton_per_action) {
    static_assert(ACTION::silentAction == -1);
    static_assert(EDGE::nullEdge == 0);

    // non-sync offset:
    const auto num_automata_instances = get_number_automata_instances();
    nonSyncOffsets.resize(num_automata_instances, 0);
    AutomatonIndex_type automaton_index; // NOLINT(*-init-variables)
    for (automaton_index = 1; automaton_index < num_automata_instances; ++automaton_index) {
        nonSyncOffsets[automaton_index] = nonSyncOffsets[automaton_index - 1] + PLAJA_UTILS::cast_numeric<int>(edges_per_automaton_per_action[automaton_index - 1][ACTION::silentAction + 1].size());
    }
    syncOpFactor = nonSyncOffsets.back() + (edges_per_automaton_per_action.back()[ACTION::silentAction + 1]).size(); // update action op id info: 1 + max(operator index)
    for (automaton_index = 0; automaton_index < num_automata_instances; ++automaton_index) { --nonSyncOffsets[automaton_index]; } // to account for null-edge

    // sync factors:
    auto* system = model.get_system();
    auto num_syncs = system->get_number_syncs();
    perSyncOpOffsetsAndFactors.resize(num_syncs);
    for (SyncIndex_type sync_index = 0; sync_index < num_syncs; ++sync_index) { // for each sync ...
        perSyncOpOffsetsAndFactors[sync_index].resize(num_automata_instances, { 0, 0 });
        unsigned int sync_factor_acc = 1;
        const Synchronisation* sync = system->get_sync(sync_index);
        PLAJA_ASSERT(sync->get_size_synchronise() == num_automata_instances)
        for (automaton_index = 0; automaton_index < num_automata_instances; ++automaton_index) { // for each automaton ...
            const ActionID_type action_id = sync->get_syncActionID(automaton_index);
            PLAJA_ASSERT(action_id != ACTION::silentAction)
            if (action_id != ACTION::nullAction) {  // if it participates ...
                std::list<Edge*> edges = edges_per_automaton_per_action[automaton_index][action_id + 1/*for silent action*/];
                if (edges.empty()) { // special case: sync. directive may specify automata without a respectively labeled edge
                    perSyncOpOffsetsAndFactors[sync_index][automaton_index] = std::make_pair(0 /*minimal id*/, sync_factor_acc);
                } else { // intuitively expected case: there are respectively labeled edges:
                    perSyncOpOffsetsAndFactors[sync_index][automaton_index] = std::make_pair(edges.front()->get_id() /*minimal id*/, sync_factor_acc);
                    sync_factor_acc *= edges.size(); // update sync-factor
                }
            }
        }

        syncOpFactor = std::max(syncOpFactor, sync_factor_acc); // update action op id info: 1 + max(operator index)
    }

    updateOpFactor = syncOpFactor * (num_syncs + 1); // set action op update info: max(action op id) + 1

}

/* */

PLAJA::integer ModelInformation::get_initial_value_int(VariableIndex_type state_index) const { return initialValues->get_int(state_index); }

PLAJA::floating ModelInformation::get_initial_value_float(VariableIndex_type state_index) const { return initialValues->get_float(state_index); }

PLAJA::floating ModelInformation::get_initial_value(VariableIndex_type state_index) const { return initialValues->operator[](state_index); }

bool ModelInformation::is_valid(VariableIndex_type state_index, PLAJA::floating value) const {
    return PLAJA_FLOATS::lte(get_lower_bound(state_index), value, PLAJA::floatingPrecision) and PLAJA_FLOATS::lte(value, get_upper_bound(state_index), PLAJA::floatingPrecision);
}

bool ModelInformation::is_valid(VariableIndex_type state_index, PLAJA::integer value) const {
    return get_lower_bound_int(state_index) <= value and value <= get_upper_bound_int(state_index);
}

bool ModelInformation::is_valid(const StateValues& state) const {

    for (auto it = stateIndexIteratorInt(true); !it.end(); ++it) {
        const PLAJA::integer value = state.get_int(it.state_index());
        if (value < it.lower_bound_int() or it.upper_bound_int() < value) { return false; }
    }

    for (auto it = stateIndexIteratorFloat(); !it.end(); ++it) {
        if (not is_valid(it.state_index(), state.get_float(it.state_index()))) { return false; }
    }

    return true;
}

#ifndef NDEBUG

bool ModelInformation::is_valid(const StateBase& state) const {
    // TODO: Quick fix, code duplicate of StateValues specific function.

    for (auto it = stateIndexIteratorInt(true); !it.end(); ++it) {
        if (not is_valid(it.state_index(), state.get_int(it.state_index()))) { return false; }
    }

    for (auto it = stateIndexIteratorFloat(); !it.end(); ++it) {
        if (not is_valid(it.state_index(), state.get_float(it.state_index()))) { return false; }
    }

    return true;
}

bool ModelInformation::is_valid(const StateBase& state, VariableIndex_type state_index) const {
    if (is_integer(state_index)) {
        return is_valid(state_index, state.get_int(state_index));
    } else {
        return is_valid(state_index, state.get_float(state_index));
    }
}

#endif

/* Unused functions. */

std::size_t ModelInformation::perfect_hash(const StateValues& state) const {
    PLAJA_ASSERT(get_floating_state_size() == 0)
    std::size_t hash_code = 0;
    for (auto it = stateIndexIteratorInt(true); !it.end(); ++it) {
        hash_code += hashFactorsInt[it.state_index()] * (state.get_int(it.state_index()) - it.lower_bound_int());
    }
    return hash_code;
}
