//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//


#ifndef PLAJA_PA_STATE_BASE_H
#define PLAJA_PA_STATE_BASE_H

#include <memory>
#include <utility>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/forward_ast.h"
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../../states/forward_states.h"
#include "../using_predicate_abstraction.h"
#include "forward_pa_states.h"

class PaStateBase {

private:
    const PredicatesExpression* predicates;

    const std::size_t numPredicates; // NOLINT(*-avoid-const-or-ref-data-members)
    const std::size_t numLocations; // NOLINT(*-avoid-const-or-ref-data-members)

protected:
    PaStateBase(const PredicatesExpression* predicates, std::size_t num_preds, std::size_t num_locs);
public:
    virtual ~PaStateBase() = 0;
    DELETE_CONSTRUCTOR(PaStateBase)

    /******************************************************************************************************************/

    [[nodiscard]] const PredicatesExpression* get_predicates() const { return predicates; }

    [[nodiscard]] const Expression& get_predicate(PredicateIndex_type pred_index) const;

    [[nodiscard]] inline std::size_t get_size_locs() const { return numLocations; }

    [[nodiscard]] inline std::size_t get_size_predicates() const { return numPredicates; }

    [[nodiscard]] inline std::size_t size() const { return numLocations + numPredicates; }

    /******************************************************************************************************************/

    [[nodiscard]] virtual PA::TruthValueType predicate_value(PredicateIndex_type pred_index) const = 0;

    [[nodiscard]] virtual bool is_set(PredicateIndex_type pred_index) const = 0;

    /******************************************************************************************************************/

    [[nodiscard]] virtual bool has_location_state() const;

    [[nodiscard]] virtual PLAJA::integer location_value(VariableIndex_type loc_index) const = 0;

    /******************************************************************************************************************/

    [[nodiscard]] virtual bool has_entailment_information() const = 0;

    [[nodiscard]] virtual bool is_entailed(PredicateIndex_type pred_index) const = 0;

    [[nodiscard]] virtual PA::EntailmentValueType entailment_value(PredicateIndex_type pred_index) const = 0;

    /******************************************************************************************************************/

    [[nodiscard]] bool is_same_location_state(const PaStateBase& other) const;

    [[nodiscard]] bool is_same_predicate_state(const PaStateBase& other) const;

    [[nodiscard]] bool operator==(const PaStateBase& other) const;

    [[nodiscard]] inline bool operator!=(const PaStateBase& other) const { return not operator==(other); }

    [[nodiscard]] bool agrees_on_entailment(const PaStateBase& other) const;

    /******************************************************************************************************************/

    [[nodiscard]] bool is_same_location_state(const StateBase& concrete_state) const;

    [[nodiscard]] bool agree(const StateBase& concrete_state, PredicateIndex_type pred_index) const;

    [[nodiscard]] bool is_same_predicate_state_sub(const StateBase& concrete_state, PredicateIndex_type start_index) const;

    [[nodiscard]] inline bool is_same_predicate_state(const StateBase& concrete_state) const { return is_same_predicate_state_sub(concrete_state, 0); }

    [[nodiscard]] inline bool is_abstraction(const StateBase& concrete_state) const { return is_same_location_state(concrete_state) and is_same_predicate_state(concrete_state); }

    /******************************************************************************************************************/

    /** For each variable compute a lower and upper bound (syntactically) entailed by this state. */
    [[nodiscard]] std::pair<StateValues, StateValues> compute_bounds(const class ModelInformation& model_info) const;

    /******************************************************************************************************************/

    [[nodiscard]] std::unique_ptr<PaStateValues> to_pa_state_values(bool do_locs) const;

    void to_location_state(StateValues& location_state) const;

    FCT_IF_DEBUG([[nodiscard]] std::size_t get_number_of_set_predicates() const;)

    FCT_IF_DEBUG([[nodiscard]] StateValues to_state_values() const;)

    [[nodiscard]] std::unique_ptr<Expression> to_array_value(const Model* model = nullptr, bool include_entailment = false) const;

    void dump() const;

    /******************************************************************************************************************/

    class LocationStateIterator {
        friend PaStateBase;

    protected:
        const PaStateBase* paState;
        VariableIndex_type locationIndex;

        explicit LocationStateIterator(const PaStateBase& pa_state):
            paState(&pa_state)
            , locationIndex(0) {}

    public:
        virtual ~LocationStateIterator() = default;
        DELETE_CONSTRUCTOR(LocationStateIterator)

        inline void operator++() { ++locationIndex; }

        [[nodiscard]] inline bool end() const { return locationIndex >= paState->numLocations; }

        [[nodiscard]] inline VariableIndex_type state_index() const { return locationIndex; }

        [[nodiscard]] inline VariableIndex_type location_index() const { return locationIndex; }

        [[nodiscard]] inline PLAJA::integer location_value() const { return paState->location_value(locationIndex); }
    };

    [[nodiscard]] inline LocationStateIterator init_loc_state_it() const { return LocationStateIterator(*this); }

    /******************************************************************************************************************/

    class PredicateStateIterator {
        friend PaStateBase;

    protected:
        const PaStateBase* paState;
        PredicateIndex_type predIndex;

        explicit PredicateStateIterator(const PaStateBase& pa_state, PredicateIndex_type start_index):
            paState(&pa_state)
            , predIndex(start_index) {}

    public:
        virtual ~PredicateStateIterator() = default;
        DELETE_CONSTRUCTOR(PredicateStateIterator)

        inline void operator++() { ++predIndex; }

        [[nodiscard]] inline bool end() const { return predIndex >= paState->get_size_predicates(); }

        // [[nodiscard]] inline VariableIndex_type state_index() const { return stateIndex; } // that is actually the information we want to hide
        [[nodiscard]] inline PredicateIndex_type predicate_index() const { return predIndex; }

        [[nodiscard]] const Expression& predicate() const { return paState->get_predicate(predicate_index()); }

        [[nodiscard]] inline PA::TruthValueType predicate_value() const { return paState->predicate_value(predicate_index()); }

        [[nodiscard]] inline bool is_set() const { return paState->is_set(predicate_index()); }

        [[nodiscard]] inline bool has_entailment_information() const { return paState->has_entailment_information(); }

        [[nodiscard]] inline PA::EntailmentValueType entailment_value() const { return paState->entailment_value(predicate_index()); }

        [[nodiscard]] inline bool is_entailed() const { return paState->is_entailed(predicate_index()); }

    };

    [[nodiscard]] inline PredicateStateIterator init_pred_state_it() const { return PredicateStateIterator(*this, 0); }

};

#endif //PLAJA_PA_STATE_BASE_H
