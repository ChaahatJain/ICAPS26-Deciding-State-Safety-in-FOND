//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//


#include "pa_state_base.h"
#include "../../../parser/ast/expression/non_standard/comment_expression.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../../parser/ast/expression/array_value_expression.h"
#include "../../../parser/ast/expression/constant_value_expression.h"
#include "../../../parser/visitor/retrieve_variable_bound.h"
#include "../../../globals.h"
#include "../../information/model_information.h"
#include "../../states/state_values.h"
#include "predicate_state.h"

/******************************************************************************************************************/

PaStateBase::PaStateBase(const PredicatesExpression* predicates, std::size_t num_preds, std::size_t num_locs): // NOLINT(bugprone-easily-swappable-parameters)
    predicates(predicates)
    , numPredicates(num_preds)
    , numLocations(num_locs) {
    PLAJA_ASSERT(not predicates or num_preds <= predicates->get_number_predicates())
}

PaStateBase::~PaStateBase() = default;

/**********************************************************************************************************************/

const Expression& PaStateBase::get_predicate(PredicateIndex_type pred_index) const {
    PLAJA_ASSERT(pred_index < get_size_predicates())
    PLAJA_ASSERT(predicates)
    PLAJA_ASSERT(predicates->get_predicate(pred_index))
    return *predicates->get_predicate(pred_index);
}

bool PaStateBase::has_location_state() const { return true; }

/**********************************************************************************************************************/

bool PaStateBase::is_same_location_state(const PaStateBase& other) const {
    if (get_size_locs() != other.get_size_locs()) { return false; }

    PLAJA_ASSERT(has_location_state() == other.has_location_state())

    for (auto it = init_loc_state_it(); !it.end(); ++it) {
        if (it.location_value() != other.location_value(it.location_index())) { return false; }
    }

    return true;
}

bool PaStateBase::is_same_predicate_state(const PaStateBase& other) const {
    if (get_size_predicates() != other.get_size_predicates()) { return false; }

    for (auto it = init_pred_state_it(); !it.end(); ++it) {
        if (it.predicate_value() != other.predicate_value(it.predicate_index())) { return false; }
    }

    return true;
}

bool PaStateBase::agrees_on_entailment(const PaStateBase& other) const {
    if (get_size_predicates() != other.get_size_predicates()) { return false; }

    if (has_entailment_information()) {

        if (not other.has_entailment_information()) { return false; }

        for (auto it = init_pred_state_it(); !it.end(); ++it) {
            if (it.entailment_value() != other.entailment_value(it.predicate_index())) { return false; }
        }

        return true;

    } else {

        return not other.has_entailment_information();

    }

}

bool PaStateBase::operator==(const PaStateBase& other) const { return is_same_location_state(other) and is_same_predicate_state(other); }

/**********************************************************************************************************************/

bool PaStateBase::is_same_location_state(const StateBase& concrete_state) const {

    for (auto loc_it = init_loc_state_it(); !loc_it.end(); ++loc_it) {
        if (concrete_state.get_int(loc_it.state_index()) != loc_it.location_value()) {
            return false;
        }
    }

    return true;

}

bool PaStateBase::agree(const StateBase& concrete_state, PredicateIndex_type pred_index) const {

    PLAJA_ASSERT(is_set(pred_index))

    return predicate_value(pred_index) == get_predicate(pred_index).evaluate_integer(concrete_state);

}

bool PaStateBase::is_same_predicate_state_sub(const StateBase& concrete_state, PredicateIndex_type start_index) const { // NOLINT(misc-no-recursion)

    for (auto pred_it = PredicateStateIterator(*this, start_index); !pred_it.end(); ++pred_it) {
        PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or pred_it.is_set()) // expected

        if (not PLAJA_GLOBAL::lazyPA or pred_it.is_set()) {
            if (pred_it.predicate_value() != pred_it.predicate().evaluate_integer(concrete_state)) {
                STMT_IF_DEBUG(
                    if (PLAJA_GLOBAL::do_additional_outputs()) {
                       // std::cout << "Concrete dump "; concrete_state.dump(true);
                        PLAJA_LOG_DEBUG(PLAJA_UTILS::string_f("Predicate %i does not evaluate to %i.", pred_it.predicate_index(), pred_it.predicate_value()))
                        pred_it.predicate().dump();
                    }
                )
                return false;
            }
        }

    }

    PLAJA_ASSERT(start_index == 0 or is_same_predicate_state_sub(concrete_state, 0))

    return true;
}

/**********************************************************************************************************************/

