//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MODEL_INFORMATION_H
#define PLAJA_MODEL_INFORMATION_H

#include <memory>
#include <vector>
#include <unordered_map>
#include "../../parser/ast_forward_declarations.h"
#include "../../parser/using_parser.h"
#include "../../utils/structs_utils/action_op_id_structure.h"
#include "../../utils/structs_utils/action_op_structure.h"
#include "../../utils/structs_utils/update_op_structure.h"
#include "../../utils/default_constructors.h"
#include "../../assertions.h"
#include "../states/forward_states.h"
#include "synchronisation_information.h"

/**
 * (Constant-dependent) information about the model, moreover the variables, e.g. lower/upper bounds, domain sizes.
 * Assumes the model to be completely *instantiated*, i.e., each automaton instance is represented by its own object instance on the implementation level.
 * Implements a (limited) perfect hash function for states of bounded integers and Booleans, i.e., a unique value for each state (for state spaces of restricted size);
 * cf. [1] Stefan Edelkamp. Planning with pattern databases. In A. Cesta and D. Borrajo, editors, Recent Advances in AI Planning. 6th European Conference on Planning (ECP-01), Lecture Notes in Artificial Intelligence, pages 13–24, Toledo, Spain, September 2001. Springer-Verlag.
 * Uses the same hash to compute unique action operator IDs and tight operator indexes (maximal number of operators is thus *theoretically* restricted by this implementation).
 */
class ModelInformation final {
public:

    /* Invariant. */
    static inline SyncID_type sync_index_to_id(SyncIndex_type index) { return index + 1; }

    static inline SyncIndex_type sync_id_to_index(SyncID_type sync_id) { return sync_id - 1; }

    static inline ActionID_type action_label_to_id(ActionLabel_type label) { return label; }

    static inline ActionLabel_type action_id_to_label(ActionID_type id) { return id; }

    // Hack to retrieve model info w/o including model header:
    static const ModelInformation& get_model_info(const Model& model);
private:
    // Variable information (sorted by model index, including automata location variables):
    std::vector<PLAJA::integer> lowerBoundsInt; // the minimal elements in the domain of the variables; *0* for Booleans
    std::vector<PLAJA::integer> upperBoundsInt; // the maximal elements in the domain of the variables; *1* for Booleans
    std::vector<Range_type> rangesInt; // ranges, i.e, the domain size for bounded integers and booleans
    std::vector<unsigned int> hashFactorsInt; // a factor used to compute a hash code for each state (unique for discrete states)
    //
    std::vector<PLAJA::floating> lowerBoundsFloat; // the minimal elements in the domain of the variables; *0* for Booleans
    std::vector<PLAJA::floating> upperBoundsFloat; // the maximal elements in the domain of the variables; *1* for Booleans
    //
    std::unique_ptr<StateValues> initialValues;
    //
    std::vector<VariableID_type> varIndexToId; // TODO might implement this more space-efficiently, e.g. list of array ranges

    std::vector<VariableIndex_type> transientVariables; // transient variables by model index
    std::unordered_map<VariableID_type, std::size_t> arraySizes; // array size by variable ID

    // Synchronisation information
    const SynchronisationInformation syncInformation;

    // Operator Index information (to yield tight indexes):
    // Observation: Per sync (or non-sync) & automaton only edges with a certain action label are used, i.e., a certain "label group".
    // Approach: EdgeIDs are assigned tight w.r.t. ActionID (e.g. EdgeID 0 to 2 for silent edges, 3 to 5 for edges with label "label1" etc.).
    // Hence, caching the "EdgeID-offsets" for the "label group" of the syncs (or non-sync) per automaton + the resulting operator factors (for syncs),
    // we can use [1] to obtain a syntactically dense indexing.
    std::vector<int> nonSyncOffsets; // per automaton the index offset for non sync ops, starts with -1 due to nullEdge
    std::vector<std::vector<std::pair<unsigned int, unsigned int>>> perSyncOpOffsetsAndFactors; // per sync per automaton the EdgeID offset and hash factors (latter is invalid 0 for non-participating) (system/composition index rather than ID, i.a., 0 is *not* non-sync)

