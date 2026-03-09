//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "update.h"
#include "../../parser/ast/expression/expression.h"
#include "../../parser/ast/assignment.h"
#include "../../parser/ast/destination.h"
#include "../../parser/ast/model.h"
#include "../../parser/visitor/extern/variables_of.h"
#include "../information/model_information.h"
#include "../fd_adaptions/state.h"

Update::Update(const Model& model_ref, std::vector<const Destination*> destinations_r):
    model(&model_ref)
    , numSequences(0)
    , destinations(std::move(destinations_r)) {
    PLAJA_ASSERT(not destinations.empty())
    for (const auto* dest: destinations) { numSequences = std::max(numSequences, dest->get_number_sequences()); }
    STMT_IF_RUNTIME_CHECKS(for (SequenceIndex_type seq_index = 0; seq_index <= get_num_sequences(); ++seq_index) {
        collect_updated_vars(seq_index);
        collect_updated_array_indexes(seq_index);
    })
}

Update::Update(const Model& model_ref, const Destination* destination):
    model(&model_ref)
    , numSequences(destination->get_number_sequences())
    , destinations() {
    // destinations.reserve(num_automata);
    destinations.push_back(destination);
}

Update::Update(const Update& update):
    model(update.model)
    , numSequences(update.numSequences)
    , destinations() {
    destinations.reserve(update.destinations.capacity()); // reserve capacity, equal to expected number of automata
    for (const auto* dest: update.destinations) { destinations.push_back(dest); } // copy destination references
}

Update::Update(Update&& update) noexcept :
    model(update.model)
    , numSequences(update.numSequences)
    , destinations(std::move(update.destinations)) {}

Update::~Update() = default;

void Update::add_destination(const Destination* destination) {
    // add destinations
    numSequences = std::max(numSequences, destination->get_number_sequences());
    destinations.push_back(destination);
}

/*
void Update::merge(const Update& update) {
    size_t l = update.destinations.size();
    destinations.reserve(destinations.size() + update.destinations.size());
    for (const auto* dest: update.destinations) {
        numSequences = std::max(numSequences, dest->get_number_sequences());
        destinations.push_back(dest);
    }
}
*/

void Update::evaluate_locations(StateValues* target) const {
    PLAJA_ASSERT(target)
    for (const auto* dest: destinations) { target->assign_int<true>(dest->get_location_index(), dest->get_location_value()); }
}

void Update::evaluate_locations(State* target) const {
    PLAJA_ASSERT(target)
    for (const auto* dest: destinations) { target->assign_int<true>(dest->get_location_index(), dest->get_location_value()); }
}

struct UpdateEvaluator {

    template<typename State_t1, typename State_t2>
    inline static void evaluate_seq(const Update* this_, const State_t1* source, State_t2* target, SequenceIndex_type seq_index) {
        for (const auto* dest: this_->destinations) {
            if (seq_index < dest->get_number_sequences()) { // otherwise all local sequences handled
                dest->evaluate(source, target, seq_index);
            }
        }
    }

    template<typename State_t>
    inline static void evaluate(const Update* this_, State_t* target) {
        PLAJA_ASSERT(target)

        // update locations:
        this_->evaluate_locations(target);

        // update state variables:
        for (SequenceIndex_type seq_index = 0; seq_index < this_->numSequences; ++seq_index) {
            const StateValues state_tmp(target->to_state_values());
            UpdateEvaluator::evaluate_seq(this_, &state_tmp, target, seq_index);
            target->refresh_written_to(); // refresh state internals for double assignment checking after each sequence
        }
    }

};

void Update::evaluate(State* target) const { UpdateEvaluator::evaluate(this, target); }

void Update::evaluate(StateValues* target) const { UpdateEvaluator::evaluate(this, target); }

std::unique_ptr<State> Update::evaluate(const State& source) const {
    std::unique_ptr<State> target = source.to_construct();
    //
    if (numSequences <= 1) { // special case
        evaluate_locations(target.get());
        UpdateEvaluator::evaluate_seq(this, &source, target.get(), 0);
    } else {
        UpdateEvaluator::evaluate(this, target.get());
    }
    //
    target->finalize();
    return target;
}

