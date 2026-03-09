//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_STATE_VALUES_H
#define PLAJA_PA_STATE_VALUES_H

#include <memory>
#include "../../../utils/utils.h"
#include "pa_state_base.h"

class PaStateValuesBase: public PaStateBase {

    using LocationState = StateValues;

private:
    std::unique_ptr<LocationState> locationState;
    std::vector<PA::TruthValueType> predicateValues;

protected:

#ifndef NDEBUG
    PaStateValuesBase(const PaStateValuesBase& other);
#endif
    PaStateValuesBase(PaStateValuesBase&& other) noexcept;

public:
    PaStateValuesBase(PredicatesNumber_type num_predicates, std::size_t num_locs, const PredicatesExpression* predicates = nullptr);
    ~PaStateValuesBase() override;

#ifdef NDEBUG
    PaStateValuesBase(const PaStateValuesBase& other) = delete;
#endif
    DELETE_ASSIGNMENT(PaStateValuesBase)

    /** predicates ****************************************************************************************************/

    inline void set_predicate_value(PredicateIndex_type pred_index, PA::TruthValueType value) {
        PLAJA_ASSERT(pred_index < predicateValues.size())
        predicateValues[pred_index] = value;
    }

    [[nodiscard]] PA::TruthValueType predicate_value(PredicateIndex_type pred_index) const override {
        PLAJA_ASSERT(pred_index < get_size_predicates())
        return predicateValues[pred_index];
    }

    [[nodiscard]] bool is_set(PredicateIndex_type pred_index) const override { return not PA::is_unknown(PaStateValuesBase::predicate_value(pred_index)); }

    /** locations *****************************************************************************************************/

    void set_location(VariableIndex_type state_index, PLAJA::integer loc_value) const;

    void set_location_state(const LocationState& location_state) const;

    [[nodiscard]] bool has_location_state() const final;

    [[nodiscard]] inline const LocationState& get_location_state() const {
        PLAJA_ASSERT(has_location_state())
        return *locationState;
    }

    [[nodiscard]] inline LocationState& get_location_state() {
        PLAJA_ASSERT(has_location_state())
        return *locationState;
    }

    [[nodiscard]] PLAJA::integer location_value(VariableIndex_type loc_index) const final;

    /******************************************************************************************************************/

    [[nodiscard]] bool has_entailment_information() const override;

    [[nodiscard]] bool is_entailed(PredicateIndex_type pred_index) const override;

    [[nodiscard]] PA::EntailmentValueType entailment_value(PredicateIndex_type pred_index) const override;

    /******************************************************************************************************************/

    class PaStateValuesIterator final: public PaStateBase::PredicateStateIterator {
        friend PaStateValuesBase;

    private:

        explicit PaStateValuesIterator(const PaStateValuesBase& pa_state):
            PaStateBase::PredicateStateIterator(pa_state, 0) {}

    public:
        ~PaStateValuesIterator() override = default;
        DELETE_CONSTRUCTOR(PaStateValuesIterator)

    };

    [[nodiscard]] inline PaStateValuesIterator init_pa_state_values_it_preds() const { return PaStateValuesIterator(*this); }

};

/**********************************************************************************************************************/

/* Derive from base class to mark as final. */
class PaStateValues final: public PaStateValuesBase {

public:
    PaStateValues(PredicatesNumber_type num_predicates, std::size_t num_locs, const PredicatesExpression* predicates = nullptr);
    ~PaStateValues() override;
    DELETE_CONSTRUCTOR(PaStateValues)

};

#endif //PLAJA_PA_STATE_VALUES_H