std::pair<StateValues, StateValues> PaStateBase::compute_bounds(const class ModelInformation& model_info) const {

    StateValues lower_bounds(model_info.get_int_state_size(), model_info.get_floating_state_size());
    StateValues upper_bounds(model_info.get_int_state_size(), model_info.get_floating_state_size());

    for (auto it = init_loc_state_it(); !it.end(); ++it) {
        lower_bounds.set_int(it.location_index(), it.location_value());
        upper_bounds.set_int(it.location_index(), it.location_value());
    }

    // default bounds

    for (auto it = model_info.stateIndexIteratorInt(false); !it.end(); ++it) {
        lower_bounds.set_int(it.state_index(), it.lower_bound_int());
        upper_bounds.set_int(it.state_index(), it.upper_bound_int());
    }

    for (auto it = model_info.stateIndexIteratorFloat(); !it.end(); ++it) {
        lower_bounds.set_float(it.state_index(), it.lower_bound_float());
        upper_bounds.set_float(it.state_index(), it.upper_bound_float());
    }

    // tighten
    for (auto pred_it = PredicateStateIterator(*this, 0); !pred_it.end(); ++pred_it) {

        PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or pred_it.is_set()) // expected

        if (not PLAJA_GLOBAL::lazyPA or pred_it.is_set()) {

            auto bound = RetrieveVariableBound::retrieve_bound(pred_it.predicate());
            PLAJA_ASSERT_EXPECT(bound) // So far we only use predicates that are bounds.

            if (not bound) { continue; }

            if (not pred_it.predicate_value()) { bound->negate(); }

            const auto state_index = bound->get_state_index();
            const auto& value = bound->get_value();

            if (model_info.is_integer(state_index)) {

                switch (bound->get_op()) {

                    case BinaryOpExpression::EQ: {
                        PLAJA_ASSERT(value.is_integer())
                        const auto value_int = value.evaluate_integer_const();
                        PLAJA_ASSERT(lower_bounds.get_int(state_index) <= value_int)
                        PLAJA_ASSERT(value_int <= upper_bounds.get_int(state_index))
                        lower_bounds.set_int(state_index, value_int);
                        upper_bounds.set_int(state_index, value_int);
                        break;
                    }

                    case BinaryOpExpression::NE: {
                        PLAJA_ABORT // Not expected, not supported.
                    }

                    case BinaryOpExpression::LT: {
                        PLAJA::integer value_int; // NOLINT(*-init-variables)
                        if (value.is_integer()) {
                            value_int = value.evaluate_integer_const() - 1;
                        } else if (value.evaluates_integer_const()) {
                            value_int = value.round()->evaluate_integer_const() - 1;
                        } else {
                            value_int = value.floor()->evaluate_integer_const();
                        }
                        if (value_int < upper_bounds.get_int(state_index)) { upper_bounds.set_int(state_index, value_int); }
                        break;
                    }

                    case BinaryOpExpression::LE: {
                        PLAJA::integer value_int; // NOLINT(*-init-variables)
                        if (value.is_integer()) {
                            value_int = value.evaluate_integer_const();
                        } else if (value.evaluates_integer_const()) {
                            value_int = value.round()->evaluate_integer_const();
                        } else {
                            value_int = value.floor()->evaluate_integer_const();
                        }
                        if (value_int < upper_bounds.get_int(state_index)) { upper_bounds.set_int(state_index, value_int); }
                        break;
                    }

                    case BinaryOpExpression::GT: {
                        PLAJA::integer value_int; // NOLINT(*-init-variables)
                        if (value.is_integer()) {
                            value_int = value.evaluate_integer_const() + 1;
                        } else if (value.evaluates_integer_const()) {
                            value_int = value.round()->evaluate_integer_const() + 1;
                        } else {
                            value_int = value.ceil()->evaluate_integer_const();
                        }
                        if (value_int > lower_bounds.get_int(state_index)) { lower_bounds.set_int(state_index, value_int); }
                        break;
                    }

                    case BinaryOpExpression::GE: {
                        PLAJA::integer value_int; // NOLINT(*-init-variables)
                        if (value.is_integer()) {
                            value_int = value.evaluate_integer_const();
                        } else if (value.evaluates_integer_const()) {
                            value_int = value.round()->evaluate_integer_const();
                        } else {
                            value_int = value.ceil()->evaluate_integer_const();
                        }
                        if (value_int > lower_bounds.get_int(state_index)) { lower_bounds.set_int(state_index, value_int); }
                        break;
                    }

                    default: { PLAJA_ABORT }
                }

            } else {

                switch (bound->get_op()) {

                    case BinaryOpExpression::EQ: {
                        const auto value_float = value.evaluate_floating_const();
                        PLAJA_ASSERT(lower_bounds.get_float(state_index) <= value_float)
                        PLAJA_ASSERT(value_float <= upper_bounds.get_float(state_index))
                        lower_bounds.set_float(state_index, value_float);
                        upper_bounds.set_float(state_index, value_float);
                        break;
                    }

                    case BinaryOpExpression::NE: {
                        PLAJA_ABORT // Not expected, not supported.
                    }

                    case BinaryOpExpression::LT:
                    case BinaryOpExpression::LE: {
                        const auto value_float = value.evaluate_floating_const();
                        if (value_float < upper_bounds.get_float(state_index)) { upper_bounds.set_float(state_index, value_float); }
                        break;
                    }

                    case BinaryOpExpression::GT:
                    case BinaryOpExpression::GE: {
                        const auto value_float = value.evaluate_floating_const();
                        if (value_float > lower_bounds.get_float(state_index)) { lower_bounds.set_float(state_index, value_float); }
                        break;
                    }

                    default: { PLAJA_ABORT }

                }
            }

        }

    }

    return { std::move(lower_bounds), std::move(upper_bounds) };

}