StateValues Update::evaluate(const StateValues& source) const {
    StateValues target(source);
    //
    if (numSequences <= 1) { // special case
        evaluate_locations(&target);
        UpdateEvaluator::evaluate_seq(this, &source, &target, 0);
    } else {
        UpdateEvaluator::evaluate(this, &target);
    }
    //
    return target;
}

bool Update::agrees(const StateBase& source, const StateBase& target) const {

    PLAJA_ASSERT(numSequences <= 1)

    for (const auto& dest: destinations) {

        if (target.get_int(dest->get_location_index()) != dest->get_location_value()) { return false; }

        if (dest->get_number_sequences() == 0) { continue; } // special case

        PLAJA_ASSERT(dest->get_number_sequences() == 1)

        for (const auto& assignment: dest->get_assignments_per_seq(0)) {
            if (not assignment->agrees(source, target)) { return false; }
        }
    }

    return true;
}

/**********************************************************************************************************************/

std::unordered_set<VariableIndex_type> Update::collect_updated_locs() const {
    std::unordered_set<VariableIndex_type> updated_locs;
    for (auto const& dest: destinations) { updated_locs.insert(dest->get_location_index()); }
    return updated_locs;
}

std::unordered_set<VariableID_type> Update::collect_updated_vars(SequenceIndex_type sequence_index) const {
    std::unordered_set<VariableID_type> updated_variables;
    for (auto it = assignmentIterator(sequence_index); !it.end(); ++it) {
        STMT_IF_RUNTIME_CHECKS(RUNTIME_ASSERT(it->get_array_index() or !updated_variables.count(it->get_updated_var_id()), "Duplicate non-array variable assignment in update:\n" + it->to_string()))
        updated_variables.insert(it->get_updated_var_id());
    }
    return updated_variables;
}

std::unordered_set<VariableIndex_type> Update::collect_updated_state_indexes(SequenceIndex_type sequence_index, bool include_locs, const StateBase* source) const {
    std::unordered_set<VariableIndex_type> updated_state_indexes;
    for (auto it = assignmentIterator(sequence_index); !it.end(); ++it) {
        PLAJA_ASSERT(not it->get_array_index() or it->get_array_index()->is_constant() or source)
        const VariableIndex_type state_index = it->get_variable_index(source);
        STMT_IF_RUNTIME_CHECKS(RUNTIME_ASSERT(not updated_state_indexes.count(state_index), "Duplicate state index assignment in update:\n" + it->to_string()))
        updated_state_indexes.insert(state_index);
    }
    if (include_locs) { updated_state_indexes.merge(collect_updated_locs()); }
    return updated_state_indexes;
}

std::unordered_set<VariableID_type> Update::collect_updating_vars(SequenceIndex_type sequence_index) const {
    std::unordered_set<VariableID_type> updating_variables;
    for (auto it = assignmentIterator(sequence_index); !it.end(); ++it) {

        const auto* array_index = it->get_array_index();
        if (array_index) { updating_variables.merge(VARIABLES_OF::vars_of(array_index, true)); } // do not forget array index !!!

        if (it->is_non_deterministic()) {
            updating_variables.merge(VARIABLES_OF::vars_of(it->get_lower_bound(), true));
            updating_variables.merge(VARIABLES_OF::vars_of(it->get_upper_bound(), true));
        } else {
            updating_variables.merge(VARIABLES_OF::vars_of(it->get_value(), true));
        }

    }
    return updating_variables;
}

