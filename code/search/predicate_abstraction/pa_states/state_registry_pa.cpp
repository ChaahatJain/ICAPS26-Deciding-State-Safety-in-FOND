
#include "state_registry_pa.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../factories/configuration.h"
#include "../../information/model_information.h"
#include "../match_tree/pa_match_tree.h"
#include "../match_tree/predicate_traverser.h"
#include "abstract_state.h"
#include "predicate_state.h"

StateRegistryPa::StateRegistryPa(const PLAJA::Configuration& config, const PredicatesExpression& predicates, const ModelInformation& model_info_concrete):
    predicates(&predicates)
    , matchTree(nullptr)
    , witnessRegistry(new StateRegistry(model_info_concrete.get_ranges_int(), model_info_concrete.get_lower_bounds_int(), model_info_concrete.get_upper_bounds_int(), model_info_concrete.get_floating_state_size())) {

    automataLocations.reserve(model_info_concrete.get_number_automata_instances());
    for (auto it = model_info_concrete.locationIndexIterator(); !it.end(); ++it) { automataLocations.push_back(it.range()); }

    if constexpr (PLAJA_GLOBAL::lazyPA) { matchTree = config.has_sharable(PLAJA::SharableKey::PA_MT) ? config.get_sharable<PAMatchTree>(PLAJA::SharableKey::PA_MT) : config.set_sharable<PAMatchTree>(PLAJA::SharableKey::PA_MT, std::make_shared<PAMatchTree>(config, predicates)); }

}

StateRegistryPa::~StateRegistryPa() = default;

/**********************************************************************************************************************/

PredicatesNumber_type StateRegistryPa::get_predicates_size() const { return predicates->get_number_predicates(); }

const Expression& StateRegistryPa::get_predicate(PredicateIndex_type pred_index) const {
    PLAJA_ASSERT(predicates->get_predicate(pred_index))
    return *predicates->get_predicate(pred_index);
}

PredicateTraverser StateRegistryPa::init_predicate_traverser() const {
#ifdef LAZY_PA
    return PredicateTraverser::init(*matchTree);
#else
    return PredicateTraverser::init(get_predicates_size());
#endif
}

FCT_IF_LAZY_PA(FCT_IF_DEBUG(bool StateRegistryPa::is_closed(const PaStateBase& pa_state, bool minimal) { return PredicateTraverser::is_closed(*matchTree, pa_state, minimal); }))

/**********************************************************************************************************************/

void StateRegistryPa::add_registry(PredicatesNumber_type num_preds) {

#if 0
    STMT_IF_DEBUG(for (PredicateIndex_type pred_index = 0; pred_index < std::min(predicates->get_number_predicates(), predicates_.get_number_predicates()); ++pred_index) { PLAJA_ASSERT(*predicates->get_predicate(pred_index) == *predicates_.get_predicate(pred_index)) })

    auto num_preds = predicates_.get_number_predicates();

    // update preds
    if (num_preds > predicates->get_number_predicates()) {
        predicates->reserve(num_preds);
        for (PredicateIndex_type pred_index = predicates->get_number_predicates(); pred_index < num_preds; ++pred_index) {
            predicates->add_predicate(predicates_.get_predicate(pred_index)->deepCopy_Exp());
        }
    }
#endif

    PLAJA_ASSERT(num_preds <= predicates->get_number_predicates())

    PLAJA_ASSERT(num_preds == predicates->get_number_predicates()) // expected behavior

    if (not stateRegistries.count(num_preds)) {

        const std::vector<PLAJA::integer> lower_bounds(automataLocations.size() + num_preds, 0);

        std::vector<PLAJA::integer> upper_bounds;
        std::vector<Range_type> ranges;
        upper_bounds.reserve(automataLocations.size() + num_preds);
        ranges.reserve(automataLocations.size() + num_preds);
        // set locations
        for (auto num_locs: automataLocations) {
            upper_bounds.push_back(PLAJA_UTILS::cast_numeric<PLAJA::integer>(num_locs) - 1);
            ranges.push_back(num_locs);
        }
        // set preds
        for (auto i = 0; i < num_preds; ++i) {
            upper_bounds.push_back(PLAJA_GLOBAL::lazyPA ? 2 : 1);
            ranges.push_back(PLAJA_GLOBAL::lazyPA ? 3 : 2);
        }

        stateRegistries.emplace(num_preds, std::make_unique<StateRegistry>(ranges, lower_bounds, upper_bounds));
    }

}

//