    // Action operator ID information:
    // (deprecated, only at construction time) std::vector<unsigned int> edgeRanges; // for each automaton instance the number of edges + 1 (null-edge, i.e., non-participating)
    // (deprecated) std::vector<std::size_t> opFactors; // for each automaton instance a factor used to compute a unique ID for each possible op.
    unsigned int syncOpFactor; // i.e., 1 + max(operator index) over non-sync and all syncs
    // Update op ID information
    unsigned int updateOpFactor; // i.e., 1 + max(ActionOpID)

    std::vector<ConstantIdType> constants; // Non-lined constants.

    void add_boolean_information();
    void add_bounded_int_information(PLAJA::integer lower_bound, PLAJA::integer upper_bound);
    void add_bounded_float_information(PLAJA::floating lower_bound, PLAJA::floating upper_bound);
    void compute_variable_information(Model& model);
    void compute_constant_information(Model& model);
    void compute_edge_and_sync_information(Model& model);
    // called by compute_edge_and_sync_information:
    void compute_action_op_index_information(Model& model, const std::vector<std::vector<std::list<Edge*>>>& edges_per_automaton_per_action);
public:
    explicit ModelInformation(Model& model);
    ~ModelInformation();
    DELETE_CONSTRUCTOR(ModelInformation)

    [[nodiscard]] inline const std::vector<Range_type>& get_ranges_int() const { return rangesInt; }

    [[nodiscard]] inline const std::vector<PLAJA::integer>& get_lower_bounds_int() const { return lowerBoundsInt; }

    [[nodiscard]] inline const std::vector<PLAJA::integer>& get_upper_bounds_int() const { return upperBoundsInt; }

    [[nodiscard]] inline const StateValues& get_initial_values() const { return *initialValues; }

    [[nodiscard]] inline const std::vector<VariableIndex_type>& get_transient_variables() const { return transientVariables; }

    /* To access individual bounds. */

    [[nodiscard]] inline PLAJA::integer get_lower_bound_int(VariableIndex_type state_index) const {
        PLAJA_ASSERT(state_index < lowerBoundsInt.size())
        return lowerBoundsInt[state_index];
    }

    [[nodiscard]] inline PLAJA::integer get_upper_bound_int(VariableIndex_type state_index) const {
        PLAJA_ASSERT(state_index < upperBoundsInt.size())
        return upperBoundsInt[state_index];
    }

    [[nodiscard]] inline Range_type get_range(VariableIndex_type state_index) const {
        PLAJA_ASSERT(state_index < rangesInt.size())
        return rangesInt[state_index];
    }

    /* */

    [[nodiscard]] inline PLAJA::floating get_lower_bound_float(VariableIndex_type state_index) const {
        PLAJA_ASSERT(state_index >= lowerBoundsInt.size())
        return lowerBoundsFloat[state_index - lowerBoundsInt.size()];
    }

    [[nodiscard]] inline PLAJA::floating get_upper_bound_float(VariableIndex_type state_index) const {
        PLAJA_ASSERT(state_index >= upperBoundsInt.size())
        return upperBoundsFloat[state_index - upperBoundsInt.size()];
    }

    /* */

    [[nodiscard]] inline PLAJA::floating get_lower_bound(VariableIndex_type state_index) const { if (state_index < lowerBoundsInt.size()) { return get_lower_bound_int(state_index); } else { return get_lower_bound_float(state_index); } }

    [[nodiscard]] inline PLAJA::floating get_upper_bound(VariableIndex_type state_index) const { if (state_index < upperBoundsInt.size()) { return get_upper_bound_int(state_index); } else { return get_upper_bound_float(state_index); } }

    [[nodiscard]] PLAJA::integer get_initial_value_int(VariableIndex_type state_index) const;
    [[nodiscard]] PLAJA::floating get_initial_value_float(VariableIndex_type state_index) const;
    [[nodiscard]] PLAJA::floating get_initial_value(VariableIndex_type state_index) const;

    [[nodiscard]] inline VariableID_type get_id(VariableIndex_type state_index) const {
        PLAJA_ASSERT(not is_location(state_index))
        PLAJA_ASSERT(state_index < varIndexToId.size())
        return varIndexToId[state_index];
    }

    /* Arrays. */

    [[nodiscard]] inline std::size_t get_array_size(VariableID_type var_id) const {
        PLAJA_ASSERT(is_array(var_id))
        return arraySizes.at(var_id);
    }