std::unordered_set<VariableIndex_type> Update::collect_updating_state_indexes(SequenceIndex_type sequence_index) const {
    std::unordered_set<VariableIndex_type> updating_state_indexes;
    for (auto it = assignmentIterator(sequence_index); !it.end(); ++it) {

        const auto* array_index = it->get_array_index();
        if (array_index) { updating_state_indexes.merge(VARIABLES_OF::state_indexes_of(array_index, true, true)); } // do not forget array index !!!

        if (it->is_non_deterministic()) {
            updating_state_indexes.merge(VARIABLES_OF::state_indexes_of(it->get_lower_bound(), true, true));
            updating_state_indexes.merge(VARIABLES_OF::state_indexes_of(it->get_upper_bound(), true, true));
        } else {
            updating_state_indexes.merge(VARIABLES_OF::state_indexes_of(it->get_value(), true, true));
        }

    }

    return updating_state_indexes;
}

std::unordered_map<VariableID_type, std::vector<const Expression*>> Update::collect_updated_array_indexes(SequenceIndex_type sequence_index) const {
    std::unordered_map<VariableID_type, std::vector<const Expression*>> updated_array_indexes;
    for (auto it = assignmentIterator(sequence_index); !it.end(); ++it) {

        const VariableID_type updated_var = it->get_updated_var_id();
        const auto* index = it->get_array_index();

        if (index) {
            auto updated_indexes = updated_array_indexes.find(updated_var);

            if (updated_indexes == updated_array_indexes.end()) { updated_array_indexes.emplace(updated_var, std::vector<const Expression*> { index }); }
            else {
                STMT_IF_RUNTIME_CHECKS(for (const auto* other_index: updated_indexes->second) { RUNTIME_ASSERT(!index->equals(other_index), "Duplicate array index assignment in update:\n" + it->to_string()) })
                updated_indexes->second.push_back(index);
            }
        }

    }

    return updated_array_indexes;
}

std::unordered_set<VariableIndex_type> Update::infer_non_updated_locs(const std::unordered_set<VariableIndex_type>* updated_locs) const { // NOLINT(misc-no-recursion)
    const auto& model_info = model->get_model_information();

    if (updated_locs) {

        std::unordered_set<VariableIndex_type> non_updated_locs;

        for (auto it = model_info.locationIndexIterator(); !it.end(); ++it) {
            auto loc_index = it.location_index();
            if (not updated_locs->count(loc_index)) { non_updated_locs.insert(loc_index); }
        }

        return non_updated_locs;

    } else {

        const auto updated_locs_tmp = collect_updated_locs();
        return infer_non_updated_locs(&updated_locs_tmp);

    }
}

std::unordered_set<VariableID_type> Update::infer_non_updated_vars(SequenceIndex_type sequence_index, const std::unordered_set<VariableID_type>* updated_vars) const { // NOLINT(misc-no-recursion)
    if (updated_vars) {

        std::unordered_set<VariableID_type> non_updated_vars;

        for (auto var_it = model->variableIterator(); !var_it.end(); ++var_it) {
            const VariableID_type var_id = var_it.variable_id();
            if (not updated_vars->count(var_id)) { non_updated_vars.insert(var_id); }
        }

        return non_updated_vars;

    } else {

        const auto updated_vars_tmp = collect_updated_vars(sequence_index);
        return infer_non_updated_vars(sequence_index, &updated_vars_tmp);

    }
}

std::unordered_set<VariableIndex_type> Update::infer_non_updated_state_indexes(SequenceIndex_type sequence_index, bool include_locs, const std::unordered_set<VariableIndex_type>* updated_state_indexes) const { // NOLINT(misc-no-recursion)
    if (updated_state_indexes) {

        PLAJA_ASSERT(not include_locs)

        std::unordered_set<VariableIndex_type> non_updated_state_indexes;
        for (auto it = model->get_model_information().stateIndexIterator(include_locs); !it.end(); ++it) {
            if (not updated_state_indexes->count(it.state_index())) { non_updated_state_indexes.insert(it.state_index()); }
        }

        return non_updated_state_indexes;

    } else {

        const auto updated_vars_tmp = collect_updated_state_indexes(sequence_index, include_locs);
        return infer_non_updated_state_indexes(sequence_index, include_locs, &updated_vars_tmp);

    }
}