/**********************************************************************************************************************/

std::unique_ptr<PaStateValues> PaStateBase::to_pa_state_values(bool do_locs) const {
    auto pa_state_values = std::make_unique<PaStateValues>(numPredicates, do_locs ? numLocations : 0, predicates);

    /* Locations. */
    if (do_locs) {
        for (auto loc_it = init_loc_state_it(); !loc_it.end(); ++loc_it) {
            pa_state_values->set_location(loc_it.state_index(), loc_it.location_value());
        }
    }

    /* Predicates. */
    for (auto pred_it = init_pred_state_it(); !pred_it.end(); ++pred_it) {
        if (not PLAJA_GLOBAL::lazyPA or pred_it.is_set()) {
            pa_state_values->set_predicate_value(pred_it.predicate_index(), pred_it.predicate_value());
        }
    }

    return pa_state_values;
}

void PaStateBase::to_location_state(StateValues& location_state) const {
    PLAJA_ASSERT(location_state.size() == numLocations)

    location_state.refresh_written_to(); // to suppress double assignments errors

    for (auto loc_it = init_loc_state_it(); !loc_it.end(); ++loc_it) {
        location_state.assign_int<false>(loc_it.location_index(), loc_it.location_value());
    }

}

#ifndef NDEBUG

std::size_t PaStateBase::get_number_of_set_predicates() const {

    std::size_t counter = 0;

    for (auto it = init_pred_state_it(); !it.end(); ++it) { counter += it.is_set(); }

    return counter;

}

StateValues PaStateBase::to_state_values() const {
    StateValues state_values(numLocations + numPredicates);

    /* Locations. */
    for (auto it = init_loc_state_it(); !it.end(); ++it) {
        state_values.assign_int(it.location_index(), it.location_value());
    }

    /* Predicates. */
    VariableIndex_type state_index = numLocations;
    for (auto it = init_pred_state_it(); !it.end(); ++it) {
        state_values.assign_int(state_index++, it.predicate_value());
    }

    return state_values;
}

#endif

std::unique_ptr<Expression> PaStateBase::to_array_value(const Model* model, bool add_entailment) const {

    std::unique_ptr<ArrayValueExpression> values;

    /* Locations. */
    if (has_location_state()) {
        StateValues location_state(numLocations);
        to_location_state(location_state);
        values = PLAJA_UTILS::cast_unique<ArrayValueExpression>(model ? location_state.locs_to_array_value(*model) : location_state.to_array_value(nullptr));
    } else {
        values = std::make_unique<ArrayValueExpression>();
    }

    /* Predicates. */
    values->reserve(numLocations + numPredicates);
    for (auto it = init_pred_state_it(); !it.end(); ++it) {

        static const auto value_to_str = [](const auto& it) {
            if (it.is_set()) {
                // it.predicate().dump(true);
                if (it.predicate_value() == PA::True) { return "T"; }
                else if (it.predicate_value() == PA::False) { return "F"; }
                else { PLAJA_ABORT }

            } else {

                PLAJA_ASSERT(not PLAJA_GLOBAL::lazyPA or PA::is_unknown(it.predicate_value())) // Value should be properly initialized for lazy PA.
                return "U";

            }
        };

        if (add_entailment and has_entailment_information() and it.is_entailed()) {
            values->add_element(std::make_unique<CommentExpression>(value_to_str(it) + PLAJA_UTILS::string_f(" (entailed: %i)", it.entailment_value())));
        } else {
            values->add_element(std::make_unique<CommentExpression>(value_to_str(it)));
        }
    }

    return values;
}

void PaStateBase::dump() const { to_array_value(PLAJA_GLOBAL::currentModel, true)->dump(true); }

/**********************************************************************************************************************/