    [[nodiscard]] inline bool is_array(VariableID_type var_id) const { return arraySizes.count(var_id); }

    [[nodiscard]] inline std::size_t get_variable_size(VariableID_type var_id) const { return is_array(var_id) ? arraySizes.at(var_id) : 1; }

    [[nodiscard]] inline bool belongs_to_array(VariableIndex_type state_index) const { return is_array(get_id(state_index)); }

    [[nodiscard]] inline const std::vector<ConstantIdType>& get_constants() const { return constants; }

    /******************************************************************************************************************/

    /* Synchronisation information. */

    [[nodiscard]] inline const SynchronisationInformation& get_synchronisation_information() const { return syncInformation; }

    [[nodiscard]] inline ActionID_type sync_index_to_action(SyncIndex_type sync_index) const { return syncInformation.synchronisationResults[sync_index]; }

    [[nodiscard]] inline ActionID_type sync_id_to_action(SyncID_type sync_id) const {
        if (sync_id == SYNCHRONISATION::nonSyncID) { return ACTION::silentLabel; }
        else { return sync_index_to_action(sync_id_to_index(sync_id)); }
    }

    [[nodiscard]] inline std::size_t get_number_automata_instances() const { return syncInformation.num_automata_instances; } // number of automata/location indexes
    [[nodiscard]] inline std::size_t get_automata_offset() const { return get_number_automata_instances(); } // to the outside: automata offset

    /******************************************************************************************************************/

    /* State indexes. */

    /**
     * @return number of state indexes (including locations, summing up array sizes).
     */
    [[nodiscard]] inline std::size_t get_state_size() const { return lowerBoundsInt.size() + lowerBoundsFloat.size(); }

    [[nodiscard]] inline std::size_t get_int_state_size() const { return lowerBoundsInt.size(); }

    [[nodiscard]] inline std::size_t get_floating_state_size() const { return lowerBoundsFloat.size(); }

    [[nodiscard]] inline bool is_location(VariableIndex_type state_index) const { return state_index < get_number_automata_instances(); }

    [[nodiscard]] inline bool is_integer(VariableIndex_type state_index) const { return state_index < lowerBoundsInt.size(); }

    [[nodiscard]] inline bool is_floating(VariableIndex_type state_index) const { return not is_integer(state_index); }

    class StateIndexIterator final {
        friend ModelInformation;

    private:
        const ModelInformation& modelInfo;
        VariableIndex_type currentIndex;
        VariableIndex_type endIndex;

        explicit StateIndexIterator(const ModelInformation& model_info, VariableIndex_type start_index, VariableIndex_type end_index):
            modelInfo(model_info)
            , currentIndex(start_index)
            , endIndex(end_index) {}

    public:
        ~StateIndexIterator() = default;
        DELETE_CONSTRUCTOR(StateIndexIterator)

        inline void operator++() { ++currentIndex; }

        [[nodiscard]] inline bool end() const { return currentIndex >= endIndex; }

        [[nodiscard]] inline VariableIndex_type state_index() const { return currentIndex; }

        [[nodiscard]] inline bool is_location() const { return modelInfo.is_location(currentIndex); }

        [[nodiscard]] inline VariableIndex_type location_index() const {
            PLAJA_ASSERT(is_location())
            return state_index();
        }

        /**/

        [[nodiscard]] inline bool is_integer() const { return modelInfo.is_integer(state_index()); }

        [[nodiscard]] inline PLAJA::integer lower_bound_int() const { return modelInfo.get_lower_bound_int(currentIndex); }

        [[nodiscard]] inline PLAJA::integer upper_bound_int() const { return modelInfo.get_upper_bound_int(currentIndex); }

        [[nodiscard]] inline PLAJA::integer initial_value_int() const { return modelInfo.get_initial_value_int(currentIndex); }

        [[nodiscard]] inline Range_type range() const { return modelInfo.get_range(currentIndex); }

        /**/

        [[nodiscard]] inline bool is_floating() const { return modelInfo.is_floating(state_index()); }

        [[nodiscard]] inline PLAJA::floating lower_bound_float() const { return modelInfo.get_lower_bound_float(currentIndex); }

        [[nodiscard]] inline PLAJA::floating upper_bound_float() const { return modelInfo.get_upper_bound_float(currentIndex); }