bool Update::is_non_deterministic(SequenceIndex_type sequence_index) const {
    for (auto it = assignmentIterator(sequence_index); !it.end(); ++it) { if (it->is_non_deterministic()) { return true; } }
    return false;
}

bool Update::is_non_deterministic() const {
    PLAJA_ASSERT_EXPECT(get_num_sequences() <= 1)

    for (const auto& destination: destinations) {
        for (auto it = destination->assignmentIterator(); !it.end(); ++it) {
            if (it->is_non_deterministic()) { return true; }
        }
    }

    return false;
}

/**Iterator************************************************************************************************************/

Update::LocationIterator::LocationIterator(const std::vector<const Destination*>& destinations):
    it(destinations.cbegin())
    , itEnd(destinations.cend()) {}

VariableIndex_type Update::LocationIterator::loc() const { return (*it)->get_location_index(); }

PLAJA::integer Update::LocationIterator::dest() const { return (*it)->get_location_value(); }

inline const Destination& Update::AssignmentIterator::get_destination(std::size_t index) const {
    PLAJA_ASSERT(destinations)
    PLAJA_ASSERT(index < destinations->size())
    PLAJA_ASSERT((*destinations)[index])
    return *(*destinations)[index];
}

bool Update::AssignmentIterator::end() const { return refDest >= destinations->size(); }

Update::AssignmentIterator::AssignmentIterator(const std::vector<const Destination*>& destinations_ref, unsigned int seq_index):
    sequenceIndex(seq_index)
    , destinations(&destinations_ref)
    , refDest(0) {

    while (!end()) {
        if (sequenceIndex < get_destination(refDest).get_number_sequences() && get_destination(refDest).get_number_assignments_per_seq(sequenceIndex) > 0) {
            const auto& assignments_per_seq = get_destination(refDest).get_assignments_per_seq(sequenceIndex);
            it = assignments_per_seq.cbegin();
            itEnd = assignments_per_seq.cend();
            break;
        }
        ++refDest;
    }

}

Update::AssignmentIterator::~AssignmentIterator() = default;

void Update::AssignmentIterator::operator++() {
    if (++it == itEnd) {
        ++refDest;

        /* Next assignment list may be empty, then have to skip ... */
        while (not end()) {
            if (sequenceIndex < get_destination(refDest).get_number_sequences() and get_destination(refDest).get_number_assignments_per_seq(sequenceIndex) > 0) {
                const auto& assignments_per_seq = get_destination(refDest).get_assignments_per_seq(sequenceIndex);
                it = assignments_per_seq.cbegin();
                itEnd = assignments_per_seq.cend();
                break;
            }
            ++refDest;
        }

    }
}

const Assignment* Update::AssignmentIterator::operator()() const { return *it; }

const Assignment* Update::AssignmentIterator::operator->() const { return operator()(); }

const Assignment& Update::AssignmentIterator::operator*() const { return *operator()(); }

// short-cuts:

const LValueExpression* Update::AssignmentIterator::ref() const { return operator*().get_ref(); }

VariableID_type Update::AssignmentIterator::var() const { return operator*().get_updated_var_id(); }

const Expression* Update::AssignmentIterator::array_index() const { return operator*().get_array_index(); }

VariableIndex_type Update::AssignmentIterator::variable_index(const StateBase* state) const { return operator*().get_variable_index(state); }

const Expression* Update::AssignmentIterator::value() const { return operator*().get_value(); }

void Update::AssignmentIterator::evaluate(const StateBase* current, StateBase& target) const {
    PLAJA_ASSERT_EXPECT(not operator*().is_non_deterministic())
    operator*().evaluate(current, &target);
}

bool Update::AssignmentIterator::is_non_deterministic() const { return operator*().is_non_deterministic(); }

const Expression* Update::AssignmentIterator::lower_bound() const { return operator*().get_lower_bound(); }

const Expression* Update::AssignmentIterator::upper_bound() const { return operator*().get_upper_bound(); }

void Update::AssignmentIterator::clip(const StateBase* current, StateBase& target) const { operator*().clip(current, target); }

