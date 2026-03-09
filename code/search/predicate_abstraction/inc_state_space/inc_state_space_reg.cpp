//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "inc_state_space_reg.h"
#include "../pa_states/abstract_state.h"
#include "../pa_states/predicate_state.h"

SEARCH_SPACE_PA::IncStateSpaceReg::IncStateSpaceReg(const PLAJA::Configuration& config, std::shared_ptr<StateRegistryPa> state_reg_pa):
    IncStateSpace(config, std::move(state_reg_pa)) {
}

SEARCH_SPACE_PA::IncStateSpaceReg::~IncStateSpaceReg() = default;

/**/

void SEARCH_SPACE_PA::IncStateSpaceReg::set_registry(PredicatesNumber_type num_preds) {

    if (not nodeRegistries.count(num_preds)) {

        nodeRegistries.emplace(num_preds, std::make_unique<SourceNodeRegistry>());

    }

}

/**********************************************************************************************************************/

void SEARCH_SPACE_PA::IncStateSpaceReg::increment() {
    const auto num_preds = stateRegistryPa->get_predicates_size();

    PLAJA_ASSERT(not nodeRegistries.count(num_preds))

    nodeRegistries.emplace(num_preds, std::make_unique<SourceNodeRegistry>());

    IncStateSpace::increment();
}

void SEARCH_SPACE_PA::IncStateSpaceReg::clear_coarser() {
    const auto num_preds = stateRegistryPa->get_predicates_size();

    for (auto it = nodeRegistries.begin(); it != nodeRegistries.end();) {

        if (it->first != num_preds) { it = nodeRegistries.erase(it); }

        else { ++it; }

    }

    IncStateSpace::clear_coarser();
}

/**********************************************************************************************************************/

SEARCH_SPACE_PA::SourceNode* SEARCH_SPACE_PA::IncStateSpaceReg::get_source_node(const AbstractState& src) {
    PLAJA_ASSERT(stateRegistryPa->has_pa_state(src.get_size_predicates(), src.get_id_value()))
    return get_node_reg(src.get_size_predicates()).get_source(src.get_id_value());
}

SEARCH_SPACE_PA::UpdateOpNode* SEARCH_SPACE_PA::IncStateSpaceReg::get_update_op(const AbstractState& src, UpdateOpID_type update_op_id) {
    PLAJA_ASSERT(stateRegistryPa->has_pa_state(src.get_size_predicates(), src.get_id_value()))
    return get_node_reg(src.get_size_predicates()).get_update_node(src.get_id_value(), update_op_id);
}

SEARCH_SPACE_PA::SourceNode& SEARCH_SPACE_PA::IncStateSpaceReg::add_source_node(const AbstractState& src) {
    PLAJA_ASSERT(stateRegistryPa->has_pa_state(src.get_size_predicates(), src.get_id_value()))
    return get_node_reg(src.get_size_predicates()).add_source(src.get_id_value());
}

SEARCH_SPACE_PA::UpdateOpNode& SEARCH_SPACE_PA::IncStateSpaceReg::add_update_op(const AbstractState& src, UpdateOpID_type update_op_id) {
    PLAJA_ASSERT(stateRegistryPa->has_pa_state(src.get_size_predicates(), src.get_id_value()))
    return get_node_reg(src.get_size_predicates()).add_update_node(src.get_id_value(), update_op_id);
}

/**********************************************************************************************************************/

void SEARCH_SPACE_PA::IncStateSpaceReg::set_excluded_internal(const AbstractState& src, UpdateOpID_type update_op_id, const AbstractState& excluded_successor) {
    PLAJA_ASSERT(stateRegistryPa->has_pa_state(src.get_size_predicates(), src.get_id_value()))
    PLAJA_ASSERT(stateRegistryPa->has_pa_state(excluded_successor.get_size_predicates(), excluded_successor.get_id_value()))
    get_node_reg(src.get_size_predicates()).set_excluded(src.get_id_value(), update_op_id, excluded_successor.get_id_value());
}

void SEARCH_SPACE_PA::IncStateSpaceReg::refine_for(const AbstractState& source) {

    const auto num_preds = source.get_size_predicates();
    auto& current_node_reg = get_node_reg(num_preds);
    auto& source_node = current_node_reg.add_source(source.get_id_value());

    for (auto it_ss = registryIterator(num_preds - 1); !it_ss.end(); ++it_ss) {
        PLAJA_ASSERT(it_ss.number_of_preds() < num_preds)
        auto& coarser_ss = it_ss.node_reg();
        const StateID_type coarser_id = stateRegistryPa->is_contained_in(source, it_ss.number_of_preds());
        if (coarser_id == STATE::none) { continue; }
        //
        auto* coarser_source_node = coarser_ss.get_source(coarser_id);
        if (not coarser_source_node) { continue; }
        for (auto& [update_op_id, update_op_node]: coarser_source_node->get_transitions_per_op()) {
            auto& transition_witnesses = update_op_node.get_transition_witnesses();
            for (auto it = transition_witnesses.begin(); it != transition_witnesses.end();) {
                const State concrete_source = stateRegistryPa->lookup_witness(it->get_source());
                if (source.is_same_predicate_state_sub(concrete_source, it_ss.number_of_preds())) {
                    PLAJA_ASSERT(source.is_abstraction(concrete_source))
                    source_node.add_transition_witness(update_op_id, it->get_source(), it->get_successor());
                    it = transition_witnesses.erase(it);
                } else { ++it; }
            }
        }
    }

    STMT_IF_DEBUG(if (has_ref()) { access_ref().refine_for(source); })
}