        [[nodiscard]] inline PLAJA::floating initial_value_float() const { return modelInfo.get_initial_value_float(currentIndex); }

        /**/

        // [[nodiscard]] inline VAR::floating lower_bound() const { return modelInfo.get_lower_bound(currentIndex); }
        // [[nodiscard]] inline VAR::floating upper_bound() const { return modelInfo.get_upper_bound(currentIndex); }
        //
        // [[nodiscard]] inline PLAJA::floating initial_value() const { return modelInfo.get_initial_value(currentIndex); }

        [[nodiscard]] inline VariableID_type id() const {
            PLAJA_ASSERT(not is_location())
            return modelInfo.get_id(currentIndex);
        }

        [[nodiscard]] inline bool belongs_to_array() const { return modelInfo.belongs_to_array(currentIndex); }
    };

    [[nodiscard]] inline StateIndexIterator stateIndexIterator(bool include_locations = true) const { return StateIndexIterator(*this, include_locations ? 0 : get_number_automata_instances(), get_state_size()); }

    [[nodiscard]] inline StateIndexIterator locationIndexIterator() const { return StateIndexIterator(*this, 0, get_number_automata_instances()); }

    [[nodiscard]] inline StateIndexIterator stateIndexIteratorInt(bool include_locations = true) const { return StateIndexIterator(*this, include_locations ? 0 : get_number_automata_instances(), get_int_state_size()); }

    [[nodiscard]] inline StateIndexIterator stateIndexIteratorFloat() const { return StateIndexIterator(*this, get_int_state_size(), get_state_size()); }

    [[nodiscard]] inline std::size_t compute_state_space_size_discrete() const { return hashFactorsInt[hashFactorsInt.size() - 1] * rangesInt[rangesInt.size() - 1]; }

    [[nodiscard]] inline std::size_t compute_location_space_size() const { return hashFactorsInt[get_number_automata_instances() - 1] * rangesInt[get_number_automata_instances() - 1]; }

    /**
    * Computes a hash code [1] that is unique for the given state in the state space of the underlying modules file.
    * Limitation: Unique if the state size is less or equal 2^31-1.
    * @param state
    * @return
    */
    [[nodiscard]] [[deprecated("Hashing per StateRegistry")]] std::size_t perfect_hash(const StateValues& state) const;

    [[nodiscard]] bool is_valid(VariableIndex_type state_index, PLAJA::integer value) const;
    [[nodiscard]] bool is_valid(VariableIndex_type state_index, PLAJA::floating value) const;
    /**
    * Checks whether a state is in the state space of the underlying model, i.e., if values are within in the bounds.
    * @param state
    * @return true if the state is in the state space
    * *Note* that a state not in the state space does not imply an modeling error but may also occur as a result of abstraction.
    */
    [[nodiscard]] bool is_valid(const StateValues& state) const;
    FCT_IF_DEBUG([[nodiscard]] bool is_valid(const StateBase& state) const;)
    FCT_IF_DEBUG([[nodiscard]] bool is_valid(const StateBase& state, VariableIndex_type state_index) const;) // Should check *before* set, so hence only for debug.

    /******************************************************************************************************************/

    /**
     * Compute a unique index for a ordering of non-sync action operators.
     * @param automaton_index
     * @param edge_id
     * @return
     */
    [[nodiscard]] inline ActionOpIndex_type compute_action_op_index_non_sync(AutomatonIndex_type automaton_index, EdgeID_type edge_id) const {
        PLAJA_ASSERT(automaton_index < nonSyncOffsets.size())
        return nonSyncOffsets[automaton_index] + edge_id;
    }

    /**
    * Compute the addend to the action operator index of the given edge participating in an sync with the given *index* in the model's system.
    * @param sync_index synchronisation index, i.e., the index w.r.t. to the composition structure of the model (i.p. *not* the sync ID).
    * @param automaton_index
    * @param edge_id
    * @return
    */
    [[nodiscard]] inline ActionOpIndex_type compute_action_op_index_sync_additive(SyncIndex_type sync_index, AutomatonIndex_type automaton_index, EdgeID_type edge_id) const {
        PLAJA_ASSERT(sync_index < perSyncOpOffsetsAndFactors.size() && automaton_index < perSyncOpOffsetsAndFactors[sync_index].size())
        const auto& offset_and_factor = perSyncOpOffsetsAndFactors[sync_index][automaton_index];
        PLAJA_ASSERT(offset_and_factor.second > 0) // assert that specified automaton actually participates in synchronisation
        return offset_and_factor.second * (edge_id - offset_and_factor.first);
    }

