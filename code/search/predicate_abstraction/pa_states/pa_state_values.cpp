//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pa_state_values.h"
#include "../../states/state_values.h"

PaStateValuesBase::PaStateValuesBase(PredicatesNumber_type num_predicates, std::size_t num_locs, const PredicatesExpression* predicates):
    PaStateBase(predicates, num_predicates, num_locs)
    , locationState(num_locs > 0 ? new LocationState(num_locs) : nullptr)
#ifdef LAZY_PA
    , predicateValues(num_predicates, PA::unknown_value())
#else
    , predicateValues(num_predicates)
#endif
{
}

#ifndef NDEBUG

PaStateValuesBase::PaStateValuesBase(const PaStateValuesBase& other):
    PaStateBase(other.get_predicates(), other.get_size_predicates(), other.get_size_locs())
    , locationState(other.locationState ? other.locationState->to_ptr() : nullptr)
    , predicateValues(other.predicateValues) {
}

#endif

PaStateValuesBase::PaStateValuesBase(PaStateValuesBase&& other) noexcept :
    PaStateBase(other.get_predicates(), other.get_size_predicates(), other.get_size_locs())
    , locationState(std::move(other.locationState))
    , predicateValues(std::move(other.predicateValues)) {
}

PaStateValuesBase::~PaStateValuesBase() = default;

/**********************************************************************************************************************/

void PaStateValuesBase::set_location(VariableIndex_type state_index, PLAJA::integer loc_value) const {
    PLAJA_ASSERT(has_location_state())
    PLAJA_ASSERT(state_index < locationState->size());
    PLAJA_ASSERT(locationState->size() == get_size_locs())
    locationState->assign_int<true>(state_index, loc_value);
}

void PaStateValuesBase::set_location_state(const PaStateValuesBase::LocationState& location_state) const {
    PLAJA_ASSERT(location_state.size() == get_size_locs())
    for (VariableIndex_type state_index = 0; state_index < get_size_locs(); ++state_index) { set_location(state_index, location_state.get_int(state_index)); }
}

bool PaStateValuesBase::has_location_state() const { return locationState.get(); }

PLAJA::integer PaStateValuesBase::location_value(VariableIndex_type loc_index) const {
    PLAJA_ASSERT(has_location_state())
    return locationState->get_int(loc_index);
}

/**********************************************************************************************************************/

bool PaStateValuesBase::has_entailment_information() const { return false; }

bool PaStateValuesBase::is_entailed(PredicateIndex_type) const { PLAJA_ABORT }

PA::EntailmentValueType PaStateValuesBase::entailment_value(PredicateIndex_type) const { PLAJA_ABORT }

/**********************************************************************************************************************/

PaStateValues::PaStateValues(PredicatesNumber_type num_predicates, std::size_t num_locs, const PredicatesExpression* predicates):
    PaStateValuesBase(num_predicates, num_locs, predicates) {
}

PaStateValues::~PaStateValues() = default;