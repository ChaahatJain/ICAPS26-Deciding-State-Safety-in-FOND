#ifndef PLAJA_STATE_REGISTRY_PA_H
#define PLAJA_STATE_REGISTRY_PA_H

#include "../../../parser/ast/expression/forward_expression.h"
#include "../../information/forward_information.h"
#include "../../factories/forward_factories.h"
#include "../../fd_adaptions/state_registry.h"
#include "../../fd_adaptions/state.h"
#include "../match_tree/forward_match_tree_pa.h"
#include "../using_predicate_abstraction.h"
#include "forward_pa_states.h"
#include <map>
#ifdef USE_VERITAS
#include <interval.hpp>
#endif

class StateRegistryPa {
    friend class AbstractState;

    const PredicatesExpression* predicates;
    std::shared_ptr<PAMatchTree> matchTree;
    std::vector<Range_type> automataLocations;

    std::unordered_map<PredicatesNumber_type, std::unique_ptr<StateRegistry>> stateRegistries;

    std::unique_ptr<StateRegistry> witnessRegistry;

    // Boxes map
#ifdef USE_VERITAS
    mutable std::map<StateID_type, std::vector<veritas::Interval>> boxRegistry;
#endif

    [[nodiscard]] StateID_type is_contained_in_rec(unsigned int index, StateID_type id, const StateRegistry& target_reg, IntPacker::Bin* target, const IntPacker& src_packer, const IntPacker::Bin* source) const;

public:
    explicit StateRegistryPa(const PLAJA::Configuration& config, const PredicatesExpression& predicates, const ModelInformation& model_info_concrete);
    ~StateRegistryPa();
    DELETE_CONSTRUCTOR(StateRegistryPa)

    [[nodiscard]] const PredicatesExpression& get_predicates() const {
        PLAJA_ASSERT(predicates)
        return *predicates;
    }

    [[nodiscard]] PredicatesNumber_type get_predicates_size() const;

    [[nodiscard]] const Expression& get_predicate(PredicateIndex_type pred_index) const;

    [[nodiscard]] PredicateTraverser init_predicate_traverser() const;

    FCT_IF_LAZY_PA(FCT_IF_DEBUG([[nodiscard]] bool is_closed(const PaStateBase& pa_state, bool minimal);))

    std::size_t num_locations() { return automataLocations.size(); }

    /* */

    [[nodiscard]] bool has_registry(PredicatesNumber_type pred_num) const { return stateRegistries.count(pred_num); }

    [[nodiscard]] const StateRegistry& get_registry(PredicatesNumber_type pred_num) const {
        PLAJA_ASSERT(has_registry(pred_num))
        return *stateRegistries.at(pred_num);
    }

    [[nodiscard]] StateRegistry& get_registry(PredicatesNumber_type pred_num) {
        PLAJA_ASSERT(has_registry(pred_num))
        return *stateRegistries.at(pred_num);
    }

    void add_registry(PredicatesNumber_type num_preds);

    /* Abstract states. */

    FCT_IF_DEBUG([[nodiscard]] bool has_pa_state(PredicatesNumber_type pred_num, StateID_type state_id) const { return get_registry(pred_num).has_state(state_id); })

    std::unique_ptr<AbstractState> get_pa_state(PredicatesNumber_type pred_num, StateID_type state_id);

    /** Assumes maximal number of predicates. */
    std::unique_ptr<AbstractState> get_pa_state(StateID_type state_id);

    StateID_type add_pa_state(const PaStateValues& pa_state_values);

    std::unique_ptr<AbstractState> add_pa_state(const PredicateState& pa_state_values);

    std::unique_ptr<AbstractState> add_pa_state(const StateBase& concrete_state);

    [[nodiscard]] StateID_type is_contained_in(const AbstractState& pa_state, PredicatesNumber_type preds_num) const;

    void unregister_state_if_possible(StateID_type id);

    void drop_coarse_registries();

    /* Witnesses. */
#ifdef USE_VERITAS
    inline StateID_type add_witness(const StateValues& witness, std::vector<veritas::Interval> b) {
        auto id = witnessRegistry->add_state_id(witness);
        if (!b.empty()) { boxRegistry[id] = b; }
        return id;
    }

    [[nodiscard]] inline std::vector<veritas::Interval> get_box_witness(StateID_type state_id) const { return boxRegistry[state_id]; }

#endif

    inline StateID_type add_witness(const StateValues& witness) {
        auto id = witnessRegistry->add_state_id(witness);
        return id;
    }

    [[nodiscard]] inline std::unique_ptr<State> get_witness(StateID_type state_id) const { return witnessRegistry->get_state(state_id); };

    [[nodiscard]] inline State lookup_witness(StateID_type state_id) const { return witnessRegistry->lookup_state(state_id); };


    /* */

    [[nodiscard]] std::size_t get_max_num_bytes_per_state() const;
    [[nodiscard]] std::size_t size() const;

};

#endif // PLAJA_STATE_REGISTRY_PA_H