    /**
     * Compute the action operator index given an action operator structure.
     * @param action_op_structure, i.e., the sync ID and the participating automaton index/edge ID pairs.
     * @return
     */
    [[nodiscard]] ActionOpIndex_type compute_action_op_index(const ActionOpStructure& action_op_structure) const {
        // non sync
        if (action_op_structure.syncID == SYNCHRONISATION::nonSyncID) {
            PLAJA_ASSERT(action_op_structure.participatingEdges.size() == 1)
            const auto& auto_edge = action_op_structure.participatingEdges.front();
            return compute_action_op_index_non_sync(auto_edge.first, auto_edge.second);
        } else { // sync op:
            ActionOpIndex_type action_op_index = 0;
            SyncIndex_type sync_index = sync_id_to_index(action_op_structure.syncID);
            for (const auto& automaton_V_edge: action_op_structure.participatingEdges) {
                action_op_index += compute_action_op_index_sync_additive(sync_index, automaton_V_edge.first, automaton_V_edge.second);
            }
            return action_op_index;
        }
    }

    /**
    * Compute the action op structure for a non-sync action operator given a corresponding operator index.
    * @param action_op_index
    * @return
    */
    [[nodiscard]] ActionOpStructure compute_action_op_structure_non_sync(ActionOpIndex_type action_op_index) const {
        ActionOpStructure action_op_structure;
        action_op_structure.syncID = SYNCHRONISATION::nonSyncID;
        for (AutomatonIndex_type automaton_index = 0; automaton_index < syncInformation.num_automata_instances; ++automaton_index) {
            if (action_op_index < nonSyncOffsets[automaton_index]) {
                PLAJA_ASSERT(automaton_index - 1 > 0 && action_op_index - nonSyncOffsets[automaton_index] != EDGE::nullEdge)
                action_op_structure.participatingEdges.emplace_front(automaton_index - 1, action_op_index - nonSyncOffsets[automaton_index]);
            }
        }
        PLAJA_ABORT
    }

    /**
     * Compute the action op structure given a sync index and a corresponding operator index.
     * @param sync_index
     * @param action_op_index
     * @return
     */
    [[nodiscard]] ActionOpStructure compute_action_op_structure_sync(SyncIndex_type sync_index, ActionOpIndex_type action_op_index) const {
        ActionOpStructure action_op_structure;
        action_op_structure.syncID = sync_index_to_id(sync_index);
        auto& participating_edges = action_op_structure.participatingEdges;
        PLAJA_ASSERT(syncInformation.num_automata_instances <= std::numeric_limits<signed long>::max())
        for (signed long automata_index = syncInformation.num_automata_instances - 1; automata_index >= 0; --automata_index) { // NOLINT(cppcoreguidelines-narrowing-conversions)
            const auto& offset_and_factor = perSyncOpOffsetsAndFactors[sync_index][automata_index];
            if (offset_and_factor.second == 0) { continue; } // non-participating automaton
            EdgeID_type edge_id = (action_op_index / offset_and_factor.second) + offset_and_factor.first;
            participating_edges.emplace_front(automata_index, edge_id);
            action_op_index = action_op_index % offset_and_factor.second;
        }
        PLAJA_ASSERT(action_op_index == 0)
        return action_op_structure;
    }

    /**
     * Compute the action op structure given sync ID and a corresponding action operator index.
     * @param sync_id
     * @param action_op_index
     * @return
     */
    [[nodiscard]] ActionOpStructure compute_action_op_structure(SyncID_type sync_id, ActionOpIndex_type action_op_index) const {
        if (sync_id == SYNCHRONISATION::nonSyncID) {
            return compute_action_op_structure_non_sync(action_op_index);
        } else {
            return compute_action_op_structure_sync(sync_id_to_index(sync_id), action_op_index);
        }
    }

    /******************************************************************************************************************/