std::unique_ptr<AbstractState> StateRegistryPa::get_pa_state(PredicatesNumber_type pred_num, StateID_type state_id) { // NOLINT(*-easily-swappable-parameters)
    auto& state_reg = get_registry(pred_num);
    return AbstractState::make_ptr(state_reg.get_state_buffer(state_id), *this, state_reg.get_int_packer(), state_id, nullptr);
}

std::unique_ptr<AbstractState> StateRegistryPa::get_pa_state(StateID_type state_id) { return get_pa_state(predicates->get_number_predicates(), state_id); }

StateID_type StateRegistryPa::add_pa_state(const PaStateValues& pa_state_values) {
    const auto num_locs = automataLocations.size();
    const auto num_preds = pa_state_values.get_size_predicates();

    auto& state_reg = get_registry(num_preds);

    auto buffer = state_reg.push_state_construct();
    auto& packer = state_reg.get_int_packer();

    /* Set location values. */
    const auto& location_state = pa_state_values.get_location_state();
    PLAJA_ASSERT(location_state.size() == num_locs)
    VariableIndex_type state_index = 0;
    for (; state_index < num_locs; ++state_index) { packer.set(buffer.g_int(), state_index, location_state.get_int(state_index)); }

    /* Set predicate values. */
    for (PredicateIndex_type pred_index = 0; pred_index < num_preds; ++pred_index) {
        if constexpr (PLAJA_GLOBAL::lazyPA) {
            packer.set(buffer.g_int(), state_index++, pa_state_values.is_set(pred_index) ? pa_state_values.predicate_value(pred_index) : PA::unknown_value());
        } else {
            packer.set(buffer.g_int(), state_index++, pa_state_values.predicate_value(pred_index));
        }
    }

    return state_reg.insert_id_or_pop_state();
}

std::unique_ptr<AbstractState> StateRegistryPa::add_pa_state(const PredicateState& pa_state_values) {
    const auto& location_state = pa_state_values.get_location_state();

    PLAJA_ASSERT(location_state.size() == automataLocations.size())
    const auto num_locs = automataLocations.size();
    const auto num_preds = pa_state_values.get_size_predicates();

    auto& state_reg = get_registry(num_preds);

    auto buffer = state_reg.push_state_construct();
    auto& packer = state_reg.get_int_packer();

    // set location values
    VariableIndex_type state_index = 0;
    for (; state_index < num_locs; ++state_index) { packer.set(buffer.g_int(), state_index, location_state.get_int(state_index)); }

    // set predicate values
    for (PredicateIndex_type pred_index = 0; pred_index < num_preds; ++pred_index) {
        if constexpr (PLAJA_GLOBAL::lazyPA) {
            packer.set(buffer.g_int(), state_index++, pa_state_values.is_set(pred_index) ? pa_state_values.predicate_value(pred_index) : PA::unknown_value());
        } else {
            packer.set(buffer.g_int(), state_index++, pa_state_values.predicate_value(pred_index));
        }
    }

    auto state_id = state_reg.insert_id_or_pop_state();

    return AbstractState::make_ptr(state_reg.get_state_buffer(state_id), *this, packer, state_id, &pa_state_values);
}

std::unique_ptr<AbstractState> StateRegistryPa::add_pa_state(const StateBase& concrete_state) {

    auto& state_reg = get_registry(get_predicates_size());
    auto& packer = state_reg.get_int_packer();
    auto buffer = state_reg.push_state_construct();

    auto pa_state_view = AbstractState::make_ptr(buffer, *this, packer, STATE::none, nullptr);
    const auto num_locs = automataLocations.size();
    const auto pa_state_size = packer.get_state_size();

    /* Set location values. */
    VariableIndex_type state_index = 0;
    for (; state_index < num_locs; ++state_index) { packer.set(buffer.g_int(), state_index, concrete_state.get_int(state_index)); }

    if constexpr (PLAJA_GLOBAL::lazyPA) {

        /* Default to unknown. */
        for (; state_index < pa_state_size; ++state_index) {
            packer.set(buffer.g_int(), state_index, PA::unknown_value());
        }

        /* Set predicates in closure. */
        for (auto it = init_predicate_traverser(); !it.end(); it.next(*pa_state_view)) {
            const auto pred_index = it.predicate_index();
            packer.set(buffer.g_int(), num_locs + pred_index, get_predicate(pred_index).evaluate_integer(concrete_state));
        }

    } else {

        /* Simply iterate all predicates. */
        for (auto it = predicates->predicatesIterator(); !it.end(); ++it) {
            packer.set(buffer.g_int(), state_index++, it->evaluate_integer(concrete_state));
        }

    }

    /* Finalize abstract state. */
    pa_state_view->id = state_reg.insert_id_or_pop_state();
    pa_state_view->buffer = state_reg.get_state_buffer(pa_state_view->id); // Update buffer.

    return pa_state_view;
}

