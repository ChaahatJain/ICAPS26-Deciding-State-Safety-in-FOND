//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_CONSTRUCTION_H
#define PLAJA_ACTION_OP_CONSTRUCTION_H


namespace SUCCESSOR_GENERATION {

    template<typename ActionOp_t>
    inline void resize_action_op_vector(std::vector<std::unique_ptr<ActionOp_t>>& storage_vector, std::size_t vec_size) {
        if (storage_vector.size() < vec_size) { storage_vector.resize(vec_size); }
    }

    template<typename Edge_t, typename ActionOp_t>
    struct ActionOpConstruction {
    private:
        const Model& model;
        const ModelInformation& modelInfo;
        const SynchronisationInformation& syncInfo;
        std::vector<std::pair<std::unique_ptr<ActionOp_t>, ActionOpIndex_type>> constructorList; // cache during construction; *reset* per sync directive
        std::vector<std::vector<std::list<const Edge_t*>>> enabledEdges; // enabled edges per automaton and per non-silent action (during exploration we only use the edge IDs)

        [[nodiscard]] inline const std::list<const Edge_t*>& _enabledEdges(AutomatonIndex_type automaton_index, ActionID_type action_id) const {
            PLAJA_ASSERT(automaton_index < enabledEdges.size())
            PLAJA_ASSERT(action_id < enabledEdges[automaton_index].size())
            return enabledEdges[automaton_index][action_id];
        }

        inline void produce_with(const Edge_t* edge, ActionOpID_type additive_action_op_id, ActionOpIndex_type additive_action_op_index) {
            // produce with all existing action ops
            for (auto& action_op_index: constructorList) {
                action_op_index.first->product_with(edge, additive_action_op_id);
                action_op_index.second += additive_action_op_index;
            }
        }

    public:
        explicit ActionOpConstruction(const Model& model_):
                model(model_),
                modelInfo(ModelInformation::get_model_info(model_)), syncInfo(modelInfo.get_synchronisation_information()) {
            enabledEdges.resize(syncInfo.num_automata_instances);
            for (auto& enabled_per_automaton: enabledEdges) { enabled_per_automaton.resize(syncInfo.num_actions); }
        }

        ~ActionOpConstruction() = default;

        // edges:
        inline void add_enabled_edge(AutomatonIndex_type automaton_index, ActionID_type action_id, const Edge_t* edge) { enabledEdges[automaton_index][action_id].push_back(edge); }

        // action operators:
        inline void reserve(std::size_t c) { constructorList.reserve(c); }  // reserve per sync directive
        inline void clear() { constructorList.clear(); } // reset per sync directive

        // template for predicate abstraction
        template<typename Solver_t>
        inline void finalize(Solver_t& solver) { for (const auto& op_index_pair: constructorList) { op_index_pair.first->finalize(solver); } }

        inline void move_to(std::vector<std::unique_ptr<ActionOp_t>>& sync_ops) {
            for (auto& [op, index]: constructorList) {
                // add to permanent syn ops
                resize_action_op_vector(sync_ops, index + 1);
                sync_ops[index] = std::move(op);
            }
        }

        /**
         * Subroutine to compute action operator.
         * Adaption of process_local_op in successor_generator.cpp,
         * which is an adaption of calculateTransitions method from https://github.com/prismmodelchecker/prism/blob/8394df8e1a0058cec02f47b0437b185e3ae667d7/prism/src/simulator/Updater.java (PRISM, March 20, 2019).
         */
        void process_local_sync_edges(SyncIndex_type sync_index, const AutomatonSyncAction& condition) {
            const auto& enabled_edges = _enabledEdges(condition.automatonIndex, condition.actionID);
            // case where there is only 1 edge for this automaton
            if (enabled_edges.size() == 1) {
                const Edge_t* edge = enabled_edges.front();
                // case where this is the first operator created
                if (constructorList.empty()) {
                    constructorList.emplace_back(
                            new ActionOp_t(edge, modelInfo.compute_action_op_id_additive(ModelInformation::sync_index_to_id(sync_index)) + modelInfo.compute_action_op_id_sync_additive(sync_index, condition.automatonIndex, edge->get_id()), model, ModelInformation::sync_index_to_id(sync_index)),
                            modelInfo.compute_action_op_index_sync_additive(sync_index, condition.automatonIndex, edge->get_id())
                    );
                } else {  // case where there are existing ops
                    // product with all existing ops:
                    produce_with(edge, modelInfo.compute_action_op_id_sync_additive(sync_index, condition.automatonIndex, edge->get_id()),
                                 modelInfo.compute_action_op_index_sync_additive(sync_index, condition.automatonIndex, edge->get_id())
                    );
                }
            } else { // case where there are multiple edges (i.e. local nondeterminism)
                // case where there are no existing ops
                if (constructorList.empty()) {
                    for (const Edge_t* edge: enabled_edges) {
                        constructorList.emplace_back(
                                new ActionOp_t(edge, modelInfo.compute_action_op_id_additive(ModelInformation::sync_index_to_id(sync_index)) + modelInfo.compute_action_op_id_sync_additive(sync_index, condition.automatonIndex, edge->get_id()), model, ModelInformation::sync_index_to_id(sync_index)),
                                modelInfo.compute_action_op_index_sync_additive(sync_index, condition.automatonIndex, edge->get_id())
                        );
                    }
                } else { // case where there are existing action ops
                    std::size_t op_index;
                    // duplicate (count-1 copies of) current action op list
                    std::size_t num_duplicates = enabled_edges.size() - 1;
                    std::size_t old_number_of_ops = constructorList.size();
                    for (std::size_t i = 0; i < num_duplicates; ++i) {
                        for (op_index = 0; op_index < old_number_of_ops; ++op_index) {
                            constructorList.emplace_back(
                                    new ActionOp_t(*constructorList[op_index].first),
                                    constructorList[op_index].second
                            );
                        }
                    }
                    // products with existing action ops
                    std::size_t edge_counter = 0;
                    for (const Edge_t* edge: enabled_edges) {
                        for (op_index = 0; op_index < old_number_of_ops; ++op_index) {
                            auto& op_index_pair = constructorList[edge_counter * old_number_of_ops + op_index];
                            op_index_pair.first->product_with(edge, modelInfo.compute_action_op_id_sync_additive(sync_index, condition.automatonIndex, edge->get_id()));
                            op_index_pair.second += modelInfo.compute_action_op_index_sync_additive(sync_index, condition.automatonIndex, edge->get_id());
                        }
                        ++edge_counter;
                    }

                }
            }
        }

    };


}


#endif //PLAJA_ACTION_OP_CONSTRUCTION_H