    /**
     * Compute the addend to the action operator ID for the given sync (non-sync is possible).
     * @param sync_id
     * @return
     */
    [[nodiscard]] inline ActionOpID_type compute_action_op_id_additive(SyncID_type sync_id) const {
        return syncOpFactor * sync_id;
    }

    /**
     * Compute the action operator ID given a sync ID and corresponding operator index.
     * @param sync_id
     * @param action_op_index
     * @return
     */
    [[nodiscard]] inline ActionOpID_type compute_action_op_id(SyncID_type sync_id, ActionOpIndex_type action_op_index) const {
        return action_op_index + compute_action_op_id_additive(sync_id);
    }

    /**
     * Compute the action operator ID given the operator ID structure.
     * @param action_op_id_structure
     * @return
     */
    [[nodiscard]] inline ActionOpID_type compute_action_op_id(const ActionOpIDStructure& action_op_id_structure) const {
        return compute_action_op_id(action_op_id_structure.syncID, action_op_id_structure.actionOpIndex);
    }

    /**
    * Compute the action operator ID given an action op structure.
    * @param action_op_structure, i.e., the sync ID and the participating automaton index/edge ID pairs.
    * @return
    */
    [[nodiscard]] inline ActionOpID_type compute_action_op_id(const ActionOpStructure& action_op_structure) const {
        return compute_action_op_id(action_op_structure.syncID, compute_action_op_index(action_op_structure));
    }

    /**
     * Compute the action operator ID for the given *silent* edge.
     * @param automaton_index
     * @param edge_id
     * @return
     */
    [[nodiscard]] inline ActionOpID_type compute_action_op_id_non_sync(AutomatonIndex_type automaton_index, EdgeID_type edge_id) const {
        return compute_action_op_id_additive(SYNCHRONISATION::nonSyncID) + compute_action_op_index_non_sync(automaton_index, edge_id);
    }

    /**
     * Compute the addend to the action operator ID of the given edge for the given sync (non-sync is *not* possible).
     * @param sync_index
     * @param automaton_index
     * @param edge_id
     * @return
     */
    [[nodiscard]] inline ActionOpID_type compute_action_op_id_sync_additive(SyncIndex_type sync_index, AutomatonIndex_type automaton_index, EdgeID_type edge_id) const {
        return compute_action_op_index_sync_additive(sync_index, automaton_index, edge_id);
    }

    /**
    * Compute the action operator ID structure, i.e., the sync id and operator index, given an action operator ID.
    * @param action_op_id
    * @return
    */
    [[nodiscard]] inline ActionOpIDStructure compute_action_op_id_structure(ActionOpID_type action_op_id) const {
        PLAJA_ASSERT(action_op_id != ACTION::noneOp)
        return ActionOpIDStructure(action_op_id / syncOpFactor, action_op_id % syncOpFactor); // NOLINT(modernize-return-braced-init-list)
    }

    /**
     * Compute the action operator structure given an action operator ID.
     * @param action_op_id
     * @return
     */
    [[nodiscard]] inline ActionOpStructure compute_action_op_structure(ActionOpID_type action_op_id) const {
        PLAJA_ASSERT(action_op_id != ACTION::noneOp)
        return compute_action_op_structure(action_op_id / syncOpFactor, action_op_id % syncOpFactor);
    }

    /******************************************************************************************************************/

    [[nodiscard]] inline UpdateOpID_type compute_update_op_id(ActionOpID_type action_op_id, UpdateIndex_type update_index) const {
        PLAJA_ASSERT(action_op_id != ACTION::noneOp)
        PLAJA_ASSERT(update_index != ACTION::noneUpdate)
        return updateOpFactor * update_index + action_op_id;
    }

    [[nodiscard]] inline UpdateOpID_type compute_update_op_id(const UpdateOpStructure& update_op) const {
        return compute_update_op_id(update_op.actionOpID, update_op.updateIndex);
    }

    [[nodiscard]] inline UpdateOpStructure compute_update_op_structure(UpdateOpID_type update_op_id) const {
        PLAJA_ASSERT(update_op_id != ACTION::noneUpdateOp)
        return UpdateOpStructure(update_op_id % updateOpFactor, update_op_id / updateOpFactor); // NOLINT(modernize-return-braced-init-list)
    }

};

#endif //PLAJA_MODEL_INFORMATION_H