bool SEARCH_SPACE_PA::IncStateSpaceReg::is_excluded_label_internal(const AbstractState& source, ActionLabel_type action_label) const {
    for (auto& [num_preds, node_reg]: nodeRegistries) {
        if (num_preds >= source.get_size_predicates()) { break; } // considered all sub states
        const StateID_type coarser_id = stateRegistryPa->is_contained_in(source, num_preds);
        if (coarser_id == STATE::none) { continue; } // does not contain any information concerning source
        if (node_reg->is_excluded_label(coarser_id, action_label)) { return true; }
    }

    return false;
}

bool SEARCH_SPACE_PA::IncStateSpaceReg::is_excluded_op_internal(const AbstractState& source, ActionOpID_type action_op) const {
    for (auto& [num_preds, node_reg]: nodeRegistries) {
        if (num_preds >= source.get_size_predicates()) { break; } // considered all sub states
        const StateID_type coarser_id = stateRegistryPa->is_contained_in(source, num_preds);
        if (coarser_id == STATE::none) { continue; } // does not contain any information concerning source
        if (node_reg->is_excluded_op(coarser_id, action_op)) { return true; }
    }

    return false;
}

PredicatesNumber_type SEARCH_SPACE_PA::IncStateSpaceReg::load_entailments_internal(PredicateState& target, const AbstractState& source, UpdateOpID_type update_op, const PredicateRelations* predicate_relations) const {
    PLAJA_ASSERT(stateRegistryPa->has_pa_state(source.get_size_predicates(), source.get_id_value()))

    PredicatesNumber_type last_pred_size = 0;

    for (auto it = registryIterator(source.get_size_predicates()/* - 1*/); !it.end(); ++it) { // More robust to iterate all and use "has_entailment" flag.
        auto update_op_node = it.get_update_node(source, update_op);
        if (update_op_node and update_op_node->has_entailment()) {
            PLAJA_ASSERT(last_pred_size < update_op_node->retrieve_last_predicates_size())
            last_pred_size = update_op_node->retrieve_last_predicates_size();
            PLAJA_ASSERT(it.number_of_preds() == last_pred_size)
            update_op_node->load_entailments(target, predicate_relations);
        }
    }

    return last_pred_size;

}

inline std::unique_ptr<StateID_type> SEARCH_SPACE_PA::IncStateSpaceReg::transition_exists_internal(const AbstractState& source, UpdateOpID_type update_op_id, const AbstractState& successor) const {

    for (auto it = registryIterator(source.get_size_predicates() - 1); !it.end(); ++it) {
        const auto* update_op_node = it.get_update_node(source, update_op_id);
        if (not update_op_node) { continue; }
        // successor exclusion?
        const StateID_type successor_in_registry = stateRegistryPa->is_contained_in(successor, it.number_of_preds());
        if (successor_in_registry != STATE::none and update_op_node->is_excluded(successor_in_registry)) { return std::make_unique<StateID_type>(STATE::none); }
        //
        PLAJA_ASSERT(not std::any_of(update_op_node->get_transition_witnesses().cbegin(), update_op_node->get_transition_witnesses().cend(), [this, &source](const SEARCH_SPACE_PA::TransitionWitness& witness) { return source.is_abstraction(stateRegistryPa->lookup_witness(witness.get_source())); }))
# if 0
        for (const auto& transition_witness: update_op_node->_transition_witnesses()) {
                const auto concrete_source = stateRegistryPa->lookup_witness(transition_witness._source());
                if (not source.is_abstraction(concrete_source)) { continue; }
                const auto concrete_successor = stateRegistryPa->lookup_witness(transition_witness._successor());
                if (successor.is_abstraction(concrete_successor)) { return std::make_unique<StateID_type>(transition_witness._source()); }
            }
#endif
    }

    {
        const auto& current_node_reg = get_node_reg(source.get_size_predicates());
        const auto* update_op_node = current_node_reg.get_update_node(source.get_id_value(), update_op_id);
        if (not update_op_node) { return nullptr; }
        if (update_op_node->is_excluded(successor.get_id_value())) { return std::make_unique<StateID_type>(STATE::none); }
        for (const auto& transition_witness: update_op_node->get_transition_witnesses()) { // works only if abstract state is up-to-date
            PLAJA_ASSERT(source.is_abstraction(stateRegistryPa->lookup_witness(transition_witness.get_source())))
            const auto concrete_successor = stateRegistryPa->lookup_witness(transition_witness.get_successor());
            if (successor.is_abstraction(concrete_successor)) { return std::make_unique<StateID_type>(transition_witness.get_source()); }
        }
    }

    return nullptr;

}