// TODO actually we can use the closure order instead
StateID_type StateRegistryPa::is_contained_in_rec(unsigned int index, StateID_type id, const StateRegistry& target_reg, IntPacker::Bin* target, const IntPacker& src_packer, const IntPacker::Bin* src) const { // NOLINT(misc-no-recursion)

    if (index < target_reg.get_int_state_size()) {

        const auto truth_val = src_packer.get(src, index);
        if (PA::is_unknown(truth_val)) {

            target_reg.intPacker.set(target, index, truth_val);
            return is_contained_in_rec(index + 1, id, target_reg, target, src_packer, src);

        } else {

            target_reg.intPacker.set(target, index, truth_val);
            const auto rlt = is_contained_in_rec(index + 1, id, target_reg, target, src_packer, src);

            if (rlt != STATE::none) {

#ifndef NDEBUG
                // sanity there should be only one containing state
                target_reg.intPacker.set(target, index, PA::unknown_value());
                PLAJA_ASSERT(is_contained_in_rec(index + 1, id, target_reg, target, src_packer, src) == STATE::none)
                target_reg.intPacker.set(target, index, truth_val); // not really necessary
#endif

                return rlt;
            }

            target_reg.intPacker.set(target, index, PA::unknown_value());
            return is_contained_in_rec(index + 1, id, target_reg, target, src_packer, src);

        }

    } else {

        auto result = target_reg.registeredStates.find(id);
        return result == target_reg.registeredStates.end() ? STATE::none : *result;

    }

}

StateID_type StateRegistryPa::is_contained_in(const AbstractState& pa_state, PredicatesNumber_type preds_num) const {
    auto& pa_reg = get_registry(preds_num);
    PLAJA_ASSERT(not pa_reg.floatDataPool)

    auto& int_data_pool = pa_reg.intDataPool;
    auto& target_packer = pa_reg.intPacker;
    auto& packer_pa_state = *pa_state.paPacker;
    auto* buffer_pa_state = pa_state.buffer.g_int();

    // int
    int_data_pool.push_back(target_packer.get_bin_constructor()); // to properly maintain state set
    const StateID_type id = int_data_pool.size() - 1; // NOLINT(cppcoreguidelines-narrowing-conversions)
    auto* target_buffer = int_data_pool[id];

    if constexpr (PLAJA_GLOBAL::lazyPA) {

        const auto num_locs = pa_state.get_size_locs();

        // locs
        unsigned int index = 0;
        for (; index < num_locs; ++index) { target_packer.set(target_buffer, index, packer_pa_state.get(buffer_pa_state, index)); }

        // preds
        auto rlt = is_contained_in_rec(index, id, pa_reg, target_buffer, packer_pa_state, buffer_pa_state);

        int_data_pool.pop_back();
        return rlt;

    } else {

        for (auto i = 0; i < pa_reg.get_int_state_size(); ++i) { target_packer.set(target_buffer, i, packer_pa_state.get(buffer_pa_state, i)); }

        auto result = pa_reg.registeredStates.find(id);
        int_data_pool.pop_back();
        if (result == pa_reg.registeredStates.end()) { return STATE::none; }
        else { return *result; }

    }
}

void StateRegistryPa::unregister_state_if_possible(StateID_type id) { stateRegistries[predicates->get_number_predicates()]->unregister_state_if_possible(id); }

void StateRegistryPa::drop_coarse_registries() {

    const auto num_preds = predicates->get_number_predicates();

    for (auto it = stateRegistries.begin(); it != stateRegistries.end();) {

        if (it->first != num_preds) { it = stateRegistries.erase(it); }

        else { ++it; }

    }

}

//

std::size_t StateRegistryPa::get_max_num_bytes_per_state() const {
    std::size_t max_num_bytes = 0;
    for (auto& preds_reg: stateRegistries) { max_num_bytes = std::max(max_num_bytes, preds_reg.second->get_num_bytes_per_state()); }
    return max_num_bytes;
}

std::size_t StateRegistryPa::size() const {
    std::size_t num_states = 0;
    for (auto& preds_reg: stateRegistries) { num_states += preds_reg.second->size(); }
    return num_states;
}